#pragma once
#ifndef RATELIMIT_DISPATCH_H_
#define RATELIMIT_DISPATCH_H_


#include "xratelimit_cache.h"
#include "xratelimit_config.h"
#include "xratelimit_data_queue.h"
#include "xratelimit_server_stat.h"
#include <list>
#include <memory>
#include <functional>
#include "xbasic/xns_macro.h"


NS_BEG2(top, xChainRPC)

class RatelimitDispatch final {
public:
    enum class CheckResult {
        Passed = 0,
        Refused,
    };
    RatelimitDispatch(RatelimitCache* const cache,
        const RatelimitConfig* config,
        RatelimitServerStat* const stat);
    ~RatelimitDispatch();

    using DispatchFunc = std::function<CheckResult(RatelimitData* const)>;

    void RegistDispatchReq(DispatchFunc func);
    void RunDispatchReq(RatelimitData* const request,
        RatelimitDataQueue* const queue);
    void RequestRefused(RatelimitData* const request,
        RatelimitDataQueue* const queue);
    void RequestPassed(RatelimitData* const request,
        RatelimitDataQueue* const queue);

    void RegistDispatchResp(DispatchFunc func);
    void RunDispatchResp(RatelimitData* const response,
        RatelimitDataQueue* const queue);
    void ResponsePassed(RatelimitData* const response,
        RatelimitDataQueue* const queue);

    CheckResult CheckIp(RatelimitData* const request);
    CheckResult CheckAccount(RatelimitData* const request);
    CheckResult CheckAssetIn(RatelimitData* const request);
    CheckResult CheckAssetOut(RatelimitData* const response);


private:
    RatelimitCache* cache_{ nullptr };
    const RatelimitConfig* config_{ nullptr };
    RatelimitServerStat* server_stat_{ nullptr };
    std::list<DispatchFunc> in_func_list_;
    std::list<DispatchFunc> out_func_list_;
};

NS_END2

#endif  // !RATELIMIT_DISPATCH_H_
