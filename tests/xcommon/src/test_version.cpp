// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xip.h"

#include <gtest/gtest.h>

#include <cstdint>

TEST(xcommon, version) {
    top::common::xversion_t version1{ 0 };
    top::common::xnetwork_version_t network_veersion1{ 0 };
    ASSERT_EQ(version1, network_veersion1);

    top::common::xversion_t version2{ 8 };
    ASSERT_EQ(version2, network_veersion1);
}
