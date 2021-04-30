// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <iostream>
#include <algorithm>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/xid/xid_def.h"
#include "xpbase/base/xid/xid_generator.h"
#include "xpbase/base/kad_key/chain_kadmlia_key.h"

#define private public
#include "xtransport/udp_transport/udp_transport.h"
#include "xtransport/message_manager/multi_message_handler.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/local_node_info.h"
#include "xwrouter/message_handler/wrouter_message_handler.h"
//#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/register_routing_table.h"

#include "xwrouter/multi_routing/service_node_cache.h"

#include "xgossip/include/broadcast_layered.h"
#include "xgossip/include/gossip_bloomfilter.h"
#include "xgossip/include/gossip_bloomfilter_merge.h"
#include "xgossip/include/gossip_bloomfilter_zone.h"
#include "xgossip/include/gossip_bloomfilter_layer.h"
#include "xgossip/include/gossip_bloomfilter_super_node.h"


using ::testing::AtLeast;
using ::testing::_;
using ::testing::Return;

using namespace kadmlia;
using namespace transport::protobuf;

NS_BEG3(top,gossip,test)

class MockGossip : public wrouter::ServiceNodes {
 public:   
    //MOCK_METHOD0(PenUp, void());
    //MOCK_METHOD1(Turn, void(int degrees));
    MOCK_METHOD2(SuperRoutingNodes, void(const uint32_t &x, std::vector<kadmlia::NodeInfoPtr> & y));
    //MOCK_CONST_METHOD0(GetX, int());
};

class TestGossipBloomfilterSuperNode : public testing::Test {
public:
    enum TestRoutingType {

    };
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() { 
        global_platform_type = kChain;
        InitSrcDstNodeInfo();
        m_src_service_type = base::GetKadmliaKey(src_node_ptr->id())->GetServiceType();
        m_dst_service_type = base::GetKadmliaKey(dst_node_ptr->id())->GetServiceType();
        CreateTransportAndRtTable("unkown",src_node_ptr); 
        ASSERT_NE(nullptr,wrouter::GetRoutingTable(m_src_service_type, false));
        CreateBloomSuperPtr(transport_ptr);
    }

    virtual void TearDown() {
    }
   
    void InitSrcDstNodeInfo() {

        std::string idtype(top::kadmlia::GenNodeIdType("CN", "VPN"));        
        src_node_ptr.reset(new LocalNodeInfo());
        auto kad_key = std::make_shared<base::ChainKadmliaKey>();
        kad_key->set_xnetwork_id(kEdgeXVPN);
        kad_key->set_zone_id(26);
        kad_key->set_cluster_id(26);
        kad_key->set_group_id(26);
        kad_key->set_node_id(26);
 
        src_node_ptr->Init(
            "168.1.1.1", 1111, false, false, idtype, kad_key, kad_key->GetServiceType(), kRoleEdge);
        src_node_ptr->set_public_ip("10.0.0.1");
        src_node_ptr->set_public_port(10000);

  
        //dst node no init success
        dst_node_ptr.reset(new LocalNodeInfo());
        kad_key->set_xnetwork_id(kEdgeXVPN);
        kad_key->set_zone_id(27);
        kad_key->set_cluster_id(27);
        kad_key->set_group_id(27);
        kad_key->set_node_id(27);
 
        dst_node_ptr->Init(
            "168.1.1.10", 1111, false, false, idtype, kad_key, kad_key->GetServiceType(), kRoleEdge);
        dst_node_ptr->set_public_ip("10.0.0.10");
        dst_node_ptr->set_public_port(10000);    

        //printf("++++++++InitSrcDstNodeInfo src_node_ptr->id:%s+++++++++\n",HexEncode(src_node_ptr->id()).c_str());
        //printf("++++++++InitSrcDstNodeInfo dst_node_ptr->id:%s ++++++++\n",HexEncode(dst_node_ptr->id()).c_str());    

    }

    RoutingTablePtr CreateTransportAndRtTable(const std::string & peer,
                                              const LocalNodeInfoPtr & node_info) {

        transport_ptr.reset(new top::transport::UdpTransport());
        auto thread_message_handler = std::make_shared<transport::MultiThreadHandler>();
        thread_message_handler->Init();
        transport_ptr->Start("168.1.1.2",
                        2222,
                        thread_message_handler.get());
  
        routing_table_ptr.reset(new top::kadmlia::RoutingTable(
                transport_ptr, kNodeIdSize, node_info));
   
        EXPECT_TRUE(routing_table_ptr->Init());

        top::wrouter::UnregisterRoutingTable(m_src_service_type);
        top::wrouter::RegisterRoutingTable(m_src_service_type, routing_table_ptr);
        //printf("++++++++++++m_src_service_type.service_type:%lu++++++++++++\n", m_src_service_type);

        return routing_table_ptr;
    }

