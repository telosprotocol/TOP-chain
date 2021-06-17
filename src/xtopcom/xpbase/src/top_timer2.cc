#include "top_timer_xbase.h"

#include <assert.h>
#include <vector>
#include <iostream>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"

namespace top {
namespace base {

// --------------------------------------------------------------------------------
Timer2::~Timer2() {}

// --------------------------------------------------------------------------------
TimerManager* TimerManager::Instance() {
    static auto ins = CreateInstance();
    return ins.get();
}

std::shared_ptr<TimerManager> TimerManager::CreateInstance() {
    // return std::make_shared<TimerManagerAsio>();
    return std::make_shared<TimerManagerXbase>();
}

TimerManager::~TimerManager() {}

// --------------------------------------------------------------------------------
TimerGuard::TimerGuard() {}

void TimerGuard::Start(TimerManager* timer_manager, int delay, int repeat, TimerFunc func, const std::string& name) {
    timer_ = timer_manager->CreateTimer(delay, repeat, func, name);
}

TimerGuard::~TimerGuard() {
    if (timer_) {
        timer_->Stop();
        timer_ = nullptr;
    }
}

}  // namespace base
}  // namespace top
