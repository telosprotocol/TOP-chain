#pragma once

#include "xpbase/base/top_timer2.h"
#include <chrono>
#include <atomic>
#include <future>
#include <memory>
#include <map>

#include "xbase/xtimer.h"
#include "xbase/xthread.h"

namespace top {
namespace base {

// --------------------------------------------------------------------------------
using OnTimerProc = std::function<void(bool stop)>;
class TimerSink : public base::xtimersink_t {
public:
    TimerSink(const std::string& name, OnTimerProc proc);
    bool on_timer_start(
            const int32_t errorcode,
            const int32_t thread_id,
            const int64_t timer_id,
            const int64_t cur_time_ms,
            const int32_t timeout_ms,
            const int32_t timer_repeat_ms);
    bool on_timer_stop(
            const int32_t errorcode,
            const int32_t thread_id,
            const int64_t timer_id,
            const int64_t cur_time_ms,
            const int32_t timeout_ms,
            const int32_t timer_repeat_ms);
    bool on_timer_fire(
            const int32_t thread_id,
            const int64_t timer_id,
            const int64_t current_time_ms,
            const int32_t start_timeout_ms,
            int32_t& in_out_cur_interval_ms);

private:
    std::string name_;
    OnTimerProc proc_;
};

// --------------------------------------------------------------------------------
class TimerManagerXbase;
class TimerXbase : public Timer2, public std::enable_shared_from_this<TimerXbase> {
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    friend class TimerManagerXbase;
public:
    TimerXbase(TimerManagerXbase* timer_manager, int delay, int repeat, TimerFunc func, const std::string& name);
    ~TimerXbase();
    void Stop(bool wait);
    bool IsStopped();

private:
    void TimerProc(bool stop);
    void Start();
    void Release();

private:
    Mutex mutex_;
    TimerSink* xtimer_sink_{nullptr};
    base::xtimer_t* xtimer_{nullptr};
    bool started_{false};
    bool stopped_{false};
    bool request_stop_{false};
    int delay_{0};
    int repeat_{0};
    TimerFunc func_;
    std::string name_;
    std::atomic<int> run_count_{0};
    std::promise<void> promise_;
    std::shared_future<void> future_;
    TimerManagerXbase* timer_manager_{nullptr};
};

// --------------------------------------------------------------------------------
class TimerManagerXbase : public TimerManager {
    using Mutex = std::recursive_mutex;
    using Lock = std::unique_lock<Mutex>;
public:
    TimerManagerXbase();
    ~TimerManagerXbase();
    void Start(int thread_count);
    void Stop();
    Timer2Ptr CreateTimer(int delay, int repeat, TimerFunc func, const std::string& name);

public:
    size_t TimerCount();
    void RemoveTimer(Timer2Ptr timer);
    base::xtimer_t* CreateTimer(TimerSink* timer_sink);

private:
    Mutex mutex_;
    std::atomic<bool> started_{false};
    std::atomic<bool> stopped_{false};
    std::vector<base::xiothread_t*> threads_;
    size_t next_thread_id_{0};
    std::map<void*, std::shared_ptr<TimerXbase>> timers_;
};

}  // namespace base
}  // namespace top
