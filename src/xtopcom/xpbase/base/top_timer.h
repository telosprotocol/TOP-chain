
#pragma once

#include "top_timer2.h"
#include "top_utils.h"
#include <list>

namespace top {
namespace base {

typedef std::function<void(void)> TimerFunction;

// once callback
// not support CallAfter again!!
class Timer {
public:
    Timer(TimerManager* timer_manager, const std::string& name);
    ~Timer();
    void CallAfter(int64_t wait_us, TimerFunction func);
    void Join();

private:
    TimerManager* timer_manager_{nullptr};
    std::string name_;
    Timer2Ptr timer_;
    DISALLOW_COPY_AND_ASSIGN(Timer);
};

class SingleThreadTimer {
public:
    SingleThreadTimer();
    ~SingleThreadTimer();
    void CallAfter(int64_t wait_us, TimerFunction func);
    void Join();

private:
    typedef std::pair<std::chrono::steady_clock::time_point, TimerFunction> TimerItem;

    void TimerFlies();

    std::list<TimerItem> timer_list_;
    std::mutex timer_list_mutex_;
    std::shared_ptr<std::thread> timer_thread_;
    bool destroy_;
};

// not support Start again!!
class TimerRepeated {
public:
    TimerRepeated(TimerManager* timer_manager, const std::string& name);
    ~TimerRepeated();
    void Start(int64_t wait_us, int64_t repeated_us, TimerFunction func);
    void Join();

private:
    TimerManager* timer_manager_{nullptr};
    std::string name_;
    Timer2Ptr timer_;
    DISALLOW_COPY_AND_ASSIGN(TimerRepeated);
};

}  // namespace base
}  // namespace top
