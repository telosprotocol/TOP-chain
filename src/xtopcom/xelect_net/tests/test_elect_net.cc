// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdio.h>
#include <string.h>

#include <string>
#include <memory>
#include <fstream>

#include <gtest/gtest.h>

#define private public
#define protected public
#include "xpbase/base/kad_key/get_kadmlia_key.h"
#include "xelect_net/include/elect_main.h"
#include "xwrouter/multi_routing/net_node.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvnetwork_message.h"
#include "xvnetwork/xcodec/xmsgpack/xmessage_codec.hpp"
#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"
#include "xwrouter/register_routing_table.h"

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace top {

namespace elect {

namespace test {

class TestElectNet : public testing::Test {
public:
    static void SetUpTestCase() {
        common::xnode_id_t node_id { RandomString(40) };
        std::set<uint32_t> xnetwork_id_set = {0, 1};
        chain_demo_ = std::make_shared<ElectMain>(node_id, xnetwork_id_set);
    }

    static void TearDownTestCase() {
        chain_demo_->stop();
        chain_demo_ = nullptr;
    }

    virtual void SetUp() {
        ASSERT_TRUE(chain_demo_);
    }

    virtual void TearDown() {
    }

    void TestForElectManager();

    void TestForNetCard();

    void TestForSend();

    void TestForCmd();

    void TestForVhost();

    static std::shared_ptr<ElectMain> chain_demo_;
};

std::shared_ptr<ElectMain> TestElectNet::chain_demo_ = nullptr;

TEST_F(TestElectNet, runtest) {
    base::Config tmp_config;
    ASSERT_TRUE(tmp_config.Init("/tmp/test_xelect_net.conf"));

    auto & platform_params = data::xplatform_params::get_instance();
    std::string db_path;
    ASSERT_TRUE(tmp_config.Get("db", "path", db_path));
    platform_params.db_path = db_path;
    std::string log_path;
    ASSERT_TRUE(tmp_config.Get("log", "path", log_path));
    platform_params.log_path = log_path;
    std::string country;
    ASSERT_TRUE(tmp_config.Get("node", "country", country));
    platform_params.country = country;
    //uint32_t zone_id;
    //ASSERT_TRUE(tmp_config.Get("node", "zone_id", zone_id));
    //platform_params.zone_id = zone_id;
    std::string local_ip;
    ASSERT_TRUE(tmp_config.Get("node", "local_ip", local_ip));
    platform_params.local_ip = local_ip;
    uint16_t local_port;
    ASSERT_TRUE(tmp_config.Get("node", "local_port", local_port));
    platform_params.local_port = local_port;
    bool first_node;
    ASSERT_TRUE(tmp_config.Get("node", "first_node", first_node));
    platform_params.first_node = first_node;
    bool show_cmd;
    ASSERT_TRUE(tmp_config.Get("node", "show_cmd", show_cmd));
    platform_params.show_cmd = show_cmd;
    //bool client_mode;
    //ASSERT_TRUE(tmp_config.Get("node", "client_mode", client_mode));
    //platform_params.client_mode = client_mode;
    std::string public_endpoints;
    ASSERT_TRUE(tmp_config.Get("node", "public_endpoints", public_endpoints));
    platform_params.public_endpoints = public_endpoints;

    ASSERT_TRUE(tmp_config.Get("node", "node_id", global_node_id));

    chain_demo_->InitLog(log_path, true, false);

    auto nodecallback = [](std::string const& node_addr, std::string const& node_sign) -> int32_t {
        return 1;
    };

    chain_demo_->RegisterNodeCallback(nodecallback);


    chain_demo_->start();

    ASSERT_TRUE(global_node_id.size() > 0);

    char* argv[3];
    for (uint32_t i  = 0; i < 3; ++i) {
        argv[i] = new char[64];
        memset(argv[i], '\0', 64);
    }
    strcpy(argv[0], "thisbin");
    strcpy(argv[1], "-c");
    strcpy(argv[2], "/tmp/test_xelect_net.conf");
    base::Config tt_config;
    auto father = dynamic_cast<MultilayerNetwork*>(chain_demo_.get());
    ASSERT_EQ(father->HandleParamsAndConfig(3, argv, tt_config), 0);

    std::cout << "end call functions for ElectMain..." << std::endl;
    SleepMs(5 * 1000);

    TestForElectManager();

    // test for netcard register
    TestForNetCard();


    // test for vhost
    TestForVhost();

    // test for netcard send
    TestForSend();

    // test for elect cmd
    TestForCmd();


}


void TestElectNet::TestForElectManager() {
    auto elect_manager = chain_demo_->GetElectManager();
    ASSERT_TRUE(elect_manager);

    // read a JSON file
    std::ifstream in("/tmp/all_node_info.json");
    ASSERT_TRUE(in);

    json all_info;
    in >> all_info;
    in.close();

    std::vector<ElectNetNode> elect_data;
    std::vector<base::XipParser> local_xip_vec;
    for (json::iterator it = all_info.begin(); it != all_info.end(); ++it) {
        auto key = it.key();
        std::cout << "all_info_key:" << key  << std::endl;
        if (key == "all") {
            std::cout << "read all node_id:" << std::endl;
            std::cout << it.value() << std::endl;
            continue;
        }
        if (key == "exchange") {
            std::cout << "read exchange_id:" << std::endl;
            std::cout << it.value() << std::endl;
            continue;
        }
        std::cout << std::setw(20) << "node_id" << std::setw(20) << "pubkey" << std::setw(36) << "xip" << std::setw(4) << "gid" << std::endl;
        for (json::iterator lit = it.value().begin(); lit != it.value().end(); ++ lit) {
            std::string node_id     = (*lit)["node_id"];
            std::string public_key  = (*lit)["pubkey"];
            std::string xipstr         = (*lit)["xip"];
            uint32_t    gid         = (*lit)["gid"];

            auto sub_node_id    = node_id.substr(0, 6)     + "..." +    node_id.substr(node_id.size() - 6, 6);
            auto sub_public_key = public_key.substr(0, 6)  + "..." +    public_key.substr(public_key.size() - 6, 6);
            std::cout << std::setw(20) << sub_node_id << std::setw(20) << sub_public_key << std::setw(36) << xipstr << std::setw(4) << gid << std::endl;

            base::XipParser xip(HexDecode(xipstr));
            ElectNetNode enode {node_id, public_key, xip, "", static_cast<uint8_t>(gid), static_cast<uint64_t>(1)}; // all version is 1
            elect_data.push_back(enode);

            if (node_id == global_node_id) {
                local_xip_vec.push_back(xip);
            }
        } // end for (json::iterator lit = it.value().begin()
    } // end for (json::iterator it = all_info.begin()

    TOP_INFO("elect_manager_demo onelectupdated end, size:%u", elect_data.size());

    elect_manager->OnElectUpdated(elect_data);

    ASSERT_TRUE(local_xip_vec.size() > 0);

    base::XipParser drop_xip = local_xip_vec[RandomUint32() % local_xip_vec.size()];
    std::vector<std::string> drop_account_vec = {RandomString(40), RandomString(40)};
    elect_manager->OnElectDropNodes(drop_xip,  drop_account_vec);

    auto kadkey = base::GetKadmliaKey(local_xip_vec[0]);
    uint64_t service_type = kadkey->GetServiceType();
    auto routing_table = wrouter::GetRoutingTable(service_type, false);
    ASSERT_TRUE(routing_table);

    kadmlia::NodeInfoPtr node;
    auto add_kadkey = base::GetKadmliaKey(local_xip_vec[0], RandomString(40));
    node.reset(new kadmlia::NodeInfo(add_kadkey->Get()));
    node->public_ip = "127.0.0.1";
    node->public_port = RandomUint32() % 200 + 20000;
    node->local_ip  = "127.0.0.1";
    node->local_port = node->public_port;
    node->hash64 = base::xhash64_t::digest(node->node_id);

    routing_table->AddNode(node);
    routing_table->AddNode(node);
    routing_table->AddNode(node);
    routing_table->DropNode(node);

    for (const auto& xip : local_xip_vec) {
        elect_manager->OnElectQuit(xip);
    }

    // using chain way
    data::election::xelection_result_store_t er;
    for (uint32_t i = 0; i < 20; ++i) {
        common::xnetwork_id_t x {i};
        data::election::xelection_network_result_t t;
        er.insert(std::make_pair(x, t));
    }
    common::xzone_id_t zone {1};
    elect_manager->OnElectUpdated(er, zone);

    common::xip2_t xip2;
    elect_manager->OnElectQuit(xip2);

    uint64_t xip_high = 478937493;
    uint64_t xip_low = 923909930090;
    elect_manager->OnElectDropNodes(xip_high, xip_low, {RandomString(40)});

}


void TestElectNet::TestForNetCard() {
    auto netcard = chain_demo_->GetEcNetcard();
    ASSERT_TRUE(netcard);
    netcard->unregister_message_ready_notify(0);
    netcard->unregister_message_ready_notify(1);
    netcard->unregister_message_ready_notify(2);

    netcard->register_message_ready_notify(nullptr, 0);
    netcard->register_message_ready_notify(nullptr, 1);

    ASSERT_EQ(netcard->rumor_callback_size(), 2);

    ASSERT_FALSE(netcard->get_rumor_callback(0));
    ASSERT_FALSE(netcard->get_rumor_callback(1));
    ASSERT_FALSE(netcard->get_rumor_callback(2));

    auto callback_vec = netcard->getall_rumor_callback();
    ASSERT_EQ(callback_vec.size(), 0);

#ifdef DEBUG
    netcard->test_send("", true, true);
#endif
}


void TestElectNet::TestForSend() {
    auto netcard = chain_demo_->GetEcNetcard();
    ASSERT_TRUE(netcard);
    bool is_broadcast = true;

    base::XipParser xip;
    xip.set_xnetwork_id(0);
    xip.set_zone_id(1);
    xip.set_cluster_id(10);
    xip.set_group_id(63);

    base::KadmliaKeyPtr send_kadkey = nullptr;
    base::KadmliaKeyPtr recv_kadkey = nullptr;
    elect::xelect_message_t message;

    ASSERT_EQ(netcard->send(send_kadkey, recv_kadkey, message, is_broadcast), 2);
    send_kadkey = base::GetKadmliaKey(xip);
    recv_kadkey = base::GetKadmliaKey(xip, RandomString(40));
    is_broadcast = false;
    ASSERT_EQ(netcard->send(send_kadkey, recv_kadkey, message, is_broadcast), 4);

    recv_kadkey = base::GetKadmliaKey(RandomString(40), true);
    is_broadcast = true;
    EXPECT_EQ(netcard->send(send_kadkey, recv_kadkey, message, is_broadcast), 0);

    recv_kadkey = base::GetKadmliaKey(xip, RandomString(40));
    ASSERT_EQ(netcard->send(send_kadkey, recv_kadkey, message, is_broadcast), 4);
}


void TestElectNet::TestForCmd() {
    auto& cmd = chain_demo_->GetElectCmd();

    std::string cmdline = "help";
    cmd.ProcessCommand(cmdline);

    cmdline = "gid";
    cmd.ProcessCommand(cmdline);


    uint64_t kroot_service_type = 0xFFFFFF;
    cmdline = "getnode " + std::to_string(kroot_service_type);
    cmd.ProcessCommand(cmdline);

    cmdline = "vsend desdesdesdesdes 1 4";
    cmd.ProcessCommand(cmdline);

    cmdline = "prt";
    cmd.ProcessCommand(cmdline);

    cmdline = "root";
    cmd.ProcessCommand(cmdline);

    cmdline = "prt_all";
    cmd.ProcessCommand(cmdline);

    cmdline = "xm";
    cmd.ProcessCommand(cmdline);

    cmdline = "xm 1";
    cmd.ProcessCommand(cmdline);

    cmdline = "xm 1 1";
    cmd.ProcessCommand(cmdline);

    cmdline = "xm 1 1 1";
    cmd.ProcessCommand(cmdline);

    cmdline = "xm 1 1 1 1";
    cmd.ProcessCommand(cmdline);

    cmdline = "xm 1 1 1 1 1";
    cmd.ProcessCommand(cmdline);

    cmdline = "brthash 4 2 3 100";
    cmd.ProcessCommand(cmdline);

    cmdline = "crt 1 100 1 1 3 3 10 0 2 0 0";
    cmd.ProcessCommand(cmdline);


    cmdline = "csb 1 100 1 1 3 3 10 0 2 0 0";
    cmd.ProcessCommand(cmdline);
}


void TestElectNet::TestForVhost() {
    auto net_driver1 = chain_demo_->GetEcVhost(1);
    ASSERT_TRUE(net_driver1);

    auto net_driver = chain_demo_->GetEcVhost(0);
    ASSERT_TRUE(net_driver);

    /*
    auto cb = [](common::xnode_id_t const&, xbyte_buffer_t const &) -> void {
        return;
    };
    net_driver->register_message_ready_notify(cb);
    */
    net_driver->host_node_id();
    net_driver->host_node();

    // send_to
    common::xnode_id_t node_id {RandomString(40)};
    network::xtransmission_property_t transmission_property;

    common::xnetwork_id_t nid {0};
    common::xzone_id_t zid {1};
    common::xcluster_id_t cid {0};
    common::xgroup_id_t gid {0};
    common::xcluster_address_t xc {nid, zid, cid, gid};
    common::xnode_address_t src {xc};
    common::xnode_address_t dst {xc};
    vnetwork::xmessage_t message {};
    vnetwork::xvnetwork_message_t const vnetwork_message{ src, dst, message, 0};
    auto const bytes_message = top::codec::msgpack_encode(vnetwork_message);

    net_driver->send_to(node_id, bytes_message, transmission_property);
    

    // spead_rumor
    net_driver->spread_rumor(bytes_message);
    net_driver->spread_rumor(src.cluster_address().sharding_info(), bytes_message);

    // forward-broadcast
    net_driver->forward_broadcast(dst.cluster_address().sharding_info(), dst.type(), bytes_message);

    // p2p_bootstrap
    std::vector<network::xdht_node_t> seeds;
    net_driver->p2p_bootstrap(seeds);

    // direct send
    network::xnode_t to;
    xbyte_buffer_t verification_data;
    net_driver->direct_send_to(to, verification_data, transmission_property);

    // neighbors
    ASSERT_TRUE(net_driver->neighbors().size() == 0);

    ASSERT_TRUE(net_driver->neighbor_size_upper_limit() == 256);


    //net_driver->unregister_message_ready_notify();
}

}  // namespace test

}  // namespace kadmlia

}  // namespace top
