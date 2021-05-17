#pragma once
#ifndef RATELIMIT_CACHE_H_
#define RATELIMIT_CACHE_H_


#include <stdint.h>
#include <thread>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>
#include <condition_variable>
#include <string>

#include "xratelimit_hashmap.h"
#include "xratelimit_thread.h"
#include "xratelimit_config.h"
#include "xbase/xns_macro.h"
#include "xbase/xutl.h"

namespace top {
namespace xChainRPC {
class RatelimitConfig;
}  // namespace xChainRPC
}  // namespace top



NS_BEG2(top, xChainRPC)

class RateLimitCheck {
public:
    enum class State {
        Normal,
        Refused,
    } state_{ State::Normal };
    bool Consume() { return true; }
    bool Asset() { return true; }
    uint64_t steady_time_{ 0 };
    uint64_t refused_time_{ 0 };
};

class RateLimitCheckUseBucket : public RateLimitCheck {
public:
    RateLimitCheckUseBucket(const uint32_t rate_per_second,
        const uint32_t burst_size, const int64_t init_timenow_ms)
        : bucket_(rate_per_second, burst_size, init_timenow_ms)
    {}

    bool Consume();
    void SetAllowBorrowSize(uint32_t val) { allow_borrow_size_ = val; }
    void SetRequestToken(uint32_t val) { request_token_ = val; }
    void ResetBucket(uint32_t rate_per_second, uint32_t burst_size,
        int64_t timenow_ms, int32_t allow_borrow_size);
private:
    top::base::xtokenbucket_t bucket_;
    // Jutokenbucket_t bucket_;
    uint32_t allow_borrow_size_{ 0 };
    uint32_t request_token_{ 1 };
};

class IpCheckContent : public RateLimitCheckUseBucket {
public:
    IpCheckContent(const uint32_t rate_per_second,
        const uint32_t burst_size, const int64_t init_timenow_ms)
        : RateLimitCheckUseBucket(rate_per_second, burst_size, init_timenow_ms)
    {}
};

class AccountCheckContent : public RateLimitCheckUseBucket {
public:
    AccountCheckContent(const uint32_t rate_per_second,
        const uint32_t burst_size, const int64_t init_timenow_ms)
        : RateLimitCheckUseBucket(rate_per_second, burst_size, init_timenow_ms)
    {}
};

class RatelimitCache {
public:
    explicit RatelimitCache(const RatelimitConfig* config);
    virtual ~RatelimitCache();

    using IpHashMap = RatelimitHashmap<uint32_t, std::shared_ptr<IpCheckContent>>;
    using AccountHashMap = RatelimitHashmap<std::string,
        std::shared_ptr<AccountCheckContent>>;

    bool CheckIpRelease(uint32_t ip);
    bool CheckAccountRelease(const std::string& account);
    bool CheckAssetRelease(const std::string& account);

    std::shared_ptr<IpCheckContent> GetIpContent(uint32_t ip);
    std::shared_ptr<AccountCheckContent> GetAccountContent(
        const std::string& account);

    void ThreadFunc();
    void CleanIdle();

    void ResetIpCheckBucket(uint32_t ip);
    void ResetAccountCheckBucket(const std::string& account);

private:
    const RatelimitConfig* config_{ nullptr };
    IpHashMap ip_hashmap_;
    AccountHashMap account_hashmap_;
    bool break_out_{ false };
    std::mutex mutex_;
    std::condition_variable cv_;
    int64_t second_interval_{ 1 };
    uint64_t clean_overtime_s_{ 600 };
    RatelimitThread thread_;
};

NS_END2

#endif  // !RATELIMIT_CACHE_H_
