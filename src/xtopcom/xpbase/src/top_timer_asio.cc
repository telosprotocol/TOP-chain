
#include "top_timer_asio.h"

#include <assert.h>
#include <vector>
#include <iostream>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"

namespace top {
namespace base {

static const uint32_t kTimerThreadNum = std::thread::hardware_concurrency() * 2;

// --------------------------------------------------------------------------------
TimerAsio::TimerAsio(TimerManagerAsio* timer_manager, int delay, int repeat, TimerFunc func, const std::string& name,
        std::set<std::thread::id> id_set) {
    assert(delay > 0);
    assert(repeat >= 0);
    assert(func != nullptr);

    steady_timer_ = std::make_shared<SteadyTimer>(timer_manager->GetIoService());
    delay_ = delay;
    repeat_ = repeat;
    func_ = func;
    name_ = name;
    id_set_ = id_set;
    timer_manager_ = timer_manager;
    TOP_INFO("timer(%s) ctor", name_.c_str());
}

TimerAsio::~TimerAsio() {
    TOP_INFO("timer(%s) dtor", name_.c_str());
}

void TimerAsio::Stop(bool wait) {
    TOP_INFO("timer(%s) stopping", name_.c_str());
    bool first_time_to_stop = false;
    {
        Lock lock(mutex_);
        if (!started_ || stopped_) {
            return;
        }

        if (!request_stop_) {
            func_ = nullptr;
            request_stop_ = true;
            steady_timer_->cancel();
            first_time_to_stop = true;
        } else {
            return;  // no need stop again internally
        }
    }

    const auto tid = std::this_thread::get_id();
    // {
    //     for (auto& v : id_set_) {
    //         std::cout << "all thread id: " << v << std::endl;
    //     }
    //     std::cout << "current thread id: " << tid << std::endl;
    //     auto it = id_set_.find(tid);
    //     std::cout << (it == id_set_.end() ? "not found" : "found") << std::endl;
    //     std::cout << "count: " << id_set_.count(tid) << std::endl;
    // }
    if (wait && first_time_to_stop) {
        if (id_set_.find(tid) == id_set_.end()) {  // other thread calls
            future_.get();
        } else {
            TOP_WARN("timer(%s) skip waiting, because the calling thread may the timer runner, that may cause deadlock");
        }
    }

    TOP_INFO("timer(%s) stopped", name_.c_str());
}

bool TimerAsio::IsStopped() {
    Lock lock(mutex_);
    if (!started_) {
        return true;
    }

    return stopped_;
}

void TimerAsio::Start() {
    Lock lock(mutex_);
    assert(!started_);

    steady_timer_->expires_from_now(std::chrono::milliseconds(delay_));
    using namespace std::placeholders;
    auto f = std::bind(&TimerAsio::TimerProc, shared_from_this(), _1);
    steady_timer_->async_wait(f);

    future_ = promise_.get_future();
    started_ = true;
    TOP_INFO("timer(%s) started", name_.c_str());
}

void TimerAsio::Release() {
    timer_manager_->RemoveTimer(shared_from_this());
}

void TimerAsio::TimerProc(asio::error_code ec) {
    run_count_ += 1;

    TimerFunc func;
    {
        Lock lock(mutex_);
        if (request_stop_ || ec) {
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
        if (repeat_ > 0) {
            steady_timer_->expires_from_now(std::chrono::milliseconds(repeat_));
            using namespace std::placeholders;
            auto f = std::bind(&TimerAsio::TimerProc, shared_from_this(), _1);
            steady_timer_->async_wait(f);
            return;
        }
    }

    // run over! (repteat == 0)
    Lock lock(mutex_);
    func_ = nullptr;
    stopped_ = true;
    Release();
    promise_.set_value();
    TOP_INFO("timer(%s) run over", name_.c_str());
}

// --------------------------------------------------------------------------------
TimerManagerAsio::TimerManagerAsio() {
    TOP_INFO("timer manager inited");
}

TimerManagerAsio::~TimerManagerAsio() {
    Stop();
}

Timer2Ptr TimerManagerAsio::CreateTimer(int delay, int repeat, TimerFunc func, const std::string& name) {
    Lock lock(mutex_);
    if (!started_) {
        Start(0);
    }

    auto timer = std::make_shared<TimerAsio>(this, delay, repeat, func, name, id_set_);
    timers_[timer.get()] = timer;
    timer->Start();
    return timer;
}

void TimerManagerAsio::Start(int thread_count) {
    TOP_INFO("timer manager Start(thread_count=%d)", thread_count);
    Lock lock(mutex_);
    if (started_) {
        TOP_INFO("timer manager started before, ignore the action");
        return;
    }

    if (thread_count == 0) {
        thread_count = kTimerThreadNum;
    }

    for (int i = 0; i < thread_count; ++i) {
        TOP_INFO("starting thread(TimerManagerAsio)-%d ...", i);
        auto f = std::bind(&TimerManagerAsio::ThreadProc, this);
        auto thread = std::make_shared<std::thread>(f);
        threads_.push_back(thread);
        id_set_.insert(thread->get_id());  // maybe use one thread per io_context, so no need such a set
    }

    started_ = true;
    TOP_INFO("timer manager started");
}

void TimerManagerAsio::Stop() {
    Lock lock(mutex_);
    if (!started_ || stopped_) {
        return;
    }

    service_.stop();
    for (auto& thread : threads_) {
        thread->join();
        TOP_INFO("thread(TimerManagerAsio) stopped");
    }
    threads_.clear();
    timers_.clear();
    stopped_ = true;
    TOP_INFO("timer manager stopped");
}

asio::io_service& TimerManagerAsio::GetIoService() {
    return service_;
}

size_t TimerManagerAsio::TimerCount() {
    Lock lock(mutex_);
    return timers_.size();
}

void TimerManagerAsio::RemoveTimer(Timer2Ptr timer) {
    Lock lock(mutex_);
    timers_.erase(timer.get());
}

void TimerManagerAsio::ThreadProc() {
    TOP_INFO("timer manager threadproc");
    service_.run();
}

}  // namespace base
}  // namespace top
