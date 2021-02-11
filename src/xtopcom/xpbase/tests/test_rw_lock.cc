#include <gtest/gtest.h>

#include <iostream>
#include <chrono>
#include <memory>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_log.h"
#define private public
#include "xpbase/base/top_timer.h"
#include "xpbase/base/rw_lock.h"

namespace top {

namespace base {

namespace test {

class TestClass {
public:
    TestClass() : num_(0) {}
    void Add() {
        ++num_;
    }

private:
    int num_;
};

class TestRWLock : public testing::Test {
public:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
        // TimerManager::Instance()->Start(1);  // TODO:
        test_case_ptr_.reset(new TestClass());
        timer_.Start(15 * 1000, 15 * 1000, std::bind(&TestRWLock::TimerCall, this));
        // TOP_FATAL("0000");
    }

    virtual void TearDown() {}

    void TimerCall() {
        // TOP_FATAL("3333");
        WriteLock w_lock(rw_lock_);
        test_case_ptr_ = nullptr;
        SleepUs(1000);
        test_case_ptr_.reset(new TestClass());
        // TOP_FATAL("4444");
    }

    TimerRepeated timer_{TimerManager::Instance(), "TestRWLock"};
    ReadWriteLock rw_lock_;
    std::shared_ptr<TestClass> test_case_ptr_;
};

TEST_F(TestRWLock, All) {
    for (int i = 0; i < 10000000; ++i) {
        ReadLock r_lock(rw_lock_);
        test_case_ptr_->Add();
    }
    // TOP_FATAL("1111");
    SleepUs(160 * 1000);
    // TOP_FATAL("2222");
    ASSERT_TRUE(true);
}

}  // namespace test

}  // namespace base

}  // namespace top
