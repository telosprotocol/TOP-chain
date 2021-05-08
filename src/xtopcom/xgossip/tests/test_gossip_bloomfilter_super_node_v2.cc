// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#include <gtest/gtest.h>

#include <iostream>
#include <algorithm>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/xid/xid_def.h"
#include "xpbase/base/xid/xid_generator.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#define private public
#include "xtransport/udp_transport/udp_transport.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/local_node_info.h"
#include "xgossip/include/gossip_bloomfilter.h"
#include "xgossip/include/gossip_bloomfilter_layer.h"
#include "xgossip/include/broadcast_layered.h"
#include "xgossip/include/gossip_bloomfilter_super_node.h"
#include "xgossip/include/gossip_utils.h"
#include "xgossip/tests/gossip_routing.h"

namespace top {

using namespace kadmlia;

namespace gossip {

namespace test {

class TestGossipBloomfilterSuperNodeV2 : public testing::Test {
public:
    static void SetUpTestCase() {
        base::Config config;
        ASSERT_TRUE(config.Init("/tmp/test_xelect_net.conf"));

        std::shared_ptr<transport::MultiThreadHandler> multi_message_handler =  std::make_shared<top::transport::MultiThreadHandler>();
        ASSERT_TRUE(multi_message_handler);
        multi_message_handler->Init();
        transport::TransportPtr core_transport = std::make_shared<top::transport::UdpTransport>();
        ASSERT_TRUE(core_transport);

        std::string local_ip;
        ASSERT_TRUE(config.Get("node", "local_ip", local_ip));
        uint16_t local_port = 0;
        ASSERT_TRUE(config.Get("node", "local_port", local_port));
        ASSERT_EQ(core_transport->Start(
                local_ip,
                local_port,
                multi_message_handler.get()), top::kadmlia::kKadSuccess);

        auto bloom_gossip_ptr = std::make_shared<GossipBloomfilter>(core_transport);
        auto layered_gossip_ptr = std::make_shared<BroadcastLayered>(core_transport);
        bloom_gossip_super_node_ptr_ = std::make_shared<GossipBloomfilterSuperNode>(
                core_transport,
                bloom_gossip_ptr,
                layered_gossip_ptr);



        uint32_t routing_max_node = 120;
        uint32_t xnetwork_id = 1;
        uint8_t zone_id = 1;
        uint8_t cluster_id = 8;
        uint8_t group_id = 65;

        gossip_root_routing_table_ptr_ = CreateRootRoutingTable(
                core_transport,
                routing_max_node,
                config);
        ASSERT_TRUE(gossip_root_routing_table_ptr_);
        ASSERT_NE(gossip_root_routing_table_ptr_->nodes_size(), routing_max_node);

        gossip_routing_table_ptr_ = CreateRoutingTable(
                core_transport,
                routing_max_node,
                xnetwork_id,
                zone_id,
                cluster_id,
                group_id);
        ASSERT_TRUE(gossip_routing_table_ptr_);
        ASSERT_EQ(gossip_routing_table_ptr_->nodes_size(), routing_max_node);
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
        ASSERT_TRUE(bloom_gossip_super_node_ptr_);
        ASSERT_TRUE(gossip_root_routing_table_ptr_);
        ASSERT_TRUE(gossip_routing_table_ptr_);
    }

    virtual void TearDown() {
    }

    static kadmlia::RoutingTablePtr CreateRoutingTable(
            transport::TransportPtr transport,
            uint32_t routing_max_node,
            uint32_t xnetwork_id,
            uint8_t zone_id,
            uint8_t cluster_id,
            uint8_t group_id);

    static kadmlia::RoutingTablePtr CreateRootRoutingTable(
            transport::TransportPtr transport,
            uint32_t size,
            const base::Config& config);

 
    void CreateMessage(transport::protobuf::RoutingMessage& message, const std::string& src, const std::string& des);

