#pragma once
#ifndef RATELIMIT_THREAD_H_
#define RATELIMIT_THREAD_H_

#include <thread>
#include <memory>
#include <functional>
#include "xbase/xns_macro.h"
#include "xbase/xobject.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"

NS_BEG2(top, xChainRPC)

using ThreadFunc = std::function<void(void)>;

class ThreadBase {
public:
    explicit ThreadBase(ThreadFunc func) : func_(func) {}
    virtual ~ThreadBase() {}

protected:
    ThreadFunc func_;
};


class RatelimitThread : public ThreadBase {
public:
    explicit RatelimitThread(ThreadFunc func)
        : ThreadBase(func)
        , thread_(func_)
    {}
    virtual ~RatelimitThread() {
        thread_.join();
    }

private:
    std::thread thread_;
};

class XIOThread : public ThreadBase {
public:
    explicit XIOThread(ThreadFunc func)
        : ThreadBase(func)
        , call_func_([this](top::base::xcall_t& call, const int32_t thread_id,
            const uint64_t time_now_ms)->bool {
                this->func_();
                return true;
            }) {
        thread_ = top::base::xiothread_t::create_thread(
            top::base::xcontext_t::instance(), 0, -1);
        thread_->send_call(call_func_);
    }

    virtual ~XIOThread() {
        thread_->close();
        thread_->release_ref();
    }

private:
    top::base::xcall_t call_func_;
    top::base::xiothread_t* thread_;
};
NS_END2

#endif  // !RATELIMIT_SERVER_STAT_H_
