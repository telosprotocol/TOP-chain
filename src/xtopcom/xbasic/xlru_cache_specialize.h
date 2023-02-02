// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cassert>
#include <list>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <vector>

NS_BEG2(top, basic)
// from https://stackoverflow.com/questions/2504178/lru-cache-design
template<typename KeyT, typename ValueT>
class xlru_cache_specialize {
private:
    std::list<std::pair<KeyT, ValueT>> item_list_;
    std::unordered_map<KeyT, typename std::list<std::pair<KeyT, ValueT>>::iterator> item_map_;
    size_t const max_size_;
    mutable std::mutex mutex_;

public:
    xlru_cache_specialize(size_t const max_size) : max_size_{max_size} {
    }

    void put(const KeyT& key, const ValueT& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = item_map_.find(key);
        if (it != item_map_.end()) {
            item_list_.erase(it->second);
            item_map_.erase(it);
        }
        item_list_.push_front({key, value});
        item_map_.insert({key, item_list_.begin()});
        clean_with_lock_hold();
    }

    void insert(std::pair<KeyT, ValueT> const & item) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = item_map_.find(item.first);
        if (it != item_map_.end()) {
            item_list_.erase(it->second);
            item_map_.erase(it);
        }

        item_list_.push_front(item);
        item_map_.insert({item.first, item_list_.begin()});
        clean_with_lock_hold();
    }

    bool get(const KeyT& key, ValueT& value) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = item_map_.find(key);
        if (it == item_map_.end()) {
            return false;
        }

        assert(key == it->second->first);

        value = it->second->second;
        return true;
    }

    bool back(KeyT& key, ValueT& value) const {
        std::lock_guard<std::mutex> lock{mutex_};

        if (item_list_.empty()) {
            return false;
        }

        const auto& item = item_list_.back();
        key = item.first;
        value = item.second;

        return true;
    }

    void erase(const KeyT& key) {
        std::lock_guard<std::mutex> lock{mutex_};
        auto it = item_map_.find(key);
        if (it != item_map_.end()) {
            item_list_.erase(it->second);
            item_map_.erase(it);
        }
    }

    bool contains(KeyT const & key) const {
        std::lock_guard<std::mutex> lock{mutex_};
        return item_map_.find(key) != std::end(item_map_);
    }

    ValueT const & at(KeyT const & key) const {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = item_map_.find(key);
        if (it == item_map_.end()) {
            throw std::out_of_range{"lru cache out of range "};  // NOLINT(cert-err60-cpp)
        }

        assert(key == it->second->first);

        return it->second->second;
    }

private:
    void clean_with_lock_hold() {
        while (item_map_.size() > max_size_) {
            auto last_it = item_list_.end();
            --last_it;
            item_map_.erase(last_it->first);
            item_list_.pop_back();
        }
    }
};

template <typename KeyT, typename ValueT, typename MutexT>
class xtop_lru_cache {
private:
    std::list<std::pair<KeyT, ValueT>> item_list_;
    std::unordered_map<KeyT, typename std::list<std::pair<KeyT, ValueT>>::iterator> item_map_;
    size_t const max_size_;
    mutable MutexT mutex_;

public:
    xtop_lru_cache(size_t const max_size) : max_size_{max_size} {
    }

    std::vector<std::pair<KeyT, ValueT>> put(const KeyT & key, const ValueT & value) {
        std::lock_guard<MutexT> lock(mutex_);

        std::vector<std::pair<KeyT, ValueT>> erased_vec;
        auto it = item_map_.find(key);
        if (it != item_map_.end()) {
#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)
            erased_vec.push_back(std::make_pair(key, it->second->second));
#endif
            item_list_.erase(it->second);
            item_map_.erase(it);
        }
        item_list_.push_front({key, value});
        item_map_.insert({key, item_list_.begin()});
        clean_with_lock_hold(erased_vec);
        return erased_vec;
    }

    std::vector<std::pair<KeyT, ValueT>> put(const KeyT & key, ValueT && value) {
        std::lock_guard<MutexT> lock(mutex_);

        std::vector<std::pair<KeyT, ValueT>> erased_vec;
        auto it = item_map_.find(key);
        if (it != item_map_.end()) {
#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)
            erased_vec.push_back(std::make_pair(key, it->second->second));
#endif
            item_list_.erase(it->second);
            item_map_.erase(it);
        }
        item_list_.push_front({key, std::move(value)});
        item_map_.insert({key, item_list_.begin()});
        clean_with_lock_hold(erased_vec);
        return erased_vec;
    }


    bool get(const KeyT & key, ValueT & value) const {
        std::lock_guard<MutexT> lock(mutex_);

        auto it = item_map_.find(key);
        if (it == item_map_.end()) {
            return false;
        }

        assert(key == it->second->first);

        value = it->second->second;
        return true;
    }

    //bool back(KeyT & key, ValueT & value) const {
    //    std::lock_guard<MutexT> lock{mutex_};

    //    if (item_list_.empty()) {
    //        return false;
    //    }

    //    const auto & item = item_list_.back();
    //    key = item.first;
    //    value = item.second;

    //    return true;
    //}

    std::vector<std::pair<KeyT, ValueT>> erase(const KeyT & key) {
        std::lock_guard<MutexT> lock{mutex_};
        std::vector<std::pair<KeyT, ValueT>> erased_vec;
        auto it = item_map_.find(key);
        if (it != item_map_.end()) {
#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)
            erased_vec.push_back(std::make_pair(key, it->second->second));
#endif
            item_list_.erase(it->second);
            item_map_.erase(it);
        }
        return erased_vec;
    }

    bool contains(KeyT const & key) const {
        std::lock_guard<MutexT> lock{mutex_};
        return item_map_.find(key) != std::end(item_map_);
    }

    void clear() {
        std::lock_guard<MutexT> lock{mutex_};
        item_map_.clear();
        item_list_.clear();
    }

private:
    void clean_with_lock_hold(std::vector<std::pair<KeyT, ValueT>> & erased_vec) {
        while (item_map_.size() > max_size_) {
            auto last_it = item_list_.end();
            --last_it;
#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)
            erased_vec.push_back(std::make_pair(last_it->first, last_it->second));
#endif
            item_map_.erase(last_it->first);
            item_list_.pop_back();
        }
    }
};

template <typename KeyT, typename ValueT = KeyT, typename MutexT = std::mutex>
using xlru_cache_t = xtop_lru_cache<KeyT, ValueT, MutexT>;

NS_END2
