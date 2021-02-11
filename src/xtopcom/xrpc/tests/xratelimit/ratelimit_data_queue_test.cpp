#include "gtest/gtest.h"
#include "xrpc/xratelimit/xratelimit_data_queue.h"
#include <vector>
#include <string>
#include <thread>


using namespace std;
using namespace top::xChainRPC;


class RatelimitDataQueueTest : public testing::Test {
protected:
    static void SetUpTestCase()
    {}

    static void TearDownTestCase()
    {}

    static void ThreadFunc() {
        // unique_lock<mutex> lock(mutex_);
        // cv_.wait(lock);

        for (int i{ 0 }; i < count_; ++i) {
            RatelimitData* data = new RatelimitData();
            queue_.PushRequestIn(data);
        }
    }

    static void BreakoutThreadFunc() {
        usleep(1000);
        queue_.BreakOut();
    }

    static void Start() {
        cv_.notify_all();
    }

    static RatelimitDataQueue queue_;
    static mutex mutex_;
    static condition_variable cv_;
    static const int count_{ 10 };
};

RatelimitDataQueue RatelimitDataQueueTest::queue_;
const int RatelimitDataQueueTest::count_;
condition_variable RatelimitDataQueueTest::cv_;

TEST_F(RatelimitDataQueueTest, TestSingleThread) {
    bool br{ false };
    RatelimitDataQueue queue;
    RatelimitData* data = new RatelimitData();
    EXPECT_NE(data, nullptr);
    queue.PushRequestIn(data);
    RatelimitData* ret{ nullptr };
    br = queue.PluckRequest(ret);
    EXPECT_TRUE(br);
    EXPECT_EQ(data, ret);

    queue.PushResponseIn(data);
    ret = nullptr;
    br = queue.PluckResponse(ret);
    EXPECT_TRUE(br);
    EXPECT_EQ(data, ret);
    queue.BreakOut();

    delete data;
}

TEST_F(RatelimitDataQueueTest, TestMultiThread) {
    const int thread_count_{ 3 };
    vector<shared_ptr<thread>> threads_;
    for (int i{ 0 }; i < thread_count_; ++i) {
        threads_.push_back(make_shared<thread>(ThreadFunc));
    }

    usleep(1000);
    bool br{ false };
    for (int i{ 0 }; i < count_ * thread_count_; ++i) {
        RatelimitData* ret{ nullptr };
        br = queue_.PluckRequest(ret);
        EXPECT_TRUE(br);
        EXPECT_NE(ret, nullptr);
        delete ret;
    }
    queue_.BreakOut();
    EXPECT_EQ(queue_.RequestInCount(), 0);

    for (auto t : threads_) {
        t->join();
    }
}

TEST_F(RatelimitDataQueueTest, TestBreakout) {
    thread thread_(BreakoutThreadFunc);

    bool br{ false };
    RatelimitData* ret{ nullptr };
    br = queue_.PluckResponse(ret);
    EXPECT_FALSE(br);
    EXPECT_EQ(ret, nullptr);
    thread_.join();
}
