//
//  test_bloomfilter.cc
//  test
//
//  Created by Charlie Xie 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <gtest/gtest.h>

#include <iostream>

#include "xpbase/base/top_utils.h"
#define private public
#include "xpbase/base/uint64_bloomfilter.h"


namespace top {

namespace test {

class TestUint64BloomFilter : public testing::Test {
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

TEST_F(TestUint64BloomFilter, Compare) {
    {
        auto begin_time = GetCurrentTimeMicSec();
        for (uint32_t i = 0; i < 100000; ++i) {
            base::Uint64BloomFilter bloomfilter(4096, 11);
            bloomfilter.Add("hello world");
            bloomfilter.Add("hello world");
            bloomfilter.Add("hello world");
            bloomfilter.Add("hello world");
            bloomfilter.Add("hello world");
            bloomfilter.Add("hello world");
            std::vector<uint64_t> data_vec = bloomfilter.Uint64Vector();
            base::Uint64BloomFilter tmp_bloomfilter{ data_vec, 11 };
        }
        auto use_time = GetCurrentTimeMicSec() - begin_time;
        std::cout << "uint64 use time: " << use_time << std::endl;
    }
}

TEST_F(TestUint64BloomFilter, Add) {
    base::Uint64BloomFilter bloomfilter(4096, 2);
    bloomfilter.Add("hello world");
    bloomfilter.Add("hello world1");
    bloomfilter.Add("hello world12");
    bloomfilter.Add("hello world13");
    bloomfilter.Add("hello world11");
    bloomfilter.Add("hello world133");
    ASSERT_TRUE(bloomfilter.Contain("hello world"));
}

TEST_F(TestUint64BloomFilter, Contain) {
    base::Uint64BloomFilter bloomfilter(4096, 2);
    ASSERT_FALSE(bloomfilter.Contain("hello world131"));
    bloomfilter.Add("hello world");
    bloomfilter.Add("hello world1");
    bloomfilter.Add("hello world12");
    bloomfilter.Add("hello world13");
    bloomfilter.Add("hello world11");
    bloomfilter.Add("hello world133");
    ASSERT_TRUE(bloomfilter.Contain("hello world"));
    ASSERT_TRUE(bloomfilter.Contain("hello world1"));
    ASSERT_TRUE(bloomfilter.Contain("hello world12"));
    ASSERT_TRUE(bloomfilter.Contain("hello world13"));
    ASSERT_TRUE(bloomfilter.Contain("hello world11"));
    ASSERT_TRUE(bloomfilter.Contain("hello world133"));
    ASSERT_FALSE(bloomfilter.Contain("hello world131"));
    ASSERT_FALSE(bloomfilter.Contain("hello world132"));
}

TEST_F(TestUint64BloomFilter, String) {
    base::Uint64BloomFilter bloomfilter(4096, 2);
    std::string data("helloworld");
    for (uint32_t i = 0; i < 256; ++i) {
        data.append(std::to_string(i));
        bloomfilter.Add(data);
    }

    const std::vector<uint64_t>& data_vec = bloomfilter.Uint64Vector();
    base::Uint64BloomFilter tmp_bloomfilter{ data_vec, 2 };
    std::string pre_str = bloomfilter.string();
    std::string now_str = tmp_bloomfilter.string();
    ASSERT_EQ(pre_str, now_str);

    data = "helloworld";
    for (uint32_t i = 0; i < 256; ++i) {
        data.append(std::to_string(i));
        ASSERT_TRUE(tmp_bloomfilter.Contain(data));
    }
}

TEST_F(TestUint64BloomFilter, GetHash) {
    base::Uint64BloomFilter bloomfilter{ 4096, 2 };
    std::string data("hello_world");
    uint32_t hash_high;
    uint32_t hash_low;
    bloomfilter.GetHash(data, hash_high, hash_low);

    uint64_t hash_value = XXH64(data.c_str(), data.size(), bloomfilter.kHashSeed);
    uint32_t high = static_cast<uint32_t>(hash_value >> 32 & 0x00000000FFFFFFFF);
    uint32_t low = static_cast<uint32_t>(hash_value & 0x00000000FFFFFFFF);
    ASSERT_EQ(high, hash_high);
    ASSERT_EQ(low, hash_low);

    uint64_t tmp_high = hash_high;
    uint64_t tmp_low = hash_low;
    tmp_high = tmp_high << 32;
    uint64_t new_val = tmp_high + tmp_low;
    ASSERT_EQ(new_val, hash_value);
}

TEST_F(TestUint64BloomFilter, AddHash) {
    base::Uint64BloomFilter bf(4096, 2);
    const uint64_t data = 0x1122334455667788;
    const auto data2 = data + 1;
    bf.Add(data);
    ASSERT_TRUE(bf.Contain(data));
    ASSERT_FALSE(bf.Contain(data2));
}

TEST_F(TestUint64BloomFilter, HashNum2) {
    base::Uint64BloomFilter bf(4096, 2);
    const std::string data = "data";
    const std::string data2 = "data2";
    bf.Add(data);
    ASSERT_TRUE(bf.Contain(data));
    ASSERT_FALSE(bf.Contain(data2));
}

TEST_F(TestUint64BloomFilter, HashNum3) {
    base::Uint64BloomFilter bf(4096, 3);
    const std::string data = "data";
    const std::string data2 = "data2";
    bf.Add(data);
    ASSERT_TRUE(bf.Contain(data));
    ASSERT_FALSE(bf.Contain(data2));
}

TEST_F(TestUint64BloomFilter, AddIndex) {
    base::Uint64BloomFilter bf(4096, 2);
    const uint32_t data = 123;
    const auto data2 = data + 1;
    bf.Add(data);
    ASSERT_TRUE(bf.Contain(data));
    ASSERT_FALSE(bf.Contain(data2));
}

TEST_F(TestUint64BloomFilter, MergeMore) {
    base::Uint64BloomFilter bf(4096, 2);
    base::Uint64BloomFilter bf2(4096, 2);
    bf.MergeMore(bf2.Uint64Vector());
}

}  // namespace test

}  // namespace top
