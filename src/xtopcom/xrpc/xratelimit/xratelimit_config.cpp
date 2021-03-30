#include "xratelimit_config.h"



NS_BEG2(top, xChainRPC)

RatelimitConfig::RatelimitConfig()
{}

RatelimitConfig::~RatelimitConfig()
{}

bool RatelimitConfig::Read(const char* config_file) {
    return false;
}

void RatelimitConfig::SetMaxThreads(int max_threads) {
    max_threads_ = max_threads;
}

int RatelimitConfig::GetMaxThreads() const {
    return max_threads_;
}

void RatelimitConfig::SetCheckIp(bool is_check) {
    check_ip_ = is_check;
}

bool RatelimitConfig::GetCheckIp() const {
    return check_ip_;
}

void RatelimitConfig::SetCheckAccount(bool is_check) {
    check_account_ = is_check;
}

bool RatelimitConfig::GetCheckAccount() const {
    return check_account_;
}

void RatelimitConfig::SetCheckAsset(bool is_check) {
    check_asset_ = is_check;
}

bool RatelimitConfig::GetCheckAsset() const {
    return check_asset_;
}

void RatelimitConfig::SetRatePerSecond(uint32_t val) {
    rate_per_second_ = val;
}

uint32_t RatelimitConfig::GetRatePerSecond() const {
    return rate_per_second_;
}

void RatelimitConfig::SetBurstSize(uint32_t val) {
    burst_size_ = val;
}

uint32_t RatelimitConfig::GetBurstSize() const {
    return burst_size_;
}

void RatelimitConfig::SetCacheTimeoutSecond(uint32_t val) {
    cache_timeout_second_ = val;
}

uint32_t RatelimitConfig::GetCacheTimeoutSecond() const {
    return cache_timeout_second_;
}

void RatelimitConfig::SetAllowBorrowSize(uint32_t val) {
    allow_borrow_size_ = val;
}

uint32_t RatelimitConfig::GetAllowBorrowSize() const {
    return allow_borrow_size_;
}

void RatelimitConfig::SetRequestToken(uint32_t val) {
    request_token_ = val;
}

uint32_t RatelimitConfig::GetRequestToken() const {
    return request_token_;
}

void RatelimitConfig::SetRequestOutThreadCount(uint32_t val) {
    request_out_thread_count_ = val;
}

uint32_t RatelimitConfig::GetRequestOutThreadCount() const {
    return request_out_thread_count_;
}

void RatelimitConfig::SetResponseOutThreadCount(uint32_t val) {
    response_out_thread_count_ = val;
}

uint32_t RatelimitConfig::GetResponseOutThreadCount() const {
    return response_out_thread_count_;
}

void RatelimitConfig::SetPrintStat(bool val) {
    print_stat_ = val;
}

bool RatelimitConfig::GetPrintStat() const {
    return print_stat_;
}

NS_END2
