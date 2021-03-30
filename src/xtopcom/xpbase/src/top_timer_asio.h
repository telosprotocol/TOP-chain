#pragma once

#include "xpbase/base/top_timer2.h"
#include <chrono>
#include <atomic>
#include <future>
#include <memory>
#include <map>
#include <thread>
#include <set>

#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif//ASIO_STANDALONE
#include "asio.hpp"

namespace top {
namespace base {

// --------------------------------------------------------------------------------
using SteadyTimer = asio::basic_waitable_timer<std::chrono::steady_clock>;
class TimerManagerAsio;
class TimerAsio : public Timer2, public std::enable_shared_from_this<TimerAsio> {
    using Mutex = std::mutex;
    using Lock = std::unique_lock<Mutex>;
    friend class TimerManagerAsio;
public:
    TimerAsio(TimerManagerAsio* timer_manager, int delay, int repeat, TimerFunc func, const std::string& name,
            std::set<std::thread::id> id_set);
    ~TimerAsio();
    void Stop(bool wait);
    bool IsStopped();

private:
    void TimerProc(asio::error_code ec);
    void Start();
    void Release();

private:
    Mutex mutex_;
    std::shared_ptr<SteadyTimer> steady_timer_;
    bool started_{false};
    bool stopped_{false};
    bool request_stop_{false};
    int delay_{0};
    int repeat_{0};
    TimerFunc func_;
    std::string name_;
    std::set<std::thread::id> id_set_;  // all the threads which may call the timer
    std::atomic<int> run_count_{0};
    std::promise<void> promise_;
    std::shared_future<void> future_;
    TimerManagerAsio* timer_manager_{nullptr};
};

// --------------------------------------------------------------------------------
class TimerManagerAsio : public TimerManager {
    using Mutex = std::recursive_mutex;
    using Lock = std::unique_lock<Mutex>;
public:
    TimerManagerAsio();
    ~TimerManagerAsio();
    void Start(int thread_count);
    void Stop();
    Timer2Ptr CreateTimer(int delay, int repeat, TimerFunc func, const std::string& name);    

public:
    asio::io_service& GetIoService();
    size_t TimerCount();
    void RemoveTimer(Timer2Ptr timer);

private:
    void ThreadProc();

private:
    asio::io_service service_;
    asio::io_service::work work_{service_};
    Mutex mutex_;
    std::atomic<bool> started_{false};
    std::atomic<bool> stopped_{false};
    std::vector<std::shared_ptr<std::thread>> threads_;
    std::set<std::thread::id> id_set_;
    std::map<void*, std::shared_ptr<TimerAsio>> timers_;
};

}  // namespace base
}  // namespace top
