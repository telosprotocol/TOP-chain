#include <gtest/gtest.h>

#include "uint128_t.h"

TEST(Arithmetic, unary_plus){
    const top::xstake::uint128_t value(0x12345ULL);
    EXPECT_EQ(+value, value);
}

TEST(Arithmetic, unary_minus){
    const top::xstake::uint128_t val(1);
    const top::xstake::uint128_t neg = -val;
    EXPECT_EQ(-val, neg);
    EXPECT_EQ(-neg, val);
    EXPECT_EQ(neg, top::xstake::uint128_t(0xffffffffffffffffULL, 0xffffffffffffffffULL));
}