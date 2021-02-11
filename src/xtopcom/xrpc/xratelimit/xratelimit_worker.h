#pragma once
#ifndef RATELIMIT_WORKER_H_
#define RATELIMIT_WORKER_H_

#include <thread>
#include <vector>
#include <mutex>
#include <functional>
#include "xratelimit_config.h"
#include "xratelimit_data_queue.h"
#include "xratelimit_server_stat.h"
#include "xratelimit_dispatch.h"
#include "xratelimit_thread.h"
#include "xbasic/xns_macro.h"


NS_BEG2(top, xChainRPC)

class RatelimitWorkerPool;

class RatelimitWorker {

    using RatelimitThreadPtr = std::shared_ptr<RatelimitThread>;
public:
    RatelimitWorker(const int idx, RatelimitWorkerPool* const pool);
    virtual ~RatelimitWorker();

    void Func();
    virtual void WorkerLogic() = 0;
    void Shutdown();
    void Start();

protected:
    int idx_{ -1 };
    int uthread_count_;
    bool shut_down_{ false };
    // UThreadEpollScheduler* worker_scheduler_{ nullptr };
    RatelimitWorkerPool* pool_{ nullptr };
    RatelimitThreadPtr thread_;
};

class InWorker : public RatelimitWorker {
public:
    InWorker(const int idx, RatelimitWorkerPool* const pool);
    virtual void WorkerLogic();
};

class OutWorker : public RatelimitWorker {
public:
    OutWorker(const int idx, RatelimitWorkerPool* const pool);
    virtual void WorkerLogic();
};


using WorkerFactoryFunc = std::function<RatelimitWorker* (
    const int idx, RatelimitWorkerPool* const pool)>;

class RatelimitWorkerPool final {
public:
    RatelimitWorkerPool(const int thread_count,
        RatelimitDataQueue* const data_queue,
        RatelimitServerStat* const ratelimit_server_stat,
        RatelimitDispatch* const dispatch,
        WorkerFactoryFunc factory);
    ~RatelimitWorkerPool();

private:
    friend class RatelimitWorker;
    friend class InWorker;
    friend class OutWorker;
    int idx_{ -1 };
    RatelimitDataQueue* data_queue_{ nullptr };
    std::vector<RatelimitWorker*> worker_list_;
    RatelimitServerStat* server_stat_{ nullptr };
    RatelimitDispatch* dispatch_{ nullptr };
    WorkerFactoryFunc factory_;
    size_t last_notify_idx_;
    std::mutex mutex_;
};

NS_END2

#endif // !RATELIMIT_WORKER_H_
