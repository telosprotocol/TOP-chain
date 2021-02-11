#include "gtest/gtest.h"
#include "xrpc/xratelimit/xratelimit_cache.h"
#include "xrpc/xratelimit/xratelimit_config.h"
#include "xbase/xutl.h"
#include <thread>
#include <mutex>
#include <memory>
#include <chrono>


using namespace std;
using namespace top::xChainRPC;


TEST(RatelimitCacheCase, TestTokenBucket) {
    uint32_t rate_per_second{ 10 * 1000 };
    uint32_t burst_size{ 12 * 1000 };

    auto now_time = std::chrono::steady_clock::now();
    int64_t init_timenow_ms = (std::chrono::duration_cast<
        std::chrono::milliseconds>(now_time.time_since_epoch())).count();
    top::base::xtokenbucket_t bucket(rate_per_second,
        burst_size, init_timenow_ms);

    int32_t ret{ 0 };
    int32_t request_token{ 2 * 1000 };
    for (uint i{ 0 }; i < rate_per_second / request_token; ++i) {
        // auto now_time = std::chrono::steady_clock::now();
        // int64_t timenow_ms = (std::chrono::duration_cast<std::chrono::milliseconds>(now_time.time_since_epoch())).count();
        int64_t timenow_ms = init_timenow_ms;
        ret = bucket.consume(request_token, timenow_ms, 0);
        EXPECT_EQ(ret, request_token);
    }

    int64_t timenow_ms = init_timenow_ms + 1500;
    init_timenow_ms = timenow_ms;
    ret = bucket.consume(burst_size, timenow_ms, 0);    // create 12 tokens
    EXPECT_EQ(ret, burst_size);
    ret = bucket.consume(1, timenow_ms, 0);
    EXPECT_EQ(ret, -1);     // no enought token
}

TEST(RatelimitCacheCase, TestCache) {
    uint32_t rate_per_second{ 10 * 1000 };
    uint32_t burst_size{ 12 * 1000 };
    uint32_t allow_borrow_size{ 0 };
    uint32_t request_token{ 1 * 1000 };
    RatelimitConfig config;
    config.SetRatePerSecond(rate_per_second);
    config.SetBurstSize(burst_size);
    config.SetAllowBorrowSize(allow_borrow_size);
    config.SetRequestToken(request_token);
    RatelimitCache cache(&config);

    bool br{ false };
    uint32_t ip{ 100 };
    for (uint i{ 0 }; i < rate_per_second / request_token; ++i) {
        br = cache.CheckIpRelease(ip);
        EXPECT_TRUE(br);
    }
    br = cache.CheckIpRelease(ip);
    EXPECT_FALSE(br);   // bucket empty

    // reset bucket, at last change one param
    rate_per_second = 5 * 1000;
    burst_size = 6 * 1000;
    allow_borrow_size = 1 * 1000;
    request_token = 1 * 1000;
    config.SetRatePerSecond(rate_per_second);
    config.SetBurstSize(burst_size);
    config.SetAllowBorrowSize(allow_borrow_size);
    config.SetRequestToken(request_token);
    cache.ResetIpCheckBucket(ip);

    for (uint i{ 0 }; i < rate_per_second / request_token; ++i) {
        br = cache.CheckIpRelease(ip);
        EXPECT_TRUE(br);
    }
    br = cache.CheckIpRelease(ip);
    EXPECT_TRUE(br);    // borrow 1
    br = cache.CheckIpRelease(ip);
    EXPECT_FALSE(br);   // bucket empty

    sleep(1);
    for (uint i{ 0 }; i < rate_per_second / request_token; ++i) {
        br = cache.CheckIpRelease(ip);
        EXPECT_TRUE(br);
    }
}
