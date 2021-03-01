// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if 0
#include "xcommon/xip.h"

#include <gtest/gtest.h>

#include <cstdint>

TEST(xcommon, xip) {
    top::common::xip2_t xip;
    top::common::xzone_id_t expected{ 127 };
    xip.zone_id(expected);
    auto actual = xip.zone_id();

    EXPECT_EQ(expected, actual);
    EXPECT_EQ(static_cast<std::uint8_t>(127), actual.value());

    auto high = xip.raw_low_part();
    EXPECT_NE(0, high);
}
#endif
