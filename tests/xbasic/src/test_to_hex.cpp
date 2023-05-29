// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xhex.h"

#include <gtest/gtest.h>

NS_BEG2(top, tests)

TEST(to_hex, zero) {
    EXPECT_EQ(top::to_hex(0), "0");
    EXPECT_EQ(top::to_hex(0, "0x"), "0x0");
    EXPECT_EQ(top::to_hex(0, "0X"), "0X0");
}

TEST(to_hex, one) {
    EXPECT_EQ(top::to_hex(1), "1");
    EXPECT_EQ(top::to_hex(1, "0x"), "0x1");
    EXPECT_EQ(top::to_hex(1, "0X"), "0X1");
}

TEST(to_hex, one_byte) {
    EXPECT_EQ(top::to_hex(0x01), "1");
    EXPECT_EQ(top::to_hex(0x01, "0x"), "0x1");
    EXPECT_EQ(top::to_hex(0x01, "0X"), "0X1");
}

TEST(to_hex, two_bytes) {
    EXPECT_EQ(top::to_hex(0x0102), "102");
    EXPECT_EQ(top::to_hex(0x0102, "0x"), "0x102");
    EXPECT_EQ(top::to_hex(0x0102, "0X"), "0X102");
}

TEST(to_hex, eight_bytes) {
    EXPECT_EQ(top::to_hex(0x0102030405060708), "102030405060708");
    EXPECT_EQ(top::to_hex(0x0102030405060708, "0x"), "0x102030405060708");
    EXPECT_EQ(top::to_hex(0x0102030405060708, "0X"), "0X102030405060708");
}

NS_END2
