#include "xratelimit_worker.h"
#include <iostream>
#include <assert.h>


NS_BEG2(top, xChainRPC)

RatelimitWorker::RatelimitWorker(const int idx, RatelimitWorkerPool* const pool)
    : idx_(idx)
    , pool_(pool)
//    , thread_(std::bind(&RatelimitWorker::Func, this))
{}

RatelimitWorker::~RatelimitWorker()
{}

void RatelimitWorker::Func() {  // don't do anything else.
    
    WorkerLogic();
}

void RatelimitWorker::Shutdown() {
    shut_down_ = true;
}

void RatelimitWorker::Start() {
    thread_ = std::make_shared<RatelimitThread>(
        std::bind(&RatelimitWorker::Func, this));
}

InWorker::InWorker(const int idx, RatelimitWorkerPool* const pool)
    : RatelimitWorker(idx, pool)
{}

void InWorker::WorkerLogic() {
    while (!shut_down_) {
        pool_->server_stat_->request_worker_idles_++;
        RatelimitData* req{ nullptr };
        shut_down_ = !pool_->data_queue_->PluckRequest(req);
        pool_->server_stat_->request_worker_idles_--;
        if (req != nullptr) {
            pool_->dispatch_->RunDispatchReq(req, pool_->data_queue_);
        }
    }
    std::cout << "InWorker::WorkerLogic() Exit:" << idx_ << std::endl;
}


OutWorker::OutWorker(const int idx, RatelimitWorkerPool* const pool)
    : RatelimitWorker(idx, pool)
{}

void OutWorker::WorkerLogic() {
    while (!shut_down_) {
        pool_->server_stat_->response_worker_idles_++;
        RatelimitData* resp{ nullptr };
        shut_down_ = !pool_->data_queue_->PluckResponse(resp);
        pool_->server_stat_->response_worker_idles_--;
        if (resp != nullptr) {
            pool_->dispatch_->RunDispatchResp(resp, pool_->data_queue_);
        }
    }
    std::cout << "OutWorker::WorkerLogic() Exit:" << idx_ << std::endl;
}


RatelimitWorkerPool::RatelimitWorkerPool(const int thread_count,
    RatelimitDataQueue* const data_queue,
    RatelimitServerStat* const ratelimit_server_stat,
    RatelimitDispatch* const dispatch,
    WorkerFactoryFunc factory)
    : data_queue_(data_queue)
    , server_stat_(ratelimit_server_stat)
    , dispatch_(dispatch)
    , factory_(factory) {
    for (int i{ 0 }; i < thread_count; ++i) {
        auto worker(factory_(i, this));
        assert(worker != nullptr);
        worker_list_.push_back(worker);
    }
}

RatelimitWorkerPool::~RatelimitWorkerPool() {
    for (auto& worker : worker_list_) {
        worker->Shutdown();
        delete worker;
    }
    worker_list_.clear();
}

NS_END2