    static  GossipInterfacePtr bloom_gossip_super_node_ptr_;
    static kadmlia::RoutingTablePtr gossip_routing_table_ptr_;
    static kadmlia::RoutingTablePtr gossip_root_routing_table_ptr_;

};

GossipInterfacePtr TestGossipBloomfilterSuperNodeV2::bloom_gossip_super_node_ptr_ =  nullptr;
kadmlia::RoutingTablePtr TestGossipBloomfilterSuperNodeV2::gossip_routing_table_ptr_ = nullptr;
kadmlia::RoutingTablePtr TestGossipBloomfilterSuperNodeV2::gossip_root_routing_table_ptr_ = nullptr;


kadmlia::RoutingTablePtr TestGossipBloomfilterSuperNodeV2::CreateRoutingTable(
        transport::TransportPtr transport,
        uint32_t routing_max_node,
        uint32_t xnetwork_id,
        uint8_t zone_id,
        uint8_t cluster_id,
        uint8_t group_id) {
    return CreateGossipRoutingTableForTest(transport, routing_max_node, xnetwork_id, zone_id, cluster_id, group_id);
}

kadmlia::RoutingTablePtr TestGossipBloomfilterSuperNodeV2::CreateRootRoutingTable(
        transport::TransportPtr transport,
        uint32_t size,
        const base::Config& config) {
    return CreateGossipRootRoutingTableForTest(transport, size, config);
}

void TestGossipBloomfilterSuperNodeV2::CreateMessage(transport::protobuf::RoutingMessage& message, const std::string& src, const std::string& des) {
    return CreateMessageForTest(message, src, des);
}

// not important
TEST_F(TestGossipBloomfilterSuperNodeV2, Broadcast_empty_body) {
    transport::protobuf::RoutingMessage message;
    CreateMessage(message, RandomString(36), RandomString(36));
    bloom_gossip_super_node_ptr_->Broadcast(RandomUint64(), message, nullptr);
}

TEST_F(TestGossipBloomfilterSuperNodeV2, Broadcast_maxhop) {
    transport::protobuf::RoutingMessage message;
    auto src = gossip_routing_table_ptr_->get_local_node_info()->id();
    ASSERT_FALSE(src.empty());
    auto dst = gossip_routing_table_ptr_->GetRandomNode()->node_id;
    ASSERT_FALSE(dst.empty());
    CreateMessage(message, src, dst);

    message.set_hop_num(100);
    bloom_gossip_super_node_ptr_->Broadcast( message, gossip_routing_table_ptr_);
    message.mutable_gossip()->set_diff_net(true);
    message.set_hop_num(1);
    bloom_gossip_super_node_ptr_->Broadcast( message, gossip_routing_table_ptr_);
}

TEST_F(TestGossipBloomfilterSuperNodeV2, Broadcast_largehop) {
    transport::protobuf::RoutingMessage message;
    auto src = gossip_routing_table_ptr_->get_local_node_info()->id();
    ASSERT_FALSE(src.empty());
    auto dst = gossip_routing_table_ptr_->GetRandomNode()->node_id;
    ASSERT_FALSE(dst.empty());
    CreateMessage(message, src, dst);

    message.set_hop_num(8);
    bloom_gossip_super_node_ptr_->Broadcast( message, gossip_routing_table_ptr_);
}

TEST_F(TestGossipBloomfilterSuperNodeV2, Broadcast_evail) {
    transport::protobuf::RoutingMessage message;
    auto src = gossip_routing_table_ptr_->get_local_node_info()->id();
    ASSERT_FALSE(src.empty());
    auto dst = gossip_routing_table_ptr_->GetRandomNode()->node_id;
    ASSERT_FALSE(dst.empty());
    CreateMessage(message, src, dst);

    message.mutable_gossip()->set_evil_rate(10);
    bloom_gossip_super_node_ptr_->Broadcast(message, gossip_routing_table_ptr_);
}

TEST_F(TestGossipBloomfilterSuperNodeV2, Broadcast_invalid_hash64) {
}

}  // namespace test

}  // namespace gossip

}  // namespace top
#endif