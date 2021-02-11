#include <gtest/gtest.h>

#include <iostream>
#include <chrono>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_log.h"
#define private public
#include "xpbase/base/top_timer.h"

namespace top {
namespace base {
namespace test {

class TestTimer : public testing::Test {
public:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
        called_times_ = 0;
        timer_.CallAfter(15 * 1000, std::bind(&TestTimer::TimerCall, this));
    }

    virtual void TearDown() {
        SleepUs(50 * 1000);
    }

    void TimerCall() {
        if (called_times_ < 5) {
            ++called_times_;
        }
    }

private:
    uint32_t called_times_;
    Timer timer_{TimerManager::Instance(), "TestTimer"};
};

TEST_F(TestTimer, All) {
    ASSERT_TRUE(true);
}

class TestTimerRepeated : public testing::Test {
public:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
        called_times_ = 0;
        timer_.Start(
                20 * 1000, 20 * 1000,
                std::bind(&TestTimerRepeated::TimerCall, this));
    }

    virtual void TearDown() {
        SleepUs(90 * 1000);
        timer_.Join();
    }

    void TimerCall() {
        ++called_times_;
        auto tp_now = std::chrono::system_clock::now();
        auto d = tp_now.time_since_epoch();
        auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(d);
        TOP_FATAL("milli: %ld", (long)d2.count());
    }

private:
    uint32_t called_times_;
    base::TimerRepeated timer_{TimerManager::Instance(), "TestTimerRepeated"};
};

TEST_F(TestTimerRepeated, All) {
    ASSERT_TRUE(true);
}

// TODO: using excetion to check?
// TEST_F(TestTimerRepeated, assert_when_start_twice) {
//     auto f = []{};
//     base::TimerRepeated timer(TimerManager::Instance());
//     timer.Start(20 * 1000, 20 * 1000, f);
//     timer.Start(20 * 1000, 20 * 1000, f);
// }

class TestSingleTimer : public testing::Test {
public:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
        called_times_ = 0;
        timer_.CallAfter(15 * 1000, std::bind(&TestSingleTimer::TimerCall, this));
    }

    virtual void TearDown() {
        SleepUs(300 * 1000);
    }

    void TimerCall() {
        ++called_times_;
        if (called_times_ < 5) {
            timer_.CallAfter(15 * 1000, std::bind(&TestSingleTimer::TimerCall, this));
        }
    }

private:
    uint32_t called_times_;
    SingleThreadTimer timer_;
};

TEST_F(TestSingleTimer, All) {
    ASSERT_TRUE(true);
}

}  // namespace test
}  // namespace base
}  // namespace top