    std::shared_ptr<GossipBloomfilterSuperNode> 
    CreateBloomSuperPtr(const top::transport::TransportPtr &transport_ptr){

        auto bloom_gossip_ptr = std::make_shared<GossipBloomfilter>(transport_ptr);
        auto layered_gossip_ptr = std::make_shared<BroadcastLayered>(transport_ptr);
        bloom_gossip_super_ptr = std::make_shared<GossipBloomfilterSuperNode>(
                                    transport_ptr,
                                    bloom_gossip_ptr,
                                    layered_gossip_ptr);
        return bloom_gossip_super_ptr;
   }

    void ConstructMessage(RoutingMessage &message,
                          const LocalNodeInfoPtr& src_node_ptr,
                          const LocalNodeInfoPtr& dst_node_ptr)
    {        
        HopInfo* hop = message.add_hop_nodes();        
        hop->set_node_id(src_node_ptr->id());

        message.set_src_service_type(m_src_service_type);
        message.set_des_service_type(m_dst_service_type);
        message.set_hop_num(0);
        message.set_src_node_id(src_node_ptr->id());
        message.set_des_node_id(dst_node_ptr->id());
        message.set_type(kTellBootstrapStopped);
        message.set_id(CallbackManager::MessageId());

        //printf("++++++++++++src_node_ptr->id:%s  \n++++++++++++\n", HexEncode(src_node_ptr->id()).c_str());
        //printf("++++++++++++dst_node_ptr->id:%s  \n++++++++++++\n", HexEncode(dst_node_ptr->id()).c_str());
    }

    void ConstructSuperNodes(std::vector<kadmlia::NodeInfoPtr> &nodes){
            for (int i = 0; i < 10; i++) {
            std::string id = GenRandomID("CN", "VPN");
            NodeInfoPtr node_ptr;
            node_ptr.reset(new NodeInfo(id));
            node_ptr->public_ip = "192.0.0.1";
            node_ptr->public_port = i + 1;
            node_ptr->local_ip = "192.0.0.1";
            node_ptr->local_port = i + 1;
            nodes.push_back(node_ptr);
        }   
    }
    int64_t m_src_service_type;
    int64_t m_dst_service_type;

    top::transport::TransportPtr transport_ptr;
    LocalNodeInfoPtr src_node_ptr;
    LocalNodeInfoPtr dst_node_ptr;
    RoutingTablePtr  routing_table_ptr;
    std::shared_ptr<GossipBloomfilterSuperNode> bloom_gossip_super_ptr;
    RoutingMessage message;     
private: 

};

TEST_F(TestGossipBloomfilterSuperNode, Broadcast) {

}
TEST_F(TestGossipBloomfilterSuperNode, GetBroadcastInfo) {
    uint32_t xnetwork_id;
    uint64_t service_type;
    kadmlia::RoutingTablePtr rt ;
    uint64_t local_hash64;
    bool local_is_super_node = false;
    std::vector<kadmlia::NodeInfoPtr> super_nodes;

    ConstructMessage(message,src_node_ptr,dst_node_ptr);

    wrouter::ServiceNodes::Instance()->Init();
    message.set_hop_num(0);

    bloom_gossip_super_ptr->GetBroadcastInfo(message, xnetwork_id, service_type,rt,local_hash64,
                                            local_is_super_node,super_nodes);
    ASSERT_EQ(service_type,m_src_service_type);

    message.set_hop_num(1);
    bloom_gossip_super_ptr->GetBroadcastInfo(message, xnetwork_id, service_type,rt,local_hash64,
                                            local_is_super_node,super_nodes);
    ASSERT_EQ(service_type,m_dst_service_type);

}
TEST_F(TestGossipBloomfilterSuperNode, SetServiceSuperNodesToBloom) {
}
TEST_F(TestGossipBloomfilterSuperNode, RestoreServiceSuperNodesToBloom) {
}
TEST_F(TestGossipBloomfilterSuperNode, BroadcastWithNoFilter) {
}

NS_END3
#endif