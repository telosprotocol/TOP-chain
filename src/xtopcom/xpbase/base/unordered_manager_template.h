#pragma once

#include <unordered_map>
#include <mutex>

namespace top {
template<typename KeyType, typename ValueType>
class UnorderedManagerTemplate {
    typedef std::unordered_map<KeyType, ValueType> MapKeyToValue;
public:
    UnorderedManagerTemplate() {}
    virtual ~UnorderedManagerTemplate() {}
protected:
    virtual bool BackKey(KeyType& out_key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.begin();
        if (map_.end() == it) {
            return false;
        }
        out_key = it->first;
        return true;
    }
    virtual bool AddData(
        const KeyType& in_key,
        const ValueType& in_value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it_find = map_.find(in_key);
        if (it_find != map_.end()) {
            return false;
        }
        map_.insert(std::make_pair<in_key, in_value>);
        return true;
    }
    virtual bool FindData(
        const KeyType& in_key,
        ValueType& out_value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it_find = map_.find(in_key);
        if (it_find == map_.end()) {
            return false;
        }
        out_value = it_find->second;
        return true;
    }
    virtual bool HaveKey(
        const KeyType& in_key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it_find = map_.find(in_key);
        return it_find != map_.end();
    }
    mutable std::mutex mutex_;
    MapKeyToValue map_;
};
}