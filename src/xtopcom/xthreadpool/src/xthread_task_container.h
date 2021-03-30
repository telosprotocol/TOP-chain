#pragma once

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <list>
#include <set>
#include "xthread_context.h"
#include "xthread_task.h"
#include "xbasic/xns_macro.h"


NS_BEG2(top, xthreadpool)

class xthread_task_container final {
    using Content = xthread_context::Content;

public:
    using TaskContent = std::shared_ptr<xthread_task>;
    using TaskContainer = std::list<TaskContent>;

    xthread_task_container() : break_out_(false) {}
    ~xthread_task_container() {
        contract_tasks_.clear();
    }

    bool pluck_task_content(TaskContent& task,
        xthread_context* context) {
        std::unique_lock<std::mutex> lock(mutex_);
        bool is_success = true;
        do {
            if (!is_success) {
                if (break_out_)
                    return false;
                cv_.wait_for(lock, std::chrono::milliseconds(500));
            }
            if (break_out_)
                return false;

            is_success = pluck_task_content_impl(task, context);
        } while (!is_success);
        return true;
    }

    bool pluck_task_content_impl(TaskContent& task,
        xthread_context* context) {
        // save account items which pass find_if
        context->check_begin();
        auto it = std::find_if(contract_tasks_.begin(), contract_tasks_.end(),
            [context](TaskContent & t) {
                return context->check_access(t->get_contract(),
                    t->get_account());
            });

        if (it != contract_tasks_.end()) {
            task = *it;
            contract_tasks_.erase(it);
            context->add_check(task->get_contract(),
                task->get_account());
            return true;
        }
        return false;
    }

    void push_task_content(TaskContent task) {
        std::lock_guard<std::mutex> lock(mutex_);

        contract_tasks_.push_back(task);
        cv_.notify_one();
    }

    void push_task_contents(std::vector<TaskContent>& tasks) {
        std::lock_guard<std::mutex> lock(mutex_);

        std::for_each(tasks.begin(), tasks.end(),
            [this](TaskContent & t) {
                contract_tasks_.push_back(t);
                cv_.notify_one();
            });
    }

    void release_task_content(TaskContent task,
        xthread_context* context) {
        std::lock_guard<std::mutex> lock(mutex_);
        context->release_check(task->get_contract(),
            task->get_account());
    }

    void break_out() {
        std::lock_guard<std::mutex> lock(mutex_);
        break_out_ = true;
        cv_.notify_all();
    }

private:
    TaskContainer contract_tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool break_out_;
};

NS_END2
