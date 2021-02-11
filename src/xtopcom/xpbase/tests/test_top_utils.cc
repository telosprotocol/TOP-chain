//
//  test_top_utils.cc
//  test
//
//  Created by Charlie Xie 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <gtest/gtest.h>

#include <iostream>

#define private public
#include "xpbase/base/top_utils.h"

namespace top {

namespace test {

class TestTopUtils : public testing::Test {
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

TEST_F(TestTopUtils, TrimString) {
    std::string test_str("    Hello world    !   ");
    TrimString(test_str);
    ASSERT_EQ(test_str, "Hello world    !");
}

TEST_F(TestTopUtils, TrimString2) {
    std::string test_str(" Hello world    ! ");
    TrimString(test_str);
    ASSERT_EQ(test_str, "Hello world    !");
}

TEST_F(TestTopUtils, TrimString3) {
    std::string test_str("    Hello world    !");
    TrimString(test_str);
    ASSERT_EQ(test_str, "Hello world    !");
}

TEST_F(TestTopUtils, TrimString4) {
    std::string test_str("Hello world    !");
    TrimString(test_str);
    ASSERT_EQ(test_str, "Hello world    !");
}

TEST_F(TestTopUtils, TrimString5) {
    std::string test_str("Hello world    ! ");
    TrimString(test_str);
    ASSERT_EQ(test_str, "Hello world    !");
}

TEST_F(TestTopUtils, TrimString6) {
    std::string test_str("Hello world    !     ");
    TrimString(test_str);
    ASSERT_EQ(test_str, "Hello world    !");
}

TEST_F(TestTopUtils, GetCurrentTimeMsec) {
    uint64_t now_time = GetCurrentTimeMsec();
    ASSERT_TRUE(now_time > 0);
}

TEST_F(TestTopUtils, IsNum) {
    ASSERT_TRUE(IsNum("123"));
    ASSERT_TRUE(IsNum("123.89"));
    ASSERT_TRUE(IsNum("123462345234523452345234599999"));
    ASSERT_FALSE(IsNum("12346234523sdfasdf4523452345234599999"));
    ASSERT_FALSE(IsNum("123.34sdf"));
    ASSERT_FALSE(IsNum("12334sdf"));
    ASSERT_FALSE(IsNum("asdfas12334sdf"));
    ASSERT_FALSE(IsNum("asdfasd"));
}

TEST_F(TestTopUtils, RandomAscString) {
    const auto str = RandomAscString(10);
    ASSERT_EQ(10, str.size());
}

TEST_F(TestTopUtils, RandomInt32) {
    RandomInt32();
}

TEST_F(TestTopUtils, RandomUint32) {
    RandomUint32();
}

TEST_F(TestTopUtils, RandomUint64) {
    RandomUint64();
}

TEST_F(TestTopUtils, RandomUint16) {
    RandomUint16();
}

TEST_F(TestTopUtils, HexEncode) {
    std::string str;
    str.append(1, (char)0x12);
    str.append(1, (char)0x34);
    auto str2 = HexEncode(str);
    ASSERT_EQ("1234", str2);
}

TEST_F(TestTopUtils, HexDecode) {
    const std::string str("1234");
    auto str2 = HexDecode(str);
    ASSERT_EQ(0x12, str2[0]);
    ASSERT_EQ(0x34, str2[1]);
}

TEST_F(TestTopUtils, Base64Encode) {
    const std::string str("1234");
    auto str2 = Base64Encode(str);
    ASSERT_EQ("MTIzNA==", str2);
}

TEST_F(TestTopUtils, Base64Decode) {
    const std::string str("MTIzNA==");
    auto str2 = Base64Decode(str);
    ASSERT_EQ("1234", str2);
}

TEST_F(TestTopUtils, Base64Substr) {
    const std::string str("1234");
    auto str2 = Base64Substr(str);
    ASSERT_EQ("MTIzNA==", str2);
}

TEST_F(TestTopUtils, HexSubstr) {
    {
        std::string str;
        std::string STR2;
        for (int i = 0; i < 6; ++i) {
            str.append(1, (char)i);
            STR2 += "0" + std::to_string(i);
        }
        const auto str2 = HexSubstr(str);
        ASSERT_EQ(STR2, str2);
    }
    
    {
        std::string str;
        for (int i = 0; i < 7; ++i) {
            str.append(1, (char)i);
        }
        const auto str2 = HexSubstr(str);
        ASSERT_EQ("000102..040506", str2);
    }
}

TEST_F(TestTopUtils, IsBigEnd_IsSmallEnd) {
    const auto both = IsBigEnd() && IsSmallEnd();
    const auto either = IsBigEnd() || IsSmallEnd();
    const auto neither = !IsBigEnd() && !IsSmallEnd();
    ASSERT_FALSE(both);
    ASSERT_TRUE(either);
    ASSERT_FALSE(neither);
}

TEST_F(TestTopUtils, StringHash) {
    const auto str1 = StringHash("1234");
    const auto str2 = StringHash("1234");
    ASSERT_EQ(str1, str2);
}

TEST_F(TestTopUtils, GetGlobalXidWithNodeId) {
    GetGlobalXidWithNodeId("");
}

TEST_F(TestTopUtils, GetStringSha128) {
    const auto str1 = GetStringSha128("1234");
    const auto str2 = GetStringSha128("1234");
    ASSERT_EQ(str1, str2);
}


}  // namespace test

}  // namespace top
