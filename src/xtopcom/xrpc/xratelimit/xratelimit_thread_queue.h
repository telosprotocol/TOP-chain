#pragma once
#ifndef RATELIMIT_THREAD_QUEUE_H_
#define RATELIMIT_THREAD_QUEUE_H_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <atomic>
#include "xbase/xns_macro.h"



NS_BEG2(top, xChainRPC)

template <class T>
class ThreadQueue {
public:
    ThreadQueue()
        : break_out_(false)
        , size_(0)
    {}

    ~ThreadQueue() {
        break_out();
    }

    size_t size() {
        return static_cast<size_t>(size_);
    }

    bool empty() {
        return static_cast<size_t>(size_) == 0;
    }

    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
        size_++;
        cv_.notify_one();
    }

    bool pluck(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (break_out_)
            return false;

        while (queue_.empty()) {
            cv_.wait(lock);
            if (break_out_)
                return false;
        }
        size_--;
        value = queue_.front();
        queue_.pop();

        return true;
    }

    bool pick(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        } else {
            size_--;
            value = queue_.front();
            queue_.pop();
            return true;
        }
    }

    void break_out() {
        std::lock_guard<std::mutex> lock(mutex_);
        break_out_ = true;
        cv_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
    bool break_out_;
    std::atomic_int size_;
};
NS_END2

#endif  // !RATELIMIT_THREAD_QUEUE_H_
