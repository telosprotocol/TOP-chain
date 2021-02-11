#pragma once
#include <mutex>
#include <memory>
#include <vector>
#include <list>
#include <algorithm>
#include <deque>
#include <unordered_map>
#include "xpbase/base/manager_template.h"

namespace top
{
template<typename ValueType,int32_t N,int32_t RepeatN>
class ContainTemplate {
public:
    ContainTemplate() {}
    virtual ~ContainTemplate() {}
protected:
    virtual bool AddData(
        const ValueType& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it_find = map_.find(value);
        if(it_find != map_.end()) {
            if(it_find->second >= RepeatN) {
                return false;
            }
            ++it_find->second;
            return true;
        }
        map_.insert(std::pair<ValueType,uint32_t>(value,1));
        contain_.push_back(value);
        if (contain_.size() <= N) {
            return true;
        }
        map_.erase(contain_.front());
        contain_.pop_front();
        return true;
    }

    virtual bool FindData(
        const ValueType& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it_find = map_.find(value);
        if(it_find == map_.end()) {
            return false;
        }
        return it_find->second >= RepeatN;
    }

    virtual int32_t Count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return contain_.size();
    }

    virtual void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::unordered_map<ValueType,uint32_t> empty_map;
        std::swap(empty_map, map_);
        std::deque<ValueType> empty_deque;
        std::swap(empty_deque, contain_);
    }

    virtual void GetAllData(std::vector<ValueType>& all_datas) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::copy(contain_.begin(),contain_.end(), std::back_inserter(all_datas));
    }

    virtual void PopAllData(std::vector<ValueType>& all_datas) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::copy(contain_.begin(),contain_.end(), std::back_inserter(all_datas));
        std::unordered_map<ValueType,uint32_t> empty_map;
        std::swap(empty_map, map_);
        std::deque<ValueType> empty_deque;
        std::swap(empty_deque, contain_);
    }

    virtual bool Full() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return contain_.size() >= N;
    }
    mutable std::mutex mutex_;
    std::unordered_map<ValueType,uint32_t> map_;
    std::deque<ValueType> contain_;
};
}  //  namespace top
