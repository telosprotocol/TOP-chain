// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/top_utils.h"

using namespace top;

int main(int argc, char *argv[]) {
    top::global_platform_type = top::kPlatform;
    top::global_node_id = RandomString(256);
    top::global_node_signkey = RandomString(256);

    xinit_log("bitvpn_ut.log", true, true);
    xset_log_level(enum_xlog_level_debug);
    top::base::Config config;
    config.Set("node", "zone_id", 1);
    testing::GTEST_FLAG(output) = "xml:";
    testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    int ret = RUN_ALL_TESTS();
    TOP_INFO("exit");
    return ret;
}
