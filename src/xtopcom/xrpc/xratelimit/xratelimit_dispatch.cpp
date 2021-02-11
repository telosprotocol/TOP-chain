#include "xratelimit_dispatch.h"
#include <iostream>



NS_BEG2(top, xChainRPC)

RatelimitDispatch::RatelimitDispatch(RatelimitCache* const cache,
    const RatelimitConfig* config,
    RatelimitServerStat* const stat)
    : cache_(cache)
    , config_(config)
    , server_stat_(stat) {
    if (config_->GetCheckIp()) {
        RegistDispatchReq(std::bind(&RatelimitDispatch::CheckIp,
            this, std::placeholders::_1));
    }
    if (config_->GetCheckAccount()) {
        RegistDispatchReq(std::bind(&RatelimitDispatch::CheckAccount,
            this, std::placeholders::_1));
    }
    if (config_->GetCheckAsset()) {
        RegistDispatchReq(std::bind(&RatelimitDispatch::CheckAssetIn,
            this, std::placeholders::_1));
        RegistDispatchResp(std::bind(&RatelimitDispatch::CheckAssetOut,
            this, std::placeholders::_1));
    }
}

RatelimitDispatch::~RatelimitDispatch()
{}

void RatelimitDispatch::RegistDispatchReq(DispatchFunc func) {
    in_func_list_.push_back(func);
}

void RatelimitDispatch::RunDispatchReq(RatelimitData* const request,
    RatelimitDataQueue* const queue) {
    server_stat_->inqueue_pop_requests_++;
    for (auto dispatch : in_func_list_) {
        if (dispatch(request) != CheckResult::Passed) {
            RequestRefused(request, queue);
            return;
        }
    }
    RequestPassed(request, queue);
}

void RatelimitDispatch::RequestRefused(RatelimitData* const request,
    RatelimitDataQueue* const queue) {
    server_stat_->outqueue_push_responses_++;
    request->err_ = static_cast<int>(CheckResult::Refused);
    queue->PushResponseOut(request);
}

void RatelimitDispatch::RequestPassed(RatelimitData* const request,
    RatelimitDataQueue* const queue) {
    server_stat_->outqueue_push_requests_++;
    queue->PushRequestOut(request);
}

void RatelimitDispatch::RegistDispatchResp(DispatchFunc func) {
    out_func_list_.push_back(func);
}

void RatelimitDispatch::RunDispatchResp(RatelimitData* const response,
    RatelimitDataQueue* const queue) {
    server_stat_->inqueue_pop_responses_++;
    for (auto dispatch : out_func_list_) {
        dispatch(response);
    }
    ResponsePassed(response, queue);
}

void RatelimitDispatch::ResponsePassed(RatelimitData* const request,
    RatelimitDataQueue* const queue) {
    server_stat_->outqueue_push_responses_++;
    queue->PushResponseOut(request);
}

RatelimitDispatch::CheckResult
RatelimitDispatch::CheckIp(RatelimitData* const request) {
    return (cache_->CheckIpRelease(request->ip_)) ?
        CheckResult::Passed : CheckResult::Refused;
}

RatelimitDispatch::CheckResult
RatelimitDispatch::CheckAccount(RatelimitData* const request) {
    return (cache_->CheckAccountRelease(request->account_)) ?
        CheckResult::Passed : CheckResult::Refused;
}

RatelimitDispatch::CheckResult
RatelimitDispatch::CheckAssetIn(RatelimitData* const request) {
    return (cache_->CheckAssetRelease(request->account_)) ?
        CheckResult::Passed : CheckResult::Refused;
}

RatelimitDispatch::CheckResult
RatelimitDispatch::CheckAssetOut(RatelimitData* const response) {
    return CheckResult::Passed;
}

NS_END2
