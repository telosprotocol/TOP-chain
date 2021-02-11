// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>


#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_config.h"
#include "xgossip/tests/config_info.h"
#include "xgossip/tests/all_node_info.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/routing_table/callback_manager.h"
#include "xwrouter/root/root_routing_manager.h"
#include "xwrouter/register_routing_table.h"

using namespace top;

int main(int argc, char *argv[]) {
    global_platform_type = kChain;
    global_node_signkey = RandomString(256);

    {
        std::fstream fout("/tmp/test_xgossip.conf", std::ios::out);
        fout << str_file;
        fout.close();

        std::fstream fout_all_node("/tmp/all_node_info.json", std::ios::out);
        fout_all_node << all_node_info_str;
        fout_all_node.close();
    }

    base::Config config;
    config.Init("/tmp/test_xgossip.conf");
    std::string log_path("./xgossip.log");
    config.Get("log", "path", log_path);
    bool log_debug = true;
    config.Get("log", "debug", log_debug);
    bool log_off = false;
    config.Get("log", "off", log_off);
    std::cout << "using log_path:" << log_path << std::endl;
    if (!log_off) {
        xinit_log(log_path.c_str(), true, true);
        if (log_debug) {
            xset_log_level(enum_xlog_level_debug);
        } else {
            xset_log_level(enum_xlog_level_debug);
        }
    }

    config.Get("node", "node_id", global_node_id);
    top::kadmlia::CreateGlobalXid(config);
    top::kadmlia::CallbackManager::Instance();
    auto root_manager_ptr = wrouter::RootRoutingManager::Instance();
    wrouter::SetRootRoutingManager(root_manager_ptr);

    testing::GTEST_FLAG(output) = "xml:";
    testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    int ret = RUN_ALL_TESTS();
    TOP_INFO("exit");
    return ret;
}
