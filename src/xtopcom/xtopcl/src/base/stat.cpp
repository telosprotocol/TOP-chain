#include "stat.h"

#include <chrono>

namespace xChainSDK
{
    stat::stat()
        : thread_(&stat::ThreadFunc, this)
    {
    }

    stat::~stat()
    {
        shut_down_ = true;
        cv_.notify_all();
        thread_.join();
    }

    void stat::do_per_second()
    {
    }

    void stat::ThreadFunc()
    {
        while (!shut_down_)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, std::chrono::seconds(1));

            do_per_second();
        }
    }

    request_stat::request_stat()
    {
    }

    request_stat::~request_stat()
    {
    }

    void request_stat::do_per_second()
    {
        auto count = response_succ_count_ - store_response_succ_;
        store_response_succ_ = response_succ_count_;
        if (count > response_succ_count_max_)
        {
            response_succ_count_max_ = count;
        }
        if (count < response_succ_count_min_ || response_succ_count_min_ == 0)
        {
            response_succ_count_min_ = count;
        }
    }

    void request_stat::print_stat()
    {
    }
    void request_stat::clear()
    {
        request_count_ = 0;
        response_count_ = 0;
        response_succ_count_ = 0;
        response_fail_count_ = 0;

        response_succ_count_max_ = 0;
        response_succ_count_min_ = 0;

        store_response_succ_ = 0;
    }
    void request_stat::check_over()
    {
        if (response_count_ == test_count_)
        {
            print_stat();
            clear();
        }
    }
}