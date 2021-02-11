//
//  test_exception.cc
//  test
//
//  Created by Charlie Xie 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <gtest/gtest.h>

#include <iostream>

#include "xpbase/base/exception.h"

namespace top {

namespace test {

class TestException : public testing::Test {
public:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

TEST_F(TestException, All) {
    CheckCastException except;
    ASSERT_EQ(except.what(), std::string("ERROR: check cast failed!"));
    CheckCastException except1("test error!");
    ASSERT_EQ(except1.what(), std::string("test error!"));
}

}  // namespace test

}  // namespace top
