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

TEST_F(TestTopUtils, SplitString1) {
    std::string nodes("aaaaa");
    std::set<std::string> node_sets;
    top::SplitString(nodes, ',', node_sets);
    ASSERT_NE(node_sets.find("aaaaa"), node_sets.end());
    ASSERT_EQ(node_sets.find("bbbbb"), node_sets.end());
    ASSERT_EQ(node_sets.size(), 1);
}

TEST_F(TestTopUtils, SplitString2) {
    std::string nodes("aaaaa,bbbbb,ccccc");
    std::set<std::string> node_sets;
    top::SplitString(nodes, ',', node_sets);

    ASSERT_NE(node_sets.find("aaaaa"), node_sets.end());
    ASSERT_NE(node_sets.find("bbbbb"), node_sets.end());
    ASSERT_NE(node_sets.find("ccccc"), node_sets.end());
    ASSERT_EQ(node_sets.find("ddddd"), node_sets.end());
    ASSERT_EQ(node_sets.size(), 3);

}
}  // namespace test

}  // namespace top
