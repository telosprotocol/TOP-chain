#include "xratelimit_server_stat.h"

#include <iostream>
#include <chrono>


NS_BEG2(top, xChainRPC)

RatelimitServerStat::RatelimitServerStat(bool print_stat)
    : thread_(&RatelimitServerStat::ThreadFunc, this)
    , print_stat_(print_stat)
{}

RatelimitServerStat::~RatelimitServerStat() {
    shut_down_ = true;
    cv_.notify_all();
    thread_.join();
}

void RatelimitServerStat::PrintStat() {
    if (!print_stat_)
        return;

    std::cout << "push req in: " << inqueue_push_requests_ << std::endl;
    std::cout << "pop req in: " << inqueue_pop_requests_ << std::endl;

    std::cout << "push req out: " << outqueue_push_requests_ << std::endl;
    std::cout << "pop req out: " << outqueue_pop_requests_ << std::endl;

    std::cout << "push resp in: " << inqueue_push_responses_ << std::endl;
    std::cout << "pop resp in: " << inqueue_pop_responses_ << std::endl;

    std::cout << "push resp out: " << outqueue_push_responses_ << std::endl;
    std::cout << "pop resp out: " << outqueue_pop_responses_ << std::endl;

    uint32_t request_per_second = outqueue_push_requests_ - store_request_in_;
    std::cout << "request in per second: " << request_per_second << std::endl;
    store_request_in_ = outqueue_push_requests_;
    if (request_per_second > max_request_per_second_) {
        max_request_per_second_ = request_per_second;
    }
    std::cout << "max request_in per second: " <<
        max_request_per_second_ << std::endl;
}

void RatelimitServerStat::ThreadFunc() {
    while (!shut_down_) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait_for(lock, std::chrono::seconds(1));

        PrintStat();
    }
}

NS_END2
