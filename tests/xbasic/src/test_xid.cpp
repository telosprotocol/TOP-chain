// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xid.hpp"

#include <gtest/gtest.h>

#define XBASIC_XID_VERIFY_BODY1(ID_TAG, ID_TYPE, ID_NAME)   \
    struct ID_TAG{};                                        \
    top::xsimple_id_t<ID_TAG, ID_TYPE, 0, std::numeric_limits<ID_TYPE>::max()> ID_NAME;           \
    EXPECT_EQ(ID_NAME, ID_NAME.max());               \
    top::xsimple_id_t<ID_TAG, ID_TYPE, 0, std::numeric_limits<ID_TYPE>::max()> ID_NAME ## 1;      \
    EXPECT_TRUE(ID_NAME == ID_NAME ## 1);                   \
    EXPECT_FALSE(ID_NAME != ID_NAME ## 1);                  \
    EXPECT_FALSE(ID_NAME < ID_NAME ## 1);                   \
    EXPECT_FALSE(ID_NAME ## 1 < ID_NAME)

TEST(xbasic, xid) {
    XBASIC_XID_VERIFY_BODY1(id_tag1, std::uint8_t, u8);
    XBASIC_XID_VERIFY_BODY1(id_tag2, std::uint16_t, u16);
    XBASIC_XID_VERIFY_BODY1(id_tag3, std::uint32_t, u32);
    XBASIC_XID_VERIFY_BODY1(id_tag4, std::uint64_t, u64);
    XBASIC_XID_VERIFY_BODY1(id_tag5, std::int8_t, i8);
    XBASIC_XID_VERIFY_BODY1(id_tag6, std::int16_t, i16);
    XBASIC_XID_VERIFY_BODY1(id_tag7, std::int32_t, i32);
    XBASIC_XID_VERIFY_BODY1(id_tag8, std::int64_t, i64);
}
