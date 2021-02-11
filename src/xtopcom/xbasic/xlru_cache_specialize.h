// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <mutex>
#include "xbasic/xns_macro.h"

NS_BEG2(top, basic)
// from https://stackoverflow.com/questions/2504178/lru-cache-design
template<typename key, typename value>
class xlru_cache_specialize {
 public:
    using key_value = std::pair<key, value>;

    xlru_cache_specialize(size_t max_size) :
        m_max_size(max_size) {
    }

    void put(const key& k, const value& v) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_item_map.find(k);
        if (it != m_item_map.end()) {
            m_item_list.erase(it->second);
            m_item_map.erase(it);
        }
        m_item_list.push_front(std::make_pair(k, v));
        m_item_map.insert(std::make_pair(k, m_item_list.begin()));
        clean();
    }

    bool get(const key& k, value& v) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_item_map.find(k);
        if (it == m_item_map.end()) {
            return false;
        } else {
            v = it->second->second;
            return true;
        }
    }

    bool back(key& k, value& v) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_item_list.empty()) {
            return false;
        }
        const auto& item = m_item_list.back();
        k = item.first;
        v = item.second;
        return true;
    }

    void erase(const key& k) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_item_map.find(k);
        if (it != m_item_map.end()) {
            m_item_list.erase(it->second);
            m_item_map.erase(it);
        }
    }

 private:
    void clean() {
        while (m_item_map.size() > m_max_size) {
            auto last_it = m_item_list.end();
            last_it--;
            m_item_map.erase(last_it->first);
            m_item_list.pop_back();
        }
    }

 private:
    std::list<key_value> m_item_list;
    std::unordered_map<key, decltype(m_item_list.begin())> m_item_map;
    size_t m_max_size;
    std::mutex m_mutex;
};
NS_END2
