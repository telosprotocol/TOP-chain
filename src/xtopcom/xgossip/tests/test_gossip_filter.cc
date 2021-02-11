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
#include "xgossip/include/gossip_bloomfilter.h"
#include "xgossip/include/gossip_utils.h"
#include "xgossip/include/gossip_filter.h"

namespace top {

using namespace kadmlia;

namespace gossip {

namespace test {

class TestGossipFilter : public testing::Test {
public:
    static void SetUpTestCase() {
        gossip_filter_ = GossipFilter::Instance();
        ASSERT_TRUE(gossip_filter_);
        ASSERT_TRUE(gossip_filter_->Init());
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
        ASSERT_TRUE(gossip_filter_);
    }

    virtual void TearDown() {
    }

    static GossipFilter* gossip_filter_;

};

GossipFilter* TestGossipFilter::gossip_filter_ = nullptr;


TEST_F(TestGossipFilter, FilterMessage_No_Msghash) {
    transport::protobuf::RoutingMessage message;
    // make message
    message.set_is_root(false);
    message.set_broadcast(true);
    message.set_type(100); // any msg type
    message.set_priority(enum_xpacket_priority_type_routine);
    message.set_src_node_id(RandomString(36));
    message.set_des_node_id(RandomString(36));
    message.set_id(CallbackManager::MessageId());
    message.set_data(RandomString(100));

    // broadcast block
    auto gossip_block = message.mutable_gossip();
    gossip_block->set_neighber_count(3);
    gossip_block->set_gossip_type(gossip::kGossipBloomfilter);
    gossip_block->set_max_hop_num(10);
    gossip_block->set_evil_rate(0);
    if (message.is_root()) {
        gossip_block->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);
    } else {
        gossip_block->set_ign_bloomfilter_level(0);
    }
    gossip_block->set_left_overlap(0);
    gossip_block->set_right_overlap(0);
    gossip_block->set_block(message.data());


    /*
    uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + message.data());
    gossip_block->set_msg_hash(msg_hash);
    */

    gossip_filter_->FilterMessage(message);
}

TEST_F(TestGossipFilter, FilterMessage) {
    transport::protobuf::RoutingMessage message;
    // make message
    message.set_is_root(false);
    message.set_broadcast(true);
    message.set_type(100); // any msg type
    message.set_priority(enum_xpacket_priority_type_routine);
    message.set_src_node_id(RandomString(36));
    message.set_des_node_id(RandomString(36));
    message.set_id(CallbackManager::MessageId());
    message.set_data(RandomString(100));

    // broadcast block
    auto gossip_block = message.mutable_gossip();
    gossip_block->set_neighber_count(3);
    gossip_block->set_gossip_type(gossip::kGossipBloomfilter);
    gossip_block->set_max_hop_num(10);
    gossip_block->set_evil_rate(0);
    if (message.is_root()) {
        gossip_block->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);
    } else {
        gossip_block->set_ign_bloomfilter_level(0);
    }
    gossip_block->set_left_overlap(0);
    gossip_block->set_right_overlap(0);
    gossip_block->set_block(message.data());

    uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + message.data());
    gossip_block->set_msg_hash(msg_hash);

    gossip_filter_->FilterMessage(message);
}

TEST_F(TestGossipFilter, FilterMessage_FindOK) {
    transport::protobuf::RoutingMessage message;
    // make message
    message.set_is_root(false);
    message.set_broadcast(true);
    message.set_type(100); // any msg type
    message.set_priority(enum_xpacket_priority_type_routine);
    message.set_src_node_id(RandomString(36));
    message.set_des_node_id(RandomString(36));
    message.set_id(CallbackManager::MessageId());
    message.set_data(RandomString(100));

    // broadcast block
    auto gossip_block = message.mutable_gossip();
    gossip_block->set_neighber_count(3);
    gossip_block->set_gossip_type(gossip::kGossipBloomfilter);
    gossip_block->set_max_hop_num(10);
    gossip_block->set_evil_rate(0);
    if (message.is_root()) {
        gossip_block->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);
    } else {
        gossip_block->set_ign_bloomfilter_level(0);
    }
    gossip_block->set_left_overlap(0);
    gossip_block->set_right_overlap(0);
    gossip_block->set_block(message.data());

    uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + message.data());
    gossip_block->set_msg_hash(msg_hash);

    gossip_filter_->FilterMessage(message);

    gossip_filter_->FilterMessage(message);
}

TEST_F(TestGossipFilter, FilterMessage_AddFail) {
    transport::protobuf::RoutingMessage message;
    // make message
    message.set_is_root(false);
    message.set_broadcast(true);
    message.set_type(100); // any msg type
    message.set_priority(enum_xpacket_priority_type_routine);
    message.set_src_node_id(RandomString(36));
    message.set_des_node_id(RandomString(36));
    message.set_id(CallbackManager::MessageId());
    message.set_data(RandomString(100));

    // broadcast block
    auto gossip_block = message.mutable_gossip();
    gossip_block->set_neighber_count(3);
    gossip_block->set_gossip_type(gossip::kGossipBloomfilter);
    gossip_block->set_max_hop_num(10);
    gossip_block->set_evil_rate(0);
    if (message.is_root()) {
        gossip_block->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);
    } else {
        gossip_block->set_ign_bloomfilter_level(0);
    }
    gossip_block->set_left_overlap(0);
    gossip_block->set_right_overlap(0);
    gossip_block->set_block(message.data());

    uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + message.data());
    gossip_block->set_msg_hash(msg_hash);

    gossip_filter_->FilterMessage(message);

    gossip_filter_->FilterMessage(message);
}


}  // namespace test

}  // namespace gossip

}  // namespace top
