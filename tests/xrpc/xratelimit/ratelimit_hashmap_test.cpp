#include "gtest/gtest.h"
#include "xrpc/xratelimit/xratelimit_hashmap.h"
#include <vector>
#include <string>



using namespace std;
using namespace top::xChainRPC;



class RatelimitHashmapTest : public testing::Test {
public:
    struct TestContent {
        int nKey_;
        string strKey_;
        int nValue_;
    };

protected:
    static void SetUpTestCase() {
        for (int i{ 0 }; i < count_; ++i) {
            char str[16] = { 0 };
            snprintf(str, 16, "key-%d", i);
            auto content = new TestContent{ i, string{str}, i };
            contents_.push_back(content);
        }
    }

    static void TearDownTestCase() {
        for (auto p : contents_) {
            delete p;
        }
    }

public:
    static const int count_{ 10 };
    static vector<TestContent*> contents_;
    static RatelimitHashmap<int, TestContent*> hashmap_int_;
    static RatelimitHashmap<string, TestContent*> hashmap_str_;
};

const int RatelimitHashmapTest::count_;
vector<RatelimitHashmapTest::TestContent*> RatelimitHashmapTest::contents_;
RatelimitHashmap<int, RatelimitHashmapTest::TestContent*>
RatelimitHashmapTest::hashmap_int_;
RatelimitHashmap<string, RatelimitHashmapTest::TestContent*>
RatelimitHashmapTest::hashmap_str_;

TEST_F(RatelimitHashmapTest, TestInsert) {
    for (int i{ 0 }; i < count_; ++i) {
        auto content = contents_[i];
        hashmap_int_.Insert(content->nKey_, content);
        hashmap_str_.Insert(content->strKey_, content);
    }
    EXPECT_EQ(hashmap_int_.Size(), count_);
    EXPECT_EQ(hashmap_str_.Size(), count_);
    // Test repetitive insertion
    bool br{ true };
    for (int i{ 0 }; i < count_; ++i) {
        auto content = contents_[i];
        br = hashmap_int_.Insert(content->nKey_, content);
        EXPECT_FALSE(br);
        br = hashmap_str_.Insert(content->strKey_, content);
        EXPECT_FALSE(br);
    }
}

TEST_F(RatelimitHashmapTest, TestFind) {
    bool br{ false };
    for (int i{ 0 }; i < count_; ++i) {
        auto content = contents_[i];
        TestContent* value{ nullptr };
        br = hashmap_int_.Find(content->nKey_, value);
        EXPECT_TRUE(br);
        EXPECT_EQ(value, content);
        br = hashmap_str_.Find(content->strKey_, value);
        EXPECT_TRUE(br);
        EXPECT_EQ(value, content);
    }
}

TEST_F(RatelimitHashmapTest, TestForeach) {
    hashmap_int_.Foreach([](int, TestContent * cont) { cont->nValue_++; });
    for (int i{ 0 }; i < count_; ++i) {
        TestContent* value{ nullptr };
        bool br = hashmap_int_.Find(i, value);
        EXPECT_TRUE(br);
        EXPECT_EQ(value->nValue_, i + 1);
    }
}
