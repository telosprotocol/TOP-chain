// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xuint.hpp"

#include <gtest/gtest.h>

#include <limits>

#define XTEST_BODY_INT(NUM)                                                                        \
    auto const e1 = std::numeric_limits<std::int ## NUM ## _t>::max();                             \
    auto const e2 = std::numeric_limits<std::int ## NUM ## _t>::min();                             \
    auto const e3 = static_cast<std::int ## NUM ## _t>(0);                                         \
    auto const e4 = std::numeric_limits<std::uint ## NUM ## _t>::max();                            \
    auto const e5 = std::numeric_limits<std::uint ## NUM ## _t>::min();                            \
    auto const e6 = std::numeric_limits<std::uint ## NUM ## _t>::max() >> 1;                       \
                                                                                                   \
    auto r1 = top::xconvert_t<std::int ## NUM ## _t>::to<top::big_endian>(e1);                     \
    r1 = top::xconvert_t<std::int ## NUM ## _t, top::big_endian>::to<top::system_endian>(r1);      \
    EXPECT_EQ(e1, r1);                                                                             \
    auto r2 = top::xconvert_t<std::int ## NUM ## _t>::to<top::little_endian>(e1);                  \
    r2 = top::xconvert_t<std::int ## NUM ## _t, top::little_endian>::to<top::system_endian>(r2);   \
    EXPECT_EQ(e1, r2);                                                                             \
                                                                                                   \
    auto r3 = top::xconvert_t<std::int ## NUM ## _t>::to<top::big_endian>(e2);                     \
    r3 = top::xconvert_t<std::int ## NUM ## _t, top::big_endian>::to<top::system_endian>(r3);      \
    EXPECT_EQ(e2, r3);                                                                             \
    auto r4 = top::xconvert_t<std::int ## NUM ## _t>::to<top::little_endian>(e2);                  \
    r4 = top::xconvert_t<std::int ## NUM ## _t, top::little_endian>::to<top::system_endian>(r4);   \
    EXPECT_EQ(e2, r4);                                                                             \
                                                                                                   \
    auto r5 = top::xconvert_t<std::int ## NUM ## _t>::to<top::big_endian>(e3);                     \
    r5 = top::xconvert_t<std::int ## NUM ## _t, top::big_endian>::to<top::system_endian>(r5);      \
    EXPECT_EQ(e3, r5);                                                                             \
    auto r6 = top::xconvert_t<std::int ## NUM ## _t>::to<top::little_endian>(e3);                  \
    r6 = top::xconvert_t<std::int ## NUM ## _t, top::little_endian>::to<top::system_endian>(r6);   \
    EXPECT_EQ(e3, r5);                                                                             \
                                                                                                   \
    auto r7 = top::xconvert_t<std::uint ## NUM ## _t>::to<top::big_endian>(e4);                    \
    r7 = top::xconvert_t<std::int ## NUM ## _t, top::big_endian>::to<top::system_endian>(r7);      \
    EXPECT_EQ(e4, r7);                                                                             \
    auto r8 = top::xconvert_t<std::uint ## NUM ## _t>::to<top::little_endian>(e4);                 \
    r8 = top::xconvert_t<std::uint ## NUM ## _t, top::little_endian>::to<top::system_endian>(r8);  \
    EXPECT_EQ(e4, r8);                                                                             \
                                                                                                   \
    auto r9 = top::xconvert_t<std::uint ## NUM ## _t>::to<top::big_endian>(e5);              \
    r9 = top::xconvert_t<std::int ## NUM ## _t, top::big_endian>::to<top::system_endian>(r9);      \
    EXPECT_EQ(e5, r9);                                                                             \
    auto r10 = top::xconvert_t<std::uint ## NUM ## _t>::to<top::little_endian>(e5);                \
    r10 = top::xconvert_t<std::uint ## NUM ## _t, top::little_endian>::to<top::system_endian>(r10);\
    EXPECT_EQ(e5, r10);                                                                            \
                                                                                                   \
    auto r11 = top::xconvert_t<std::uint ## NUM ## _t>::to<top::big_endian>(e6);                   \
    r11 = top::xconvert_t<std::int ## NUM ## _t, top::big_endian>::to<top::system_endian>(r11);    \
    EXPECT_EQ(e6, r11);                                                                            \
    auto r12 = top::xconvert_t<std::uint ## NUM ## _t>::to<top::little_endian>(e6);                \
    r12 = top::xconvert_t<std::uint ## NUM ## _t, top::little_endian>::to<top::system_endian>(r12);\
    EXPECT_EQ(e6, r12)

//TEST(xbasic, int8_uint8)
//{
//    XTEST_BODY_INT(8);
//}
//
//TEST(xbasic, int16_uint16)
//{
//    XTEST_BODY_INT(16);
//}
//
//TEST(xbasic, int32_uint32)
//{
//    XTEST_BODY_INT(32);
//}
//
//TEST(xbasic, int64_uint64)
//{
//    XTEST_BODY_INT(64);
//}

TEST(xbasic, xuint) {
    using namespace std::rel_ops;

    top::xuint_t<128> const a{ 0 };
    top::xuint_t<128> const b{ 0 };

    EXPECT_EQ(a, b);

    top::xuint_t<128> const c{ 123 };
    EXPECT_NE(c, a);

    top::xuint_t<128> const d{ std::numeric_limits<std::uint64_t>::max() };
    auto const ceqd = c == d;
    EXPECT_EQ(false, ceqd);
    auto const cneq = c != d;
    EXPECT_EQ(true, cneq);

    EXPECT_EQ(true, static_cast<bool>(c));
    EXPECT_NE(false, static_cast<bool>(c));
    EXPECT_NE(true, !c);
    EXPECT_EQ(false, !d);

    EXPECT_EQ(true, c < d);
    EXPECT_EQ(true, d > c);
    EXPECT_EQ(true, a <= b);
    EXPECT_EQ(true, a >= b);
    EXPECT_EQ(true, d >= c);

    top::xuint_t<128> const x{ 0xFFFFFFFF };
    auto y = x >> 8;
    top::xuint_t<128> const z{ 0x00FFFFFF };
    EXPECT_EQ(true, y == z);

    y = x << 8;
    top::xuint_t<128> const p{ 0x000000FFFFFFFF00 };
    EXPECT_EQ(true, y == p);
}
