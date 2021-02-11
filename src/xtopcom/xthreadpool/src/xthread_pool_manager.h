#pragma once


#include "xthread_pool.h"
#include "xthread_task_container.h"
#include "xbasic/xns_macro.h"


NS_BEG2(top, xthreadpool)

class xthread_pool_manager final {
    using TaskContent = xthread_task_container::TaskContent;
public:
    xthread_pool_manager()
        : thread_pool_(&task_container_, &thread_context_, 16)
    {}
    ~xthread_pool_manager() {
        task_container_.break_out();
    }

    void break_out() {
        task_container_.break_out();
    }

    void add_task(TaskContent task) {
        task_container_.push_task_content(task);
    }

    void add_tasks(std::vector<TaskContent>& tasks) {
        task_container_.push_task_contents(tasks);
    }

    template<typename F>
    void add_call(uint64_t account, task_call_static<F> call) {
        auto task = std::make_shared<
            contract_task_caller<
            task_call_static<F>, int>>(
                call, &task_call_static<F>::call);
        task->set_account(account);
        add_task(task);
    }

    xthread_context* get_thread_context() {
        return &thread_context_;
    }

private:
    xthread_task_container task_container_;
    xthread_context thread_context_;
    xthread_pool thread_pool_;
};

NS_END2
