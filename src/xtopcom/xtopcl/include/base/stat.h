#pragma once


#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace xChainSDK
{
    class stat
    {
    public:
        stat();
        virtual ~stat();

        void ThreadFunc();
        virtual void do_per_second();

    private:
        std::mutex mutex_;
        std::condition_variable cv_;
        std::thread thread_;
        bool shut_down_{ false };
    };

    class request_stat : public stat
    {
    public:
        request_stat();
        virtual ~request_stat();

        virtual void do_per_second();
        void print_stat();
        void clear();
        void check_over();

        std::atomic_int request_count_{ 0 };
        std::atomic_int response_count_{ 0 };
        std::atomic_int response_succ_count_{ 0 };
        std::atomic_int response_fail_count_{ 0 };

        std::atomic_int response_succ_count_max_{ 0 };
        std::atomic_int response_succ_count_min_{ 0 };

        int store_response_succ_{ 0 };

        int test_count_{ 0 };
    };
}