#pragma once

#include <memory>
#include <vector>
#include <mutex>

namespace top {

template<typename Entity>
class Observer {
public:
    Observer(uint64_t observer_id) : observer_id_(observer_id) {}
    virtual ~Observer() {}
    virtual void Update(
        int32_t) = 0;
    uint64_t GetObserverID() { return observer_id_; }
    const std::vector<Entity>& GetAllEntity() {
        std::lock_guard<std::mutex> lock(mutex_);
        return vt_entity_;
    }
    void AddEntity(const Entity& entity) {
        std::lock_guard<std::mutex> lock(mutex_);
        vt_entity_.push_back(entity);
    }
    void AddEntityList(const std::vector<Entity>& vt_entity) {
        std::lock_guard<std::mutex> lock(mutex_);
        vt_entity_ = vt_entity;
    }
    void ClearAllEntity() {
        std::lock_guard<std::mutex> lock(mutex_);
        vt_entity_.clear();
    }
protected:
    uint64_t observer_id_;
    std::mutex mutex_;
    std::vector<Entity> vt_entity_;
};

template<typename Entity>
using ObserverPtr = std::shared_ptr<Observer<Entity>>;

}