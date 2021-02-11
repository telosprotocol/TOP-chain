// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>


#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xelect_net/tests/config_info.h"
#include "xelect_net/tests/all_node_info.h"

using namespace top;

int main(int argc, char *argv[]) {
    global_platform_type = kChain;
    global_node_signkey = RandomString(256);

    {
        std::fstream fout("/tmp/test_xelect_net.conf", std::ios::out);
        fout << str_file;
        fout.close();

        std::fstream fout_all_node("/tmp/all_node_info.json", std::ios::out);
        fout_all_node << all_node_info_str;
        fout_all_node.close();
    }

    testing::GTEST_FLAG(output) = "xml:";
    testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    int ret = RUN_ALL_TESTS();
    TOP_INFO("exit");
    return ret;
}
