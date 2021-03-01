#include <gtest/gtest.h>

#include "uint128_t.h"

TEST(BitWise, xor){
    top::xstake::uint128_t t  ((bool)     true);
    top::xstake::uint128_t f  ((bool)     false);
    top::xstake::uint128_t u8 ((uint8_t)  0xaaULL);
    top::xstake::uint128_t u16((uint16_t) 0xaaaaULL);
    top::xstake::uint128_t u32((uint32_t) 0xaaaaaaaaULL);
    top::xstake::uint128_t u64((uint64_t) 0xaaaaaaaaaaaaaaaa);

    const top::xstake::uint128_t val(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL);

    EXPECT_EQ(t   ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f1ULL));
    EXPECT_EQ(f   ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL));
    EXPECT_EQ(u8  ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f05aULL));
    EXPECT_EQ(u16 ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f05a5aULL));
    EXPECT_EQ(u32 ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f05a5a5a5aULL));
    EXPECT_EQ(u64 ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0x5a5a5a5a5a5a5a5aULL));

    EXPECT_EQ(t   ^= val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f1ULL));
    EXPECT_EQ(f   ^= val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL));
    EXPECT_EQ(u8  ^= val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f05aULL));
    EXPECT_EQ(u16 ^= val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f05a5aULL));
    EXPECT_EQ(u32 ^= val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f05a5a5a5aULL));
    EXPECT_EQ(u64 ^= val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0x5a5a5a5a5a5a5a5aULL));

    // zero
    EXPECT_EQ(top::xstake::uint128_t() ^ val, val);
}

TEST(External, xor){
    bool     t   = true;
    bool     f   = false;
    uint8_t  u8  = 0xaaULL;
    uint16_t u16 = 0xaaaaULL;
    uint32_t u32 = 0xaaaaaaaaULL;
    uint64_t u64 = 0xaaaaaaaaaaaaaaaaULL;

    const top::xstake::uint128_t val(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL);

    EXPECT_EQ(t   ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f1ULL));
    EXPECT_EQ(f   ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f0f0ULL));
    EXPECT_EQ(u8  ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f0f05aULL));
    EXPECT_EQ(u16 ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f0f0f05a5aULL));
    EXPECT_EQ(u32 ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0xf0f0f0f05a5a5a5aULL));
    EXPECT_EQ(u64 ^  val, top::xstake::uint128_t(0xf0f0f0f0f0f0f0f0ULL, 0x5a5a5a5a5a5a5a5aULL));

    EXPECT_EQ(t   ^= val, true);
    EXPECT_EQ(f   ^= val, true);
    EXPECT_EQ(u8  ^= val, (uint8_t)  0x5aULL);
    EXPECT_EQ(u16 ^= val, (uint16_t) 0x5a5aULL);
    EXPECT_EQ(u32 ^= val, (uint32_t) 0x5a5a5a5aULL);
    EXPECT_EQ(u64 ^= val, (uint64_t) 0x5a5a5a5a5a5a5a5aULL);

    // zero
    EXPECT_EQ(top::xstake::uint128_t() ^ val, val);
}
