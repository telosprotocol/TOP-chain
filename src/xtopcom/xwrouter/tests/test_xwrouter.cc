// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string.h>

#include <string>

#include <gtest/gtest.h>

#include "xpbase/base/endpoint_util.h"
#include "xpbase/base/kad_key/platform_kadmlia_key.h"
#define protected public
#define private public
#include "xtransport/udp_transport/udp_transport.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/local_node_info.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/root/root_routing.h"
#include "xwrouter/root/root_routing_manager.h"
#include "xwrouter/message_handler/wrouter_message_handler.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xkad/nat_detect/nat_manager_intf.h"
#include "xwrouter/xwrouter.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/multi_routing/service_node_cache.h"
#include "xgossip/include/gossip_utils.h"

namespace top {

namespace wrouter {

namespace test {

class TestXwrouter : public testing::Test {
public:
	static void SetUpTestCase() {
        w_ = Wrouter::Instance();
        ASSERT_TRUE(w_);
	}

	static void TearDownTestCase() {
        //w_ = nullptr;
	}

	virtual void SetUp() {
        ASSERT_TRUE(w_);
	}

	virtual void TearDown() {
	}


    void CreateMessageForTest(transport::protobuf::RoutingMessage& message, const std::string& src, const std::string& des);
    void send();


    static Wrouter* w_;
};

Wrouter* TestXwrouter::w_ = nullptr;

void TestXwrouter::CreateMessageForTest(transport::protobuf::RoutingMessage& message, const std::string& src, const std::string& des) {
    // make message
    message.set_is_root(false);
    message.set_broadcast(true);
    message.set_type(100); // any msg type
    message.set_priority(enum_xpacket_priority_type_routine);
    message.set_src_node_id(src);
    message.set_des_node_id(des);
    message.set_id(kadmlia::CallbackManager::MessageId());
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

    uint32_t vhash = base::xhash32_t::digest(message.data());
    std::string header_hash = std::to_string(vhash);
    gossip_block->set_header_hash(header_hash);

    uint32_t msg_hash = base::xhash32_t::digest(std::to_string(message.id()) + message.data());
    gossip_block->set_msg_hash(msg_hash);
}

void TestXwrouter::send() {
    transport::protobuf::RoutingMessage original_message;
    auto dst_kadkey = base::GetKadmliaKey(RandomString(40), true);
    CreateMessageForTest(original_message, global_xid->Get(), dst_kadkey->Get());

    transport::protobuf::RoutingMessage m1(original_message);
    w_->send(m1);

    transport::protobuf::RoutingMessage m2(original_message);
    m2.set_is_root(false);
    m2.set_broadcast(false);
    w_->send(m2);
    w_->SendToLocal(m2);
    w_->SendToLocal(m2);
    w_->SendToLocal(m2);
    w_->SendToLocal(m2);
    w_->SendDirect(m2, "127.0.0.1", 20001);

    w_->GetAllLocalIds();
    w_->GetAllLocalXips();

    transport::protobuf::RoutingMessage m3(original_message);
    m3.set_is_root(false);
    m3.set_broadcast(true);
    w_->send(m3);
    m3.mutable_gossip()->set_gossip_type(gossip::kGossipLayeredBroadcast);
    w_->send(m3);
    m3.mutable_gossip()->set_gossip_type(gossip::kGossipBloomfilterAndLayered);
    w_->send(m3);
    m3.mutable_gossip()->set_gossip_type(gossip::kGossipSetFilterAndLayered);
    w_->send(m3);
    m3.mutable_gossip()->set_gossip_type(gossip::kGossipBloomfilterMerge);
    w_->send(m3);
    m3.mutable_gossip()->set_gossip_type(gossip::kGossipBloomfilterZone);
    w_->send(m3);
    m3.mutable_gossip()->set_gossip_type(gossip::kGossipBloomfilterSuperNode);
    w_->send(m3);


    transport::protobuf::RoutingMessage m4(original_message);
    m4.set_is_root(true);
    m4.set_broadcast(true);
    w_->send(m4);
    m4.set_hop_num(1000);
    w_->send(m4);
    m4.set_des_node_id(RandomString(kNodeIdSize));
    w_->send(m4);

}

TEST_F(TestXwrouter, xwrouter) {
    send();
}


}  // namespace test

}  // namespace kadmlia

}  // namespace top
