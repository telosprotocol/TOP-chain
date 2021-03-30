
#include "top_timer_impl.h"

#include <assert.h>
#include <vector>
#include <iostream>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"

namespace top {
namespace base {

static const int64_t kTimerSleepUs = 50 * 1000;

// Timer --------------------------------------------------------------------------------
Timer::Timer(TimerManager* timer_manager, const std::string& name) {
    assert(timer_manager);
    timer_manager_ = timer_manager;
    name_ = name;
}

Timer::~Timer() {
    Join();
}

void Timer::CallAfter(int64_t wait_us, TimerFunction func) {
    assert(!timer_);
    timer_ = timer_manager_->CreateTimer(wait_us / 1000, 0, func, name_);
}

void Timer::Join() {
    if (timer_) {
        timer_->Stop();
        timer_ = nullptr;
    }
}

// SingleThreadTimer --------------------------------------------------------------------------------
SingleThreadTimer::SingleThreadTimer()
        : timer_list_(),
          timer_list_mutex_(),
          timer_thread_(nullptr),
          destroy_(false) {
    timer_thread_.reset(new std::thread(&SingleThreadTimer::TimerFlies, this));
}

SingleThreadTimer::~SingleThreadTimer() {
    Join();
}

void SingleThreadTimer::Join() {
    destroy_ = true;
    if (timer_thread_) {
        timer_thread_->join();
        timer_thread_ = nullptr;
    }

    {
        std::unique_lock<std::mutex> lock(timer_list_mutex_);
        timer_list_.clear();
    }
}

void SingleThreadTimer::CallAfter(int64_t wait_us, TimerFunction func) {
    std::unique_lock<std::mutex> lock(timer_list_mutex_);
    auto tp_timeout = std::chrono::steady_clock::now() + std::chrono::microseconds(wait_us);
    timer_list_.push_back(std::make_pair(tp_timeout, func));
}

void SingleThreadTimer::TimerFlies() {
    while (!destroy_) {
        std::vector<TimerFunction> func_vec;
        {
            std::unique_lock<std::mutex> lock(timer_list_mutex_);
            auto tp_now = std::chrono::steady_clock::now();
            auto iter = timer_list_.begin();
            while (iter != timer_list_.end()) {
                if (iter->first <= tp_now) {
                    func_vec.push_back(iter->second);
                    iter = timer_list_.erase(iter);
                } else {
                    ++iter;
                }
            }
        }

        for (auto iter = func_vec.begin(); iter != func_vec.end(); ++iter) {
            (*iter)();
        }
        SleepUs(kTimerSleepUs);
    }
}

// TimerRepeated --------------------------------------------------------------------------------
TimerRepeated::TimerRepeated(TimerManager* timer_manager, const std::string& name) {
    assert(timer_manager);
    timer_manager_ = timer_manager;
    name_ = name;
}

TimerRepeated::~TimerRepeated() {
    Join();
}

void TimerRepeated::Start(int64_t wait_us, int64_t repeated_us, TimerFunction func) {
    assert(!timer_);
    timer_ = timer_manager_->CreateTimer(wait_us / 1000, repeated_us / 1000, func, name_);
}

void TimerRepeated::Join() {
    if (timer_) {
        timer_->Stop();
        timer_ = nullptr;
    }
}

}  // namespace base
}  // namespace top
