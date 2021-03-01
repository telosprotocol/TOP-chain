#include <gtest/gtest.h>

#include "uint128_t.h"

TEST(Comparison, not_equals){
    EXPECT_EQ(!(top::xstake::uint128_t(0xdeadbeefULL) != top::xstake::uint128_t(0xdeadbeefULL)), true);
    EXPECT_EQ( (top::xstake::uint128_t(0xdeadbeefULL) != top::xstake::uint128_t(0xfee1baadULL)), true);
}

TEST(External, not_equals){
    const bool     t   = true;
    const bool     f   = false;
    const uint8_t  u8  = 0xaaULL;
    const uint16_t u16 = 0xaaaaULL;
    const uint32_t u32 = 0xaaaaaaaaULL;
    const uint64_t u64 = 0xaaaaaaaaaaaaaaaaULL;

    EXPECT_EQ((t   != top::xstake::uint128_t(f)),   true);
    EXPECT_EQ((f   != top::xstake::uint128_t(t)),   true);
    EXPECT_EQ((u8  != top::xstake::uint128_t(u64)), true);
    EXPECT_EQ((u16 != top::xstake::uint128_t(u32)), true);
    EXPECT_EQ((u32 != top::xstake::uint128_t(u16)), true);
    EXPECT_EQ((u64 != top::xstake::uint128_t(u8)),  true);

    EXPECT_EQ((t   != top::xstake::uint128_t(t)),   false);
    EXPECT_EQ((f   != top::xstake::uint128_t(f)),   false);
    EXPECT_EQ((u8  != top::xstake::uint128_t(u8)),  false);
    EXPECT_EQ((u16 != top::xstake::uint128_t(u16)), false);
    EXPECT_EQ((u32 != top::xstake::uint128_t(u32)), false);
    EXPECT_EQ((u64 != top::xstake::uint128_t(u64)), false);
}