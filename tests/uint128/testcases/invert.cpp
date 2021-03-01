#include <gtest/gtest.h>

#include "uint128_t.h"

TEST(BitWise, invert){
    EXPECT_EQ(~top::xstake::uint128_t(0x0000000000000000ULL, 0x0000000000000000ULL), top::xstake::uint128_t(0xffffffffffffffffULL, 0xffffffffffffffffULL));
    EXPECT_EQ(~top::xstake::uint128_t(0x0000000000000000ULL, 0xffffffffffffffffULL), top::xstake::uint128_t(0xffffffffffffffffULL, 0x0000000000000000ULL));
    EXPECT_EQ(~top::xstake::uint128_t(0xffffffffffffffffULL, 0xffffffffffffffffULL), top::xstake::uint128_t(0x0000000000000000ULL, 0x0000000000000000ULL));
}