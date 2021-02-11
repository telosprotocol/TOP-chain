//
//  test_check_cast.cc
//  test
//
//  Created by Charlie Xie 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <stdlib.h>
#include <math.h>

#include <iostream>

#include <gtest/gtest.h>

#ifndef __WORDSIZE
# define __WORDSIZE 32
#endif
#include "xpbase/base/check_cast.h"

namespace top {

namespace base {

namespace test {

class TestCheckCast : public testing::Test {
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

TEST_F(TestCheckCast, LongCastError) {
    const char* value = NULL;
    try {
        ASSERT_FALSE(check_cast<long>(value));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    std::string min_str = check_cast<std::string>(std::numeric_limits<long>::min());
    min_str += min_str;
    try {
        ASSERT_FALSE(check_cast<long>(min_str.c_str()));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, LongLongCastError) {
    const char* value = NULL;
    try {
        ASSERT_FALSE(check_cast<long long>(value));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    std::string min_str = check_cast<std::string>(std::numeric_limits<long>::min());
    min_str += min_str;
    try {
        ASSERT_FALSE(check_cast<long long>(min_str.c_str()));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, UnsignedLongCastError) {
    const char* value = NULL;
    try {
        ASSERT_FALSE(check_cast<unsigned long>(value));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    std::string min_str = check_cast<std::string>(std::numeric_limits<long>::min());
    min_str += min_str;
    try {
        ASSERT_FALSE(check_cast<unsigned long>(min_str.c_str()));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, FloatCastError) {
    const char* value = NULL;
    try {
        ASSERT_FALSE(check_cast<float>(value));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    std::string min_str = check_cast<std::string>(std::numeric_limits<long>::min());
    min_str += min_str;
    try {
        ASSERT_FALSE(check_cast<float>(min_str.c_str()));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, DoubleCastError) {
    const char* value = NULL;
    try {
        ASSERT_FALSE(check_cast<double>(value));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    std::string min_str = check_cast<std::string>(std::numeric_limits<long>::min());
    min_str += min_str;
    try {
        ASSERT_FALSE(check_cast<double>(min_str.c_str()));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, LongDoubleCastError) {
    const char* value = NULL;
    try {
        ASSERT_FALSE(check_cast<long double>(value));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    std::string min_str = check_cast<std::string>(std::numeric_limits<long>::min());
    min_str += min_str;
    try {
        ASSERT_FALSE(check_cast<long double>(min_str.c_str()));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, UnsignedLongLongCastError) {
    const char* value = NULL;
    try {
        ASSERT_FALSE(check_cast<unsigned long long>(value));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    try {
        ASSERT_FALSE(check_cast<unsigned long long>(check_cast<std::string>(
                std::numeric_limits<long>::min()).c_str()));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, BoolCase) {
    bool value = true;
    ASSERT_EQ(check_cast<std::string>(value), "1");
    value = false;
    ASSERT_EQ(check_cast<std::string>(value), "0");
    const char* svalue = "1";
    ASSERT_TRUE(check_cast<bool>(svalue));
    svalue = "0";
    ASSERT_FALSE(check_cast<bool>(svalue));
    svalue = "true";
    try {
        ASSERT_FALSE(check_cast<bool>(svalue));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    svalue = "false";
    try {
        ASSERT_FALSE(check_cast<bool>(svalue));
        ASSERT_FALSE(true);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, StringToNum) {
    const char* value = "1234";
    ASSERT_EQ(check_cast<short>(value), 1234);
    ASSERT_EQ(check_cast<unsigned short>(value), 1234);
    ASSERT_EQ(check_cast<int16_t>(value), 1234);
    ASSERT_EQ(check_cast<uint16_t>(value), 1234);
    ASSERT_EQ(check_cast<int>(value), 1234);
    ASSERT_EQ(check_cast<unsigned int>(value), 1234);
    ASSERT_EQ(check_cast<int32_t>(value), 1234);
    ASSERT_EQ(check_cast<uint32_t>(value), 1234);
    ASSERT_EQ(check_cast<long>(value), 1234);
    ASSERT_EQ(check_cast<unsigned long>(value), 1234);
    ASSERT_EQ(check_cast<long long>(value), 1234);
    ASSERT_EQ(check_cast<unsigned long long>(value), 1234);
    ASSERT_EQ(check_cast<int64_t>(value), 1234);
    ASSERT_EQ(check_cast<uint64_t>(value), 1234);
    ASSERT_EQ(check_cast<float>(value), 1234);
    ASSERT_EQ(check_cast<double>(value), 1234);
}

TEST_F(TestCheckCast, StringToNum6) {
    char value[5];
    memcpy(value, "1234", 4);
    value[4] = '\0';
    ASSERT_EQ(check_cast<short>(value), 1234);
    ASSERT_EQ(check_cast<unsigned short>(value), 1234);
    ASSERT_EQ(check_cast<int16_t>(value), 1234);
    ASSERT_EQ(check_cast<uint16_t>(value), 1234);
    ASSERT_EQ(check_cast<int>(value), 1234);
    ASSERT_EQ(check_cast<unsigned int>(value), 1234);
    ASSERT_EQ(check_cast<int32_t>(value), 1234);
    ASSERT_EQ(check_cast<uint32_t>(value), 1234);
    ASSERT_EQ(check_cast<long>(value), 1234);
    ASSERT_EQ(check_cast<unsigned long>(value), 1234);
    ASSERT_EQ(check_cast<long long>(value), 1234);
    ASSERT_EQ(check_cast<unsigned long long>(value), 1234);
    ASSERT_EQ(check_cast<int64_t>(value), 1234);
    ASSERT_EQ(check_cast<uint64_t>(value), 1234);
    ASSERT_EQ(check_cast<float>(value), 1234);
    ASSERT_EQ(check_cast<double>(value), 1234);
}

TEST_F(TestCheckCast, NumToString) {
    {
        short value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        signed short value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        unsigned short value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        int16_t value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        uint16_t value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        int value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        signed int value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        unsigned int value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        int32_t value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        uint32_t value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        long value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        signed long value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        unsigned long value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        long long value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        signed long long value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        unsigned long long value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        int64_t value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        uint64_t value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        float value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
    {
        double value = 1234;
        ASSERT_EQ(check_cast<std::string>(value), "1234");
    }
}

TEST_F(TestCheckCast, InvalidStringToNum) {
    const char* value = "1234999999999999999999999999999999999999999999999999999";
    try {
        ASSERT_EQ(check_cast<short>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    try {
        ASSERT_EQ(check_cast<unsigned short>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    try {
        ASSERT_EQ(check_cast<int16_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<uint16_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<int>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
   
    try {
        ASSERT_EQ(check_cast<unsigned int>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    try {
        ASSERT_EQ(check_cast<int32_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<uint32_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<unsigned long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<long long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<unsigned long long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<int64_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<uint64_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<float>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, EmptyTest) {
    const char* value = "";
    try {
        ASSERT_EQ(check_cast<short>(value), 0);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    value = "\0";
    try {
        ASSERT_EQ(check_cast<short>(value), 0);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, InvalidStringToNum2) {
    const char* value = "1234fger";
    try {
        ASSERT_EQ(check_cast<short>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<unsigned short>(value), 1234);
        ASSERT_TRUE(false);
    }
    catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<int16_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<uint16_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<int>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
   
    try {
        ASSERT_EQ(check_cast<unsigned int>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    try {
        ASSERT_EQ(check_cast<int32_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<uint32_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<unsigned long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<long long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<unsigned long long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<int64_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<uint64_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<float>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<double>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, InvalidStringToNum4) {
    const char* value = "1234.45ddfd";
    try {
        ASSERT_TRUE(fabs(check_cast<float>(value) - 1234.45) <=
            std::numeric_limits<float>::epsilon());
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    try {
        ASSERT_TRUE(fabs(check_cast<double>(value) - 1234.45) <=
            std::numeric_limits<double>::epsilon());
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

TEST_F(TestCheckCast, InvalidStringToNum3) {
    const char* value = "1234.45";
    ASSERT_TRUE(fabs(check_cast<float>(value) - 1234.45f) <=
        std::numeric_limits<float>::epsilon());
    ASSERT_TRUE(fabs(check_cast<double>(value) - 1234.45) <=
        std::numeric_limits<double>::epsilon());
    try {
        ASSERT_EQ(check_cast<short>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<unsigned short>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<int16_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<uint16_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<int>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
   
    try {
        ASSERT_EQ(check_cast<unsigned int>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }

    try {
        ASSERT_EQ(check_cast<int32_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<uint32_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<unsigned long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<long long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<unsigned long long>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<int64_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
    try {
        ASSERT_EQ(check_cast<uint64_t>(value), 1234);
        ASSERT_TRUE(false);
    } catch (CheckCastException& ex) {
        ASSERT_TRUE(true);
    }
}

}  // namespace test

}  // namespace kadmlia

}  // namespace top
