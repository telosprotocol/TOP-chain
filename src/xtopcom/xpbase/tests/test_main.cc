//
//  test_main.cc
//  test
//
//  Created by Charlie Xie 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "xpbase/base/top_log.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_timer.h"

using namespace top;

int main(int argc, char *argv[]) {
    top::global_platform_type = top::kChain;
    top::global_node_id_hash = std::string(16, '\0');
    xinit_log("bitvpn_ut.log", true, true);
    xset_log_level(enum_xlog_level_debug);

    // for test timer
    top::base::TimerManager::Instance()->Start(2);

    top::base::Config config;
    config.Set("node", "zone_id", 1);
    testing::GTEST_FLAG(output) = "xml:";
    testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    int ret = RUN_ALL_TESTS();
    TOP_INFO("exit");
    return ret;
}
