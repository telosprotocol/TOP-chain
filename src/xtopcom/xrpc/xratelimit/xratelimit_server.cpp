#include "xratelimit_server.h"


NS_BEG2(top, xChainRPC)

RatelimitServer::RatelimitServer(const RatelimitConfig& config)
    : config_(&config)
    , server_stat_(config.GetPrintStat())
    , cache_(&config)
    , dispatch_(&cache_, config_, &server_stat_)
    , in_worker_pool_(config_->GetMaxThreads(),
        &data_queue_, &server_stat_, &dispatch_,
        [](const int idx, RatelimitWorkerPool* const pool)->RatelimitWorker* {
            auto work = new InWorker(idx, pool);
            work->Start();
            return work;
        })
    , out_worker_pool_(config_->GetMaxThreads(),
        &data_queue_, &server_stat_, &dispatch_,
        [](const int idx, RatelimitWorkerPool* const pool)->RatelimitWorker* {
            auto work = new OutWorker(idx, pool);
            work->Start();
            return work;
        })
{}

RatelimitServer::~RatelimitServer() {
    BreakOut();
    request_out_pool_.clear();
    response_out_pool_.clear();
}

void RatelimitServer::RequestIn(RatelimitData* request) {
    server_stat_.inqueue_push_requests_++;
    data_queue_.PushRequestIn(request);
}

void RatelimitServer::ResponseIn(RatelimitData* response) {
    server_stat_.inqueue_push_responses_++;
    data_queue_.PushResponseIn(response);
}

RatelimitData* RatelimitServer::RequestOut() {
    RatelimitData* request{ nullptr };
    data_queue_.PluckRequestOut(request);
    return request;
}

RatelimitData* RatelimitServer::ResponseOut() {
    RatelimitData* response{ nullptr };
    data_queue_.PluckResponseOut(response);
    return response;
}

RatelimitServerStat* RatelimitServer::GetStat() {
    return &server_stat_;
}

void RatelimitServer::RegistRequestOut(OutFunc func) {
    if (request_out_pool_.empty()) {
        request_out_func_ = func;

        uint32_t count = config_->GetRequestOutThreadCount();
        for (uint32_t i{ 0 }; i < count; ++i) {
            auto thread = std::make_shared<RatelimitThread>(
                std::bind(&RatelimitServer::RequestOutThreadFunc, this));
            request_out_pool_.push_back(thread);
        }
    }
}

void RatelimitServer::RegistResponseOut(OutFunc func) {
    if (response_out_pool_.empty()) {
        response_out_func_ = func;

        uint32_t count = config_->GetResponseOutThreadCount();
        for (uint32_t i{ 0 }; i < count; ++i) {
            auto thread = std::make_shared<RatelimitThread>(
                std::bind(&RatelimitServer::ResponseOutThreadFunc, this));
            response_out_pool_.push_back(thread);
        }
    }
}

void RatelimitServer::RequestOutThreadFunc() {
    while (!shut_down_) {
        RatelimitData* request{ nullptr };
        data_queue_.PluckRequestOut(request);
        if(!shut_down_){
            server_stat_.outqueue_pop_requests_++;
            request_out_func_(request);
        }
    }
}

void RatelimitServer::ResponseOutThreadFunc() {
    while (!shut_down_) {
        RatelimitData* response{ nullptr };
        data_queue_.PluckResponseOut(response);
        if(!shut_down_){
            server_stat_.outqueue_pop_responses_++;
            response_out_func_(response);
        }
    }
}

void RatelimitServer::BreakOut() {
    shut_down_ = true;
    data_queue_.BreakOut();
}

NS_END2
