#pragma once
#ifndef RATELIMIT_CONFIG_H_
#define RATELIMIT_CONFIG_H_

#include <stdint.h>
#include "xbase/xns_macro.h"


NS_BEG2(top, xChainRPC)

class RatelimitConfig {
public:
    RatelimitConfig();
    ~RatelimitConfig();
    bool Read(const char* config_file);

    void SetMaxThreads(int max_threads);
    int GetMaxThreads() const;

    void SetCheckIp(bool is_check);
    bool GetCheckIp() const;

    void SetCheckAccount(bool is_check);
    bool GetCheckAccount() const;

    void SetCheckAsset(bool is_check);
    bool GetCheckAsset() const;

    void SetRatePerSecond(uint32_t val);
    uint32_t GetRatePerSecond() const;

    void SetBurstSize(uint32_t val);
    uint32_t GetBurstSize() const;

    void SetCacheTimeoutSecond(uint32_t val);
    uint32_t GetCacheTimeoutSecond() const;

    void SetAllowBorrowSize(uint32_t val);
    uint32_t GetAllowBorrowSize() const;

    void SetRequestToken(uint32_t val);
    uint32_t GetRequestToken() const;

    void SetRequestOutThreadCount(uint32_t val);
    uint32_t GetRequestOutThreadCount() const;

    void SetResponseOutThreadCount(uint32_t val);
    uint32_t GetResponseOutThreadCount() const;

    void SetPrintStat(bool val);
    bool GetPrintStat() const;

private:
    friend class RatelimitCache;
    int max_threads_{ 3 };
    bool check_ip_{ true };
    bool check_account_{ true };
    bool check_asset_{ true };
    uint32_t rate_per_second_{ 0 };
    uint32_t burst_size_{ 0 };
    uint32_t cache_timeout_second_{ 0 };
    uint32_t allow_borrow_size_{ 0 };
    uint32_t request_token_{ 0 };
    uint32_t request_out_thread_count_{ 2 };
    uint32_t response_out_thread_count_{ 2 };
    bool print_stat_{ false };
};

NS_END2

#endif  // !RATELIMIT_CONFIG_H_
