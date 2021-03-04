// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <fstream>


#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_config.h"
#include "xwrouter/tests/config_info.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/routing_table/callback_manager.h"
#include "xwrouter/root/root_routing_manager.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/xwrouter.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/multi_routing/service_node_cache.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xtransport/udp_transport/udp_transport.h"



using namespace top;
using namespace wrouter;

int main(int argc, char *argv[]) {
    global_platform_type = kChain;
    global_node_signkey = RandomString(256);

    {
        std::fstream fout("/tmp/test_xwrouter.conf", std::ios::out);
        fout << str_file;
        fout.close();
    }

    auto multi_message_handler = std::make_shared<top::transport::MultiThreadHandler>();
    transport::TransportPtr transport = std::make_shared<top::transport::UdpTransport>();

    base::Config config;
    if (!config.Init("/tmp/test_xwrouter.conf")) {
        TOP_FATAL("init /tmp/test_wrouter.conf failed");
        return -1;
    }
    if (!config.Get("node", "node_id", global_node_id)) {
        TOP_FATAL("global_node_id empty");
        return -1;
    }

    bool show_cmd = true;
    config.Get("node", "show_cmd", show_cmd);
    bool first_node = false;
    config.Get("node", "first_node", first_node);
    std::string db_path;
    config.Get("db", "path", db_path);
    if (!top::kadmlia::CreateGlobalXid(config)) {
        TOP_FATAL("create globalxid faield");
        return -1;
    }
    top::kadmlia::CallbackManager::Instance();

    std::string country;
    config.Get("node", "country", country);

    multi_message_handler->Init();
    multi_message_handler->register_on_dispatch_callback(std::bind(&wrouter::Wrouter::recv,
            wrouter::Wrouter::Instance(),
            std::placeholders::_1,
            std::placeholders::_2));
    // attention: InitWrouter must put befor transport->Start
    base::xiothread_t* io_thread = top::base::xiothread_t::create_thread(
            top::base::xcontext_t::instance(), 0, -1);

    wrouter::Wrouter::Instance()->Init(base::xcontext_t::instance(), io_thread->get_thread_id(), transport);

    std::string local_ip;
    if (!config.Get("node", "local_ip", local_ip)) {
        TOP_FATAL("get local_ip invalid");
        return -1;
    }
    uint16_t local_port = 0;
    config.Get("node", "local_port", local_port);
    /*
    if (transport->Start(
            local_ip,
            local_port,
            multi_message_handler.get()) != 0) {
        TOP_FATAL("transport start failed");
        return -1;
    }
    */

    transport->RegisterOfflineCallback(kadmlia::HeartbeatManagerIntf::OnHeartbeatCallback);

    wrouter::SmallNetNodes::Instance()->Init();
    TOP_INFO("Init SmallNetNodes for Elect Network");
    wrouter::ServiceNodes::Instance()->Init();
    TOP_INFO("Init ServiceNodes for cache nodes");

    auto get_cache_callback = [](const uint64_t& service_type, std::vector<std::pair<std::string, uint16_t>>& vec_bootstrap_endpoint) -> bool {
        return true;
    };
    auto set_cache_callback = [](const uint64_t& service_type, const std::vector<std::pair<std::string, uint16_t>>& vec_bootstrap_endpoint) -> bool {
        return true;
    };


    base::KadmliaKeyPtr kad_key_ptr = base::GetKadmliaKey(global_node_id, true);
    auto root_manager_ptr = wrouter::RootRoutingManager::Instance();
    wrouter::SetRootRoutingManager(root_manager_ptr);
    if (root_manager_ptr->AddRoutingTable(
            transport,
            config,
            kad_key_ptr,
            get_cache_callback,
            set_cache_callback,
            false) != 0) {
        TOP_FATAL("root_manager add root_routing_table failed");
        return -1;
    }

    SleepMs(5 * 1000);
    TOP_INFO("CreateRootManager ok.");


    testing::GTEST_FLAG(output) = "xml:";
    testing::InitGoogleTest(&argc, argv);
    ::testing::InitGoogleMock(&argc, argv);
    int ret = RUN_ALL_TESTS();
    transport->Stop();
    TOP_INFO("exit");
    SleepMs(5 * 1000);
    return ret;
}
