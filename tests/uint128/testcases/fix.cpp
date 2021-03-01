#include <gtest/gtest.h>

#include "uint128_t.h"

TEST(Arithmetic, increment){
    top::xstake::uint128_t value(0);
    EXPECT_EQ(++value, 1);
    EXPECT_EQ(value++, 1);
    EXPECT_EQ(++value, 3);
}

TEST(Arithmetic, decrement){
    top::xstake::uint128_t value(0);
    EXPECT_EQ(--value, top::xstake::uint128_t(0xffffffffffffffffULL, 0xffffffffffffffffULL));
    EXPECT_EQ(value--, top::xstake::uint128_t(0xffffffffffffffffULL, 0xffffffffffffffffULL));
    EXPECT_EQ(--value, top::xstake::uint128_t(0xffffffffffffffffULL, 0xfffffffffffffffdULL));
}