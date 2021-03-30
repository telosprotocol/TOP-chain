#pragma once

#include <vector>
#include <algorithm>

#include "xpbase/base/observer.h"

namespace top {

template<typename Entity>
class Observable
{
public:
    Observable() {}
    virtual ~Observable() {}
    virtual void AddObserver(ObserverPtr<Entity> observer) {
        observers.push_back(observer);
    }
    virtual void DeleteObserver(ObserverPtr<Entity> observer) {
        if (!observer) {
            return;
        }
        uint64_t delete_id = observer->GetObserverID();
        auto it = std::find_if(observers.begin(), observers.end(),
            [&](const ObserverPtr<Entity> observer_ptr) {
            if (!observer_ptr) {
                return false;
            }
            return delete_id == observer_ptr->GetObserverID(); 
        });
        if (observers.end() == it) {
            return;
        }
        observers.erase(it);
    }
    virtual void NotifyObservers(
        int32_t context,
        const std::vector<Entity>& vt_status) {
        for (auto observer : observers) {
            observer->AddEntityList(vt_status);
            observer->Update(context);
            observer->ClearAllEntity();
        }
    }
private:
    std::vector<ObserverPtr<Entity>> observers;
};

}
