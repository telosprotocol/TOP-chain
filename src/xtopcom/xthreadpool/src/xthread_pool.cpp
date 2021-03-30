#include "xthread_pool.h"
#include <algorithm>
#include <assert.h>


NS_BEG2(top, xthreadpool)

xpool_thread::xpool_thread(xthread_pool* const pool)
    : thread_pool_(pool)
    , thread_(&xpool_thread::thread_method, this) {
}

xpool_thread::~xpool_thread() {
    thread_.join();
}

void xpool_thread::thread_method() {
    thread_pool_->on_thread_method();
}

void xpool_thread::shut_down() {
    shut_down_ = true;
}




xthread_pool::xthread_pool(xthread_task_container* container,
    xthread_context* context, const size_t thread_count)
    : thread_count_(thread_count)
    , task_container_(container)
    , thread_context_(context) {
    for (size_t i{ 0 }; i < thread_count; ++i) {
        auto t = std::make_shared<ThreadType>(this);
        assert(t != nullptr);
        threads_.push_back(t);
    }
}

xthread_pool::~xthread_pool() {
    threads_.clear();
}

void xthread_pool::on_thread_method() {
    TaskContent task;
    while (task_container_->pluck_task_content(task, thread_context_)) {
        task->do_task();
        task_container_->release_task_content(task, thread_context_);
    }
}

size_t xthread_pool::thread_count() {
    return thread_count_;
}

NS_END2
