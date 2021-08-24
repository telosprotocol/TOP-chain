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
    void join() {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

private:
    std::thread thread_;
};

NS_END2

#endif  // !RATELIMIT_SERVER_STAT_H_
