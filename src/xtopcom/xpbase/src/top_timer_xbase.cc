#include "top_timer_xbase.h"
// #include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_string_util.h"
#include <assert.h>

namespace top {
namespace base {

// --------------------------------------------------------------------------------
TimerSink::TimerSink(const std::string& name, OnTimerProc proc) {
    name_ = name;
    proc_ = proc;
}

bool TimerSink::on_timer_start(
        const int32_t errorcode,
        const int32_t thread_id,
        const int64_t timer_id,
        const int64_t cur_time_ms,
        const int32_t timeout_ms,
        const int32_t timer_repeat_ms) {
    xdbg("on_timer_start(%s)", name_.c_str());
    return true;
}

bool TimerSink::on_timer_stop(
            const int32_t errorcode,
            const int32_t thread_id,
            const int64_t timer_id,
            const int64_t cur_time_ms,
            const int32_t timeout_ms,
            const int32_t timer_repeat_ms) {
    xdbg("on_timer_stop(%s)", name_.c_str());
    OnTimerProc proc = proc_;
    proc_ = nullptr;
    proc(true);
    xdbg("on_timer_stop(%s) over", name_.c_str());
    return true;
}

bool TimerSink::on_timer_fire(
            const int32_t thread_id,
            const int64_t timer_id,
            const int64_t current_time_ms,
            const int32_t start_timeout_ms,
            int32_t& in_out_cur_interval_ms) {
    // xdbg("on_timer_fire");
    proc_(false);
    return true;
}

// --------------------------------------------------------------------------------
TimerXbase::TimerXbase(TimerManagerXbase* timer_manager, int delay, int repeat, TimerFunc func, const std::string& name) {
    assert(delay > 0);
    assert(repeat >= 0);
    assert(func != nullptr);

    using namespace std::placeholders;
    auto f = std::bind(&TimerXbase::TimerProc, this, _1);
    name_ = base::StringUtil::str_fmt("%s_%p", name.c_str(), this);
    xtimer_sink_ = new TimerSink(name_, f);
    xtimer_ = timer_manager->CreateTimer(xtimer_sink_);

    delay_ = delay;
    repeat_ = repeat;
    func_ = func;
    timer_manager_ = timer_manager;
    xdbg("timer_xbase(%s) ctor", name_.c_str());
}

TimerXbase::~TimerXbase() {
    xdbg("timer_xbase(%s) dtor", name_.c_str());
}

void TimerXbase::Stop(bool wait) {
    xdbg("timer_xbase(%s) stopping", name_.c_str());
    bool first_time_to_stop = false;
    {
        Lock lock(mutex_);
        if (!started_ || stopped_) {
            xdbg("timer_xbase(%s) not started or stopped", name_.c_str());
            return;
        }

        if (!request_stop_) {
            xdbg("timer_xbase(%s) request stop", name_.c_str());
            func_ = nullptr;
            request_stop_ = true;
            first_time_to_stop = true;
        } else {
            xdbg("timer_xbase(%s) no need stop again internally", name_.c_str());
            return;  // no need stop again internally
        }
    }

    if (first_time_to_stop) {
        xtimer_->stop();  // will not be called until on_timer_stop!!
    }

    if (wait && first_time_to_stop) {
        xdbg("timer_xbase(%s) waiting for stoping", name_.c_str());
        future_.get();
    }

    xdbg("timer_xbase(%s) stopped", name_.c_str());
}

bool TimerXbase::IsStopped() {
    Lock lock(mutex_);
    if (!started_) {
        return true;
    }

    return stopped_;
}

void TimerXbase::Start() {
    Lock lock(mutex_);
    assert(!started_);

    auto ret = xtimer_->start(delay_, repeat_);
    assert(ret == enum_xcode_successful);

    future_ = promise_.get_future();
    started_ = true;
    xdbg("timer_xbase(%s) started", name_.c_str());
}

void TimerXbase::Release() {
    timer_manager_->RemoveTimer(shared_from_this());
}

void TimerXbase::TimerProc(bool stop) {
    run_count_ += 1;

    TimerFunc func;
    {
        Lock lock(mutex_);
        if (stop) {
            xdbg("timer_xbase(%s) stopping internally", name_.c_str());
            func_ = nullptr;
            stopped_ = true;
            Release();
            promise_.set_value();
            return;
        }

        func = func_;
    }

    if (func) {
        func();
    }
}

// --------------------------------------------------------------------------------
TimerManagerXbase::TimerManagerXbase() {
    xdbg("timer_manager_xbase inited");
}

TimerManagerXbase::~TimerManagerXbase() {
    Stop();
}

Timer2Ptr TimerManagerXbase::CreateTimer(int delay, int repeat, TimerFunc func, const std::string& name) {
    Lock lock(mutex_);
    if (!started_) {
        Start(0);
    }

    auto timer = std::make_shared<TimerXbase>(this, delay, repeat, func, name);
    timers_[timer.get()] = timer;
    timer->Start();
    return timer;
}

void TimerManagerXbase::Start(int thread_count) {
    xdbg("timer_manager_xbase Start(thread_count=%d)", thread_count);
    Lock lock(mutex_);
    if (started_) {
        xdbg("timer_manager_xbase started before, ignore the action");
        return;
    }

    /*
    if (thread_count == 0) {
        thread_count = std::thread::hardware_concurrency();
        if (thread_count < 1 || thread_count > 4) {
            thread_count = 4;
        }
    }
    */
    thread_count = 1; // using one thread maybe enough

    int32_t time_out_ms = -1;
    int32_t thread_type = base::xiothread_t::enum_xthread_type_private;
    for (int i = 0; i < thread_count; ++i) {
        auto thread = base::xiothread_t::create_thread(base::xcontext_t::instance(),
                thread_type, time_out_ms);
        threads_.push_back(thread);
    }

    // SleepMs(1100);  // xbase need about 1000ms to start thread!
    started_ = true;
    xdbg("timer_manager_xbase started");
}

void TimerManagerXbase::Stop() {
    Lock lock(mutex_);
    if (!started_ || stopped_) {
        return;
    }

    // thread can't be stopped
    //   so stop all the timer!!
    // for (auto& thread : threads_) {
    //     thread->close();
    // }

    // TODO: how to stop threads?
    timers_.clear();
    stopped_ = true;
    xdbg("timer_manager_xbase stopped");
}

base::xtimer_t* TimerManagerXbase::CreateTimer(TimerSink* timer_sink) {
    Lock lock(mutex_);
    if (!started_) {
        Start(0);
    }

    auto thread_idx = next_thread_id_ % threads_.size();
    next_thread_id_ += 1;
    auto timer = threads_[thread_idx]->create_timer(timer_sink);
    return timer;
}

size_t TimerManagerXbase::TimerCount() {
    Lock lock(mutex_);
    return timers_.size();
}

void TimerManagerXbase::RemoveTimer(Timer2Ptr timer) {
    Lock lock(mutex_);
    timers_.erase(timer.get());
}

}  // namespace base
}  // namespace top
