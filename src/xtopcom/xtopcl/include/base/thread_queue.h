#pragma once
#pragma once
#ifndef THREAD_QUEUE_H_
#define THREAD_QUEUE_H_

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>

namespace xChainSDK {

template <class T>
class ThreadQueue {
public:
    ThreadQueue() : break_out_(false), size_(0) {
    }

    ~ThreadQueue() {
        break_out();
    }

    size_t size() {
        return static_cast<size_t>(size_);
    }

    bool empty() {
        return static_cast<size_t>(size_) == 0;
    }

    void push(const T & value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(value);
        size_++;
        cv_.notify_one();
    }

    bool pluck(T & value) {
        std::unique_lock<std::mutex> lock(mutex_);
        // if (break_out_)
        // {
        // std::cout << "pluct wait 59 " << queue_.size() << std::this_thread::get_id() << " " << this << std::endl;
        //     return false;
        // }
        while (queue_.empty()) {
            cv_.wait(lock);
            // if (break_out_)
            // {
            //     return false;
            // }
        }
        size_--;
        value = queue_.front();
        queue_.pop();
        lock.unlock();

        return true;
    }

    bool pick(T & value) {
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
}  // namespace xChainSDK

#endif  // !THREAD_QUEUE_H_
