#include <gtest/gtest.h>

#include <thread>
#include <vector>
#include <memory>
#include "xpbase/base/top_log.h"
#define private public
#include "xpbase/base/sem.h"

namespace top {
namespace base {
namespace test {

class TestSem : public testing::Test {
public:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    void TimerCall() {
    }

    void ThreadProcPost() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        TOP_INFO("post sem");
        sem_.Post();
    }

private:
    Sem sem_;
};

TEST_F(TestSem, pend) {
    sem_.count_ = 1;
    TOP_INFO("pending sem ...");
    sem_.Pend();
    TOP_INFO("pend sem");
    ASSERT_EQ(0, sem_.count_);

    std::thread th(&TestSem::ThreadProcPost, this);
    TOP_INFO("pending sem ...");
    sem_.Pend();
    TOP_INFO("pend sem");
    ASSERT_EQ(0, sem_.count_);
    th.join();
}

TEST_F(TestSem, pend_for) {
    ASSERT_EQ(0, sem_.count_);
    TOP_INFO("pending for sem ...");
    ASSERT_FALSE(sem_.PendFor(100));
    TOP_INFO("pend sem");
    ASSERT_EQ(0, sem_.count_);
    sem_.Post();
    TOP_INFO("pending for sem ...");
    ASSERT_TRUE(sem_.PendFor(100));
    TOP_INFO("pend sem");
    ASSERT_EQ(0, sem_.count_);

    // TOP_INFO("pending sem ...");
    // sem.Pend();  // block for ever!
}

TEST_F(TestSem, init_with_negative) {
    sem_.count_ = -1;
    sem_.Post();
    ASSERT_EQ(0, sem_.count_);
    sem_.Post();
    ASSERT_EQ(1, sem_.count_);
    TOP_INFO("pending sem ...");
    sem_.Pend();
    TOP_INFO("pend sem");
    ASSERT_EQ(0, sem_.count_);

    // TOP_INFO("pending sem ...");
    // sem.Pend();  // block for ever!
}

TEST_F(TestSem, All) {
    ASSERT_EQ(0, sem_.count_);
    const int N = 3;
    std::vector<std::shared_ptr<std::thread>> vec;
    for (int i = 0; i < N; ++i) {
        auto th = std::make_shared<std::thread>(
            [this] { sem_.Pend(); });
        vec.push_back(th);
    }

    for (int i = 0; i < N; ++i) {
        sem_.Post();
    }

    for (auto& th : vec) {
        th->join();
    }

    ASSERT_EQ(0, sem_.count_);
}

}  // namespace test
}  // namespace base
}  // namespace top
