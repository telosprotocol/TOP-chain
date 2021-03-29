#include "gtest/gtest.h"
#include "xrpc/xratelimit/xratelimit_worker.h"
#include <vector>
#include <string>
#include <thread>




using namespace std;
using namespace top::xChainRPC;


class RatelimitWorkerTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {}

    static void TearDownTestCase()
    {}

public:
    static const int thread_count_{ 3 };
    static atomic_int count_;
    static vector<std::thread::id> thread_ids_;
    static mutex mutex_;
};

const int RatelimitWorkerTest::thread_count_;
atomic_int RatelimitWorkerTest::count_{ 0 };
vector<std::thread::id> RatelimitWorkerTest::thread_ids_;
mutex RatelimitWorkerTest::mutex_;

class WorkerMock : public RatelimitWorker {
public:
    WorkerMock(const int idx, RatelimitWorkerPool* const pool)
        : RatelimitWorker(idx, pool)
    {}

    virtual void WorkerLogic() {
        lock_guard<mutex> lock(RatelimitWorkerTest::mutex_);
        RatelimitWorkerTest::count_++;
        auto t = std::this_thread::get_id();
        RatelimitWorkerTest::thread_ids_.push_back(t);
    }
};



TEST_F(RatelimitWorkerTest, TestCreate) {
    RatelimitDataQueue queue_;
    RatelimitConfig config_;
    RatelimitCache cache_(&config_);
    RatelimitServerStat stat_;
    RatelimitDispatch dispatch_(&cache_, &config_, &stat_);
    RatelimitWorkerPool work_pool(thread_count_, &queue_, &stat_, &dispatch_,
        [](const int idx, RatelimitWorkerPool * const pool)->
        RatelimitWorker * {
            auto work = new WorkerMock(idx, pool);
            work->Start();
            return work; });
    usleep(50000);
    EXPECT_EQ(RatelimitWorkerTest::count_, thread_count_);
    auto size = RatelimitWorkerTest::thread_ids_.size();
    EXPECT_EQ(size, thread_count_);
    if (RatelimitWorkerTest::thread_ids_.empty())
        return;
    auto it = RatelimitWorkerTest::thread_ids_.end() - 1;
    for (; it != RatelimitWorkerTest::thread_ids_.begin(); --it) {
        auto value = *it;
        RatelimitWorkerTest::thread_ids_.erase(it);
        auto itFind = find(RatelimitWorkerTest::thread_ids_.begin(),
            RatelimitWorkerTest::thread_ids_.end(), value);
        EXPECT_EQ(itFind, RatelimitWorkerTest::thread_ids_.end());
    }
}
