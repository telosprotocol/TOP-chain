// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/register_routing_table.h"
#include "xgossip/include/gossip_bloomfilter.h"
#include "xgossip/include/gossip_utils.h"
#include "xgossip/include/block_sync_manager.h"
#include "xgossip/tests/gossip_routing.h"

namespace top {

using namespace kadmlia;

namespace gossip {

namespace test {

class TestBlockSyncManager : public testing::Test {
public:
    static void SetUpTestCase() {
        block_sync_manager_ =  BlockSyncManager::Instance();
        ASSERT_TRUE(block_sync_manager_);

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

        uint32_t routing_max_node = 120;
        gossip_root_routing_table_ptr_ = CreateRootRoutingTable(
                core_transport,
                routing_max_node,
                config);
        ASSERT_TRUE(gossip_root_routing_table_ptr_);
        ASSERT_NE(gossip_root_routing_table_ptr_->nodes_size(), routing_max_node);

        block_sync_manager_->SetRoutingTablePtr(gossip_root_routing_table_ptr_);
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
        ASSERT_TRUE(block_sync_manager_);
        ASSERT_TRUE(gossip_root_routing_table_ptr_);
    }

    virtual void TearDown() {
    }

    void CreateMessage(transport::protobuf::RoutingMessage& message, const std::string& src, const std::string& des);

    static kadmlia::RoutingTablePtr CreateRootRoutingTable(
            transport::TransportPtr transport,
            uint32_t size,
            const base::Config& config);

    static  BlockSyncManager* block_sync_manager_;
    static kadmlia::RoutingTablePtr gossip_root_routing_table_ptr_;

};

kadmlia::RoutingTablePtr TestBlockSyncManager::CreateRootRoutingTable(
        transport::TransportPtr transport,
        uint32_t size,
        const base::Config& config) {
    return CreateGossipRootRoutingTableForTest(transport, size, config);
}

BlockSyncManager* TestBlockSyncManager::block_sync_manager_ = nullptr;
kadmlia::RoutingTablePtr TestBlockSyncManager::gossip_root_routing_table_ptr_ = nullptr;


void TestBlockSyncManager::CreateMessage(transport::protobuf::RoutingMessage& message, const std::string& src, const std::string& des) {
    return CreateMessageForTest(message, src, des);
}

TEST_F(TestBlockSyncManager, NewBroadcastMessageOnlyHeader) {
    transport::protobuf::RoutingMessage message;

    auto src = gossip_root_routing_table_ptr_->get_local_node_info()->id();
    ASSERT_FALSE(src.empty());
    auto dst = gossip_root_routing_table_ptr_->GetRandomNode()->node_id;
    ASSERT_FALSE(dst.empty());
    CreateMessage(message, src, dst);

    message.mutable_gossip()->set_block("");
    block_sync_manager_->NewBroadcastMessage(message);
    block_sync_manager_->NewBroadcastMessage(message);
}

TEST_F(TestBlockSyncManager, NewBroadcastMessage_block) {
    transport::protobuf::RoutingMessage message;

    auto src = gossip_root_routing_table_ptr_->get_local_node_info()->id();
    ASSERT_FALSE(src.empty());
    auto dst = gossip_root_routing_table_ptr_->GetRandomNode()->node_id;
    ASSERT_FALSE(dst.empty());
    CreateMessage(message, src, dst);

    block_sync_manager_->NewBroadcastMessage(message);
    block_sync_manager_->NewBroadcastMessage(message);
}

TEST_F(TestBlockSyncManager, NewBroadcastMessage_blocks) {
    transport::protobuf::RoutingMessage message;
    auto src = gossip_root_routing_table_ptr_->get_local_node_info()->id();
    ASSERT_FALSE(src.empty());
    auto dst = gossip_root_routing_table_ptr_->GetRandomNode()->node_id;
    ASSERT_FALSE(dst.empty());
    CreateMessage(message, src, dst);


    for (uint32_t i = 0 ; i < 100; ++i) {
        transport::protobuf::RoutingMessage t_message(message);
        auto index = RandomUint32() % 1;
        if (index == 0) {
            t_message.mutable_gossip()->set_block("");
        }
        block_sync_manager_->NewBroadcastMessage(t_message);
        if (RandomUint32() % 5 == 0) {
            block_sync_manager_->NewBroadcastMessage(t_message);
        }
    }
}


}  // namespace test

}  // namespace gossip

}  // namespace top
