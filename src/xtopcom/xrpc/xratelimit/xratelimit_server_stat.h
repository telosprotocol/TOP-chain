#pragma once
#ifndef RATELIMIT_SERVER_STAT_H_
#define RATELIMIT_SERVER_STAT_H_

#include <atomic>
#include <memory>
#include <thread>
#include <condition_variable>
#include "xbasic/xns_macro.h"


NS_BEG2(top, xChainRPC)

class RatelimitServerStat final {
public:
    explicit RatelimitServerStat(bool print_stat = false);
    ~RatelimitServerStat();

    void PrintStat();
    void ThreadFunc();

public:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::thread thread_;
    bool shut_down_{ false };

    std::atomic_int request_worker_idles_{ 0 };
    std::atomic_int response_worker_idles_{ 0 };

    std::atomic_int inqueue_push_requests_{ 0 };
    std::atomic_int inqueue_pop_requests_{ 0 };

    std::atomic_int outqueue_push_requests_{ 0 };
    std::atomic_int outqueue_pop_requests_{ 0 };

    std::atomic_int inqueue_push_responses_{ 0 };
    std::atomic_int inqueue_pop_responses_{ 0 };

    std::atomic_int outqueue_push_responses_{ 0 };
    std::atomic_int outqueue_pop_responses_{ 0 };

    std::atomic_int request_in_per_second_{ 0 };

    uint32_t store_request_in_{ 0 };
    uint32_t max_request_per_second_{ 0 };

    bool print_stat_{ false };
};

NS_END2

#endif  // !RATELIMIT_SERVER_STAT_H_
