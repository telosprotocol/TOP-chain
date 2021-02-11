#include <gtest/gtest.h>

#include <iostream>
#include <chrono>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_log.h"
#define private public

// change to xbase
#include "xpbase/src/top_timer_xbase.h"
#define TestTimerImpl TestTimerXbase
#define TimerManagerImpl TimerManagerXbase
#define TimerImpl TimerXbase

namespace top {
namespace base {
namespace test {

// test join/stop/dtor
class TestTimerImpl : public testing::Test {
public:
    static void SetUpTestCase() {
        timer_manager_ = std::make_shared<base::TimerManagerImpl>();
        timer_manager_->Start(3);
    }

    static void TearDownTestCase() {
        timer_manager_->Stop();
        timer_manager_ = nullptr;
    }

    void SetUp() {
        timer_manager_impl_ = dynamic_cast<base::TimerManagerImpl*>(timer_manager_.get());
        ASSERT_EQ(0u, timer_manager_impl_->TimerCount());
    }

    void TearDown() {}

private:
    uint32_t called_times_;
    base::TimerManagerImpl* timer_manager_impl_{nullptr};
    static std::shared_ptr<TimerManager> timer_manager_;
};

std::shared_ptr<TimerManager> TestTimerImpl::timer_manager_;

TEST_F(TestTimerImpl, wait_stop) {
    auto f = []{
        TOP_FATAL("wait_stop");
    };
    auto timer = timer_manager_->CreateTimer(10, 5, f, "wait_stop");
    ASSERT_EQ(1u, timer_manager_impl_->TimerCount());

    SleepMs(50);
    timer->Stop();
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());

    // stop again
    SleepMs(10);
    timer->Stop();
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());
}

TEST_F(TestTimerImpl, stop_async) {
    auto f = []{
        TOP_FATAL("stop_async");
    };
    auto timer = timer_manager_->CreateTimer(20, 0, f, "stop_async");
    timer->Stop(false);

    SleepMs(40);
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());
}

TEST_F(TestTimerImpl, IsStopped) {
    auto f = []{
        SleepMs(15);
    };
    auto timer = timer_manager_->CreateTimer(1, 0, f, "IsStopped");
    SleepMs(3);
    timer->Stop(false);
    ASSERT_EQ(1u, timer_manager_impl_->TimerCount());
    while (!timer->IsStopped()) {}
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());
}

TEST_F(TestTimerImpl, IsStopped_2) {
    auto f = []{};
    TimerImpl timer(timer_manager_impl_, 1, 0, f, "IsStopped_2");
    ASSERT_TRUE(timer.IsStopped());
}

TEST_F(TestTimerImpl, Instance) {
    auto p1 = TimerManager::Instance();
    auto p2 = TimerManager::Instance();
    ASSERT_EQ(p1, p2);
}

TEST_F(TestTimerImpl, stop_twice_1) {
    auto f = []{};
    auto timer = std::make_shared<TimerGuard>();
    timer->Start(timer_manager_.get(), 20, 0, f, "stop_twice");
    ASSERT_EQ(1u, timer_manager_impl_->TimerCount());

    SleepMs(50);
    timer = nullptr;
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());
}

TEST_F(TestTimerImpl, stop_twice_2) {
    auto f = []{
        SleepMs(50);
    };
    auto timer = std::make_shared<TimerGuard>();
    timer->Start(timer_manager_.get(), 10, 0, f, "stop_twice");
    SleepMs(20);
    timer = nullptr;
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());

    SleepMs(50);
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());
}

TEST_F(TestTimerImpl, call_once) {
    int n = 0;
    auto f = [&n]{
        n += 1;
    };
    auto timer = std::make_shared<TimerGuard>();
    timer->Start(timer_manager_.get(), 10, 0, f, "call_once");
    SleepMs(20);
    timer = nullptr;
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());

    ASSERT_EQ(1, n);
}

TEST_F(TestTimerImpl, call_repeated) {
    int n = 0;
    auto f = [&n]{
        n += 1;
    };
    auto timer = std::make_shared<TimerGuard>();
    timer->Start(timer_manager_.get(), 10, 10, f, "call_repeated");
    SleepMs(75);
    timer = nullptr;
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());

}

TEST_F(TestTimerImpl, multi_timer) {
    std::mutex mutex_n;
    int n = 0;
    auto f = [&mutex_n, &n]{
        std::unique_lock<std::mutex> lock(mutex_n);
        n += 1;
    };
    auto timer1 = std::make_shared<TimerGuard>();
    timer1->Start(timer_manager_.get(), 10, 0, f, "multi_timer_1");
    auto timer2 = std::make_shared<TimerGuard>();
    timer2->Start(timer_manager_.get(), 10, 0, f, "multi_timer_2");
    auto timer3 = std::make_shared<TimerGuard>();
    timer3->Start(timer_manager_.get(), 10, 0, f, "multi_timer_3");
    ASSERT_EQ(3u, timer_manager_impl_->TimerCount());

    SleepMs(30);
    ASSERT_EQ(3, n);

    timer1 = nullptr;
    timer2 = nullptr;
    timer3 = nullptr;
    ASSERT_EQ(0u, timer_manager_impl_->TimerCount());
}

TEST_F(TestTimerImpl, mock_hard_to_test_branch_1) {
    auto f = []{};
    auto timer_impl = new TimerImpl(timer_manager_impl_, 10, 0, f, "mock_hard_to_test_branch_1");
    timer_impl->started_ = true;
    timer_impl->request_stop_ = true;
    timer_impl->Stop(false);

    // delete timer
    Timer2* timer = timer_impl;
    delete timer;
}

TEST_F(TestTimerImpl, mock_hard_to_test_branch_2) {
    auto timer_manager_impl = new TimerManagerImpl();

    // delete timer manager
    TimerManager* timer_manager = timer_manager_impl;
    delete timer_manager;
}

TEST_F(TestTimerImpl, stuck_30s) {
    auto f = []{};
    TimerGuard timer;
    timer.Start(timer_manager_.get(), 30 * 1000, 0, f, "stuck_using_30s");
}

TEST_F(TestTimerImpl, call_stop_on_timer) {
    TimerGuard timer;
    auto f = [&]{
        timer.timer_->Stop(true);
    };
    timer.Start(timer_manager_.get(), 30, 0, f, "call_stop_on_timer");

    SleepMs(100);
    timer.timer_->Stop(true);
}

}  // namespace test
}  // namespace base
}  // namespace top
