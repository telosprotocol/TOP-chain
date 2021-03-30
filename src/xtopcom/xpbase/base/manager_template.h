#pragma once
#include <map>
#include <mutex>
#include <memory>
#include <vector>

namespace top
{
template<typename KeyType,typename ValueType>
class ManagerTemplate {
typedef std::map<KeyType, ValueType> MapKeyToValue;
using MapIterator = typename MapKeyToValue::iterator;
public:
    ManagerTemplate() {}
    virtual ~ManagerTemplate() {}
protected:
    virtual bool ResetDatas(const MapKeyToValue& map) {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
        map_ = map;
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
        map_[in_key] = in_value;
        return true;
    }

    virtual void DeleteKey(
        const KeyType& in_key) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it_find = map_.find(in_key);
        if (it_find == map_.end()) {
            return;
        }
        map_.erase(in_key);
    }

    virtual bool ModData(
        const KeyType& in_key,
        const ValueType& in_value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it_find = map_.find(in_key);
        if (it_find == map_.end()) {
            return false;
        }
        map_[in_key] = in_value;
        return true;
    }

    virtual bool HaveKey(
        const KeyType& in_key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it_find = map_.find(in_key);
        return it_find != map_.end();
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

    virtual int32_t Count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.size();
    }

    virtual void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
    }
 
    virtual void GetAllData(std::vector<ValueType>& all_datas) {
        std::lock_guard<std::mutex> lock(mutex_);
        MapIterator it = map_.begin();
        for(; map_.end() != it;++it) {
            all_datas.push_back(it->second);
        }
    }

    virtual void GetAllData(MapKeyToValue& all_datas) {
        std::lock_guard<std::mutex> lock(mutex_);
        all_datas = map_;
    }

    virtual MapIterator Begin() {
        std::lock_guard<std::mutex> lock(mutex_);
        iter_ = map_.begin();
        return iter_;
    }
    virtual MapIterator Next() {
        std::lock_guard<std::mutex> lock(mutex_);
        ++iter_;
        return iter_;
    }
    virtual MapIterator End() {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_.end();
    }
    mutable std::mutex mutex_;
    MapKeyToValue map_;
    mutable MapIterator iter_;
};
}  //  namespace top