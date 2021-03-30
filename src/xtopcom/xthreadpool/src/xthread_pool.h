
#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>
#include <vector>
#include "xthread_task_container.h"
#include "xthread_context.h"


NS_BEG2(top, xthreadpool)

class xthread_pool;
class xpool_thread {
public:
    explicit xpool_thread(xthread_pool* const pool);
    ~xpool_thread();
    void thread_method();
    void shut_down();

private:
    xthread_pool* thread_pool_{ nullptr };
    std::thread thread_;
    bool shut_down_{ false };
};


class xthread_pool final {
    using ThreadType = xpool_thread;
    using ThreadArrayContent = std::shared_ptr<ThreadType>;
    using ThreadArray = std::vector<ThreadArrayContent>;
    using TaskContent = xthread_task_container::TaskContent;
public:
    xthread_pool(xthread_task_container* container,
        xthread_context* context, const size_t thread_count);
    ~xthread_pool();

    void on_thread_method();
    size_t thread_count();

private:
    size_t thread_count_;
    ThreadArray threads_;
    xthread_task_container* task_container_{ nullptr };
    xthread_context* thread_context_{ nullptr };
};

NS_END2
