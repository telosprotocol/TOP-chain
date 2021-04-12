#pragma once
#ifndef RATELIMIT_HASH_MAP_H_
#define RATELIMIT_HASH_MAP_H_

#include <mutex>
#include <unordered_map>
#include "xbase/xns_macro.h"



NS_BEG2(top, xChainRPC)

template <class K, class V>
class RatelimitHashmap final {
public:
    using ForeachFunc = std::function<void(K, V)>;
    using ForeachEraseFunc = std::function<bool(K, V)>;

    RatelimitHashmap();
    ~RatelimitHashmap();

    size_t Size();
    bool Find(const K& key, V& val);
    bool Insert(const K& key, const V& val);
    void Foreach(ForeachFunc func) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto p : hashmap_) {
            func(p.first, p.second);
        }
    }

    void ForeachErase(ForeachEraseFunc func) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = hashmap_.begin();
        for (; it != hashmap_.end(); ) {
            if (func(it->first, it->second)) {
                it = hashmap_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    std::unordered_map<K, V> hashmap_;
    std::mutex mutex_;
};

template<class K, class V>
inline RatelimitHashmap<K, V>::RatelimitHashmap()
{}

template<class K, class V>
inline RatelimitHashmap<K, V>::~RatelimitHashmap()
{}

template<class K, class V>
inline size_t RatelimitHashmap<K, V>::Size() {
    return hashmap_.size();
}

template<class K, class V>
inline bool RatelimitHashmap<K, V>::Find(const K& key, V& val) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = hashmap_.find(key);
    if (it != hashmap_.end()) {
        val = it->second;
        return true;
    }
    return false;
}

template<class K, class V>
inline bool RatelimitHashmap<K, V>::Insert(const K& key, const V& val) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto ret = hashmap_.insert(std::make_pair(key, val));
    return ret.second;
}

NS_END2

#endif  // !RATELIMIT_HASH_MAP_H_
