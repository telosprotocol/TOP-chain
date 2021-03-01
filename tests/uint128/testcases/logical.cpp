#include <gtest/gtest.h>

#include "uint128_t.h"

TEST(Logical, and){
    const top::xstake::uint128_t A(0xffffffff);
    const top::xstake::uint128_t B(0x00000000);

    EXPECT_EQ(A && A, true);
    EXPECT_EQ(A && B, false);
}

TEST(Logical, or){
    const top::xstake::uint128_t A(0xffffffff);
    const top::xstake::uint128_t B(0x00000000);

    EXPECT_EQ(A || A, true);
    EXPECT_EQ(A || B, true);
}

TEST(Logical, not){
    EXPECT_EQ(!top::xstake::uint128_t(0xffffffff), 0);
}