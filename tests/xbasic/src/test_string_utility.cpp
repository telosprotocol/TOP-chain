// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xbasic/xstring_utility.h"

#include <gtest/gtest.h>

#include <cstring>

TEST(split, split_1_delimiter_mid) {
    std::string const input{"abcdefg:hy"};
    auto const r = top::split(input, ':');
    ASSERT_EQ(2, r.size());
    ASSERT_EQ(7, r[0].size());
    ASSERT_EQ(2, r[1].size());
    ASSERT_EQ(0, std::memcmp(input.data(), r[0].data(), r[0].size()));
    ASSERT_EQ(0, std::memcmp(input.data() + 8, r[1].data(), r[1].size()));
}

TEST(split, split_1_delimiter_end) {
    std::string const input{"abcdefg:"};
    auto const r = top::split(input, ':');
    ASSERT_EQ(2, r.size());
    ASSERT_EQ(7, r[0].size());
    ASSERT_EQ(0, std::memcmp(input.data(), r[0].data(), r[0].size()));
}

TEST(split, split_1_delimiter_begin) {
    std::string const input{":abcdefg"};
    auto const r = top::split(input, ':');
    ASSERT_EQ(2, r.size());
    ASSERT_EQ(7, r[1].size());
    ASSERT_EQ(0, std::memcmp(input.data() + 1, r[1].data(), r[1].size()));
}

TEST(split, split_2_delimiter_mid1) {
    std::string const input{"a:bcdef:g"};
    auto const r = top::split(input, ':');
    ASSERT_EQ(3, r.size());
    ASSERT_EQ(1, r[0].size());
    ASSERT_EQ(5, r[1].size());
    ASSERT_EQ(1, r[2].size());

    ASSERT_EQ(0, std::memcmp(input.data(), r[0].data(), r[0].size()));
    ASSERT_EQ(0, std::memcmp(input.data() + 2, r[1].data(), r[1].size()));
    ASSERT_EQ(0, std::memcmp(input.data() + 8, r[2].data(), r[2].size()));
}

TEST(split, split_2_delimiter_mid2) {
    std::string const input{"abcd::efg"};
    auto const r = top::split(input, ':');
    ASSERT_EQ(3, r.size());
    ASSERT_EQ(4, r[0].size());
    ASSERT_EQ(0, r[1].size());
    ASSERT_EQ(3, r[2].size());

    ASSERT_EQ(0, std::memcmp(input.data(), r[0].data(), r[0].size()));
    ASSERT_EQ(0, std::memcmp(input.data() + 6, r[2].data(), r[2].size()));
}

TEST(split, split_2_delimiter_begin_end) {
    std::string const input{":abcdefg:"};
    auto const r = top::split(input, ':');
    ASSERT_EQ(3, r.size());
    ASSERT_EQ(0, r[0].size());
    ASSERT_EQ(7, r[1].size());
    ASSERT_EQ(0, r[2].size());
    ASSERT_EQ(0, std::memcmp(input.data() + 1, r[1].data(), r[1].size()));
}

TEST(split, split_none_delimiter) {
    std::string const input{"abcdefg"};
    auto const r = top::split(input, ':');
    ASSERT_EQ(1, r.size());
    ASSERT_EQ(0, std::memcmp(input.data(), r[0].data(), r[0].size()));
}
