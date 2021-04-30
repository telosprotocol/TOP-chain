// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#include "xgossip/include/gossip_bloomfilter_super_node.h"

#include "xbase/xhash.h"
#include "xbase/xcontext.h"
#include "xbase/xbase.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/uint64_bloomfilter.h"
#include "xpbase/base/redis_client.h"
#include "xgossip/include/gossip_utils.h"
#include "xgossip/include/mesages_with_bloomfilter.h"
#include "xgossip/include/block_sync_manager.h"
#include "xpbase/base/redis_utils.h"
#include "xgossip/include/gossip_bloomfilter.h"
#include "xwrouter/register_routing_table.h"


namespace top {

namespace gossip {

GossipBloomfilterSuperNode::GossipBloomfilterSuperNode(transport::TransportPtr transport_ptr)
    : GossipInterface(transport_ptr) { }


GossipBloomfilterSuperNode::GossipBloomfilterSuperNode(
    transport::TransportPtr transport_ptr,
    const GossipInterfacePtr& bloom_gossip_ptr,
    const GossipInterfacePtr& layer_gossip_ptr)
    :GossipInterface(transport_ptr),
     bloom_gossip_ptr_(bloom_gossip_ptr),
     bloom_layer_gossip_ptr_(layer_gossip_ptr) { }

GossipBloomfilterSuperNode::~GossipBloomfilterSuperNode() {}

void GossipBloomfilterSuperNode::Broadcast(uint64_t local_hash64,
     transport::protobuf::RoutingMessage& message,
     std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> neighbors){}

void GossipBloomfilterSuperNode::Broadcast(
    transport::protobuf::RoutingMessage& message,
    kadmlia::RoutingTablePtr& routing_table) {

    uint32_t xnetwork_id;
    uint64_t service_type;
    kadmlia::RoutingTablePtr rt ;
    uint64_t local_hash64;
    bool local_is_super_node = false;
    std::vector<kadmlia::NodeInfoPtr> super_nodes;

    GetBroadcastInfo(message,xnetwork_id,service_type,rt,local_hash64,local_is_super_node,super_nodes);

    if(nullptr == rt) {
        TOP_INFO("Super Broadcast GetBroadcastInfo rt is null xnetwork_id:%u service_type:%llu "
              "msg_hash:%u msg_type:%d gossip_type:%d hop_num:%u",
              xnetwork_id,
              service_type,
              message.gossip().msg_hash(),
              message.type(),
              message.gossip().gossip_type(),
              message.hop_num());
        return;
    }

    if(super_nodes.empty()) {
        TOP_INFO("Super Broadcast super nodes is empty xnetwork_id:%u service_type:%llu local_node_id:%s super_flag:%d "
              "msg_hash:%u msg_type:%d gossip_type:%d hop_num:%u",
              xnetwork_id,
              service_type,
              HexEncode(rt->get_local_node_info()->id()).c_str(),              
              local_is_super_node,
              message.gossip().msg_hash(),
              message.type(),
              message.gossip().gossip_type(),
              message.hop_num());
        return;
    }     
    auto super_neighbors = std::make_shared<std::vector<kadmlia::NodeInfoPtr>>(super_nodes);

    //super nodes broadcast function pointer
    std::shared_ptr<GossipInterface> bloom_share_ptr = bloom_gossip_ptr_.lock();
    //service network broadcast function pointer
    std::shared_ptr<GossipInterface> bloom_layer_share_ptr = bloom_layer_gossip_ptr_.lock();

    if(local_is_super_node) { 
        SuperNodeMsgKey key(message.gossip().msg_hash(),service_type); 
        if(HaveSendToSevice(key)) {
            TOP_DEBUG("Super Broadcast xnetwork_id:%u service_type:%llu local_node_id:%s msg_hash:%u "
              "have send message to layered",
              xnetwork_id,
              service_type,
              HexEncode(rt->get_local_node_info()->id()).c_str(),
              message.gossip().msg_hash());
        } else {
            auto old_bloomfilter = MessageWithBloomfilter::Instance()->GetMessageBloomfilter(message);
            auto super_nodes_hop_num = message.hop_num();
            //super node broad to service network, should set service network super nodes to bloom
            SetServiceSuperNodesToBloom(message,service_type,super_nodes);  
            //broadcast to service network
            if(0 == message.hop_num()) {
                message.set_des_node_id(message.src_node_id());
            }
            message.set_hop_num(0);     
            bloom_layer_share_ptr->Broadcast(message,rt);
            TOP_DEBUG("Super Broadcast xnetwork_id:%u service_type:%llu local_node_id:%s msg_hash:%u "
              "send message to layered",
              xnetwork_id,
              service_type,
              HexEncode(rt->get_local_node_info()->id()).c_str(),
              message.gossip().msg_hash());

            //here bloomfilter in message contain service network all super nodes and part normal nodes
            RestoreServiceSuperNodesToBloom(message,service_type,super_nodes,old_bloomfilter);
            message.set_hop_num(super_nodes_hop_num);
        }
        //broadcast to super nodes        
        bloom_share_ptr->Broadcast(local_hash64,message,super_neighbors);
    } else  {
        if(0 == message.hop_num()) { //promoter
            //broadcast to super nodes
            message.set_des_node_id(message.src_node_id());
            bloom_share_ptr->Broadcast(local_hash64,message,super_neighbors);
        } else  {
            //broadcast to service network
            bloom_layer_share_ptr->Broadcast(message,rt);
        }
    }

    TOP_DEBUG("Super Broadcast xnetwork_id:%u service_type:%llu local_node_id:%s super_flag:%d "
              "msg_hash:%u msg_type:%d gossip_type:%d hop_num:%u super_nodes_num:%d,route_num:%d",
              xnetwork_id,
              service_type,
              HexEncode(rt->get_local_node_info()->id()).c_str(),              
              local_is_super_node,
              message.gossip().msg_hash(),
              message.type(),
              message.gossip().gossip_type(),
              message.hop_num(),
              super_nodes.size(),
              rt->nodes_size());
    return;
}

//prpare get need info for broadcast
void GossipBloomfilterSuperNode::GetBroadcastInfo(
            const transport::protobuf::RoutingMessage & message,
            uint32_t &xnetwork_id,
            uint64_t & service_type,
            kadmlia::RoutingTablePtr & routing_table,
            uint64_t & local_hash64,
            bool & local_is_super_node,
            std::vector<kadmlia::NodeInfoPtr> &super_nodes){
    base::KadmliaKeyPtr kad_key_ptr;     
   
    std::string str_node_id;
    if(0 == message.hop_num()) {
        kad_key_ptr = base::GetKadmliaKey(message.src_node_id());
        service_type= kad_key_ptr->GetServiceType();
        str_node_id = message.src_node_id();        
    } else {
        kad_key_ptr  = base::GetKadmliaKey(message.des_node_id());
        service_type = kad_key_ptr->GetServiceType();
        str_node_id = message.des_node_id();        
    }

    xnetwork_id = kad_key_ptr->Xip().xnetwork_id();
    wrouter::ServiceNodes::Instance()->SuperRoutingNodes(xnetwork_id, super_nodes);

    routing_table = wrouter::GetRoutingTable(service_type, false);
    if(nullptr == routing_table) {
        return;
    }
    local_hash64 = routing_table->get_local_node_info()->hash64();

    std::string local_node_id(routing_table->get_local_node_info()->id());
    local_is_super_node = wrouter::ServiceNodes::Instance()->LocalCheckSuperNode(
                                    xnetwork_id,
                                    service_type,
                                    local_node_id);
    TOP_DEBUG("LocalCheckSuperNode xnetwork_id:%u service_type:%llu local_node_id:%s super_flag:%d "
              "msg_hash:%u msg_type:%d gossip_type:%d",
              xnetwork_id,
              service_type,
              HexEncode(routing_table->get_local_node_info()->id()).c_str(),              
              local_is_super_node,
              message.gossip().msg_hash(),
              message.type(),
              message.gossip().gossip_type());
    return;
}

//when network nodes recive super type broadcast from super node,it can't broadcast to any supernode.
//so should set this network supernode to bloomfiler of message
void GossipBloomfilterSuperNode::SetServiceSuperNodesToBloom(
            transport::protobuf::RoutingMessage & message,
            const uint64_t & service_type,
            const std::vector<kadmlia::NodeInfoPtr> & super_nodes) {
    
    //get bloomfilter in message,if no bloomfilter then init bloomfilter bit with all zero

    auto bloomfilter = MessageWithBloomfilter::Instance()->GetMessageBloomfilter(message);
  
    if (nullptr == bloomfilter) {
        assert(bloomfilter);
        TOP_DEBUG("super broadcast stop gossip for msg_hash:%u message.type(%d) hop_num(%d)", 
                  message.gossip().msg_hash(),message.type(), message.hop_num());
        return;
    }

    for(auto node:super_nodes) {
        #if 0
        TOP_DEBUG("SetServiceSuperNodesToBloom service_type:%llu node->service_type:%llu "
              "msg_hash:%u msg_type:%d gossip_type:%d",
              service_type,
              node->service_type,
              message.gossip().msg_hash(),
              message.type(),
              message.gossip().gossip_type());
        #endif
        if(service_type != node->service_type) {
            //don't need del super nodes not equal self service type
            continue;
        }
        bloomfilter->Add(node->hash64);
    }

    //reset message bloomfiler
    message.clear_bloomfilter();

    const std::vector<uint64_t>& bloomfilter_vec = bloomfilter->Uint64Vector();
    for (uint32_t i = 0; i < bloomfilter_vec.size(); ++i) {
        message.add_bloomfilter(bloomfilter_vec[i]);
    }
    return;
}

//bloomfilter in message has been add service super nodes after broadcast to service network
//so need to restore to orign
void GossipBloomfilterSuperNode::RestoreServiceSuperNodesToBloom(
            transport::protobuf::RoutingMessage & message,
            const uint64_t & service_type,
            const std::vector<kadmlia::NodeInfoPtr> & super_nodes,
            const std::shared_ptr<base::Uint64BloomFilter> & old_bloomfilter){

    auto bloomfilter = MessageWithBloomfilter::Instance()->GetMessageBloomfilter(message);
    
    if (nullptr == bloomfilter) {
        assert(bloomfilter);
        TOP_DEBUG("super broadcast stop gossip for msg_hash:%u message.type(%d) hop_num(%d)", 
                  message.gossip().msg_hash(),message.type(), message.hop_num());
        return;
    }

    for(auto node:super_nodes) {
        #if 0
        TOP_DEBUG("RestoreServiceSuperNodesToBloom service_type:%llu node->service_type:%llu "
              "msg_hash:%u msg_type:%d gossip_type:%d ",
              service_type,
              node->service_type,
              message.gossip().msg_hash(),
              message.type(),
              message.gossip().gossip_type());
        #endif
        if(service_type != node->service_type) {
            continue;
        }
        bloomfilter->Del(node->hash64);
    }

    const std::vector<uint64_t>& old_filter_vc = old_bloomfilter->Uint64Vector();
    for (uint32_t i = 0; i < old_filter_vc.size(); ++i) {
        bloomfilter->Add(old_filter_vc[i]);
    }

    //reset message bloomfiler
    message.clear_bloomfilter();

    const std::vector<uint64_t>& new_filter_vc = bloomfilter->Uint64Vector();
    for (uint32_t i = 0; i < new_filter_vc.size(); ++i) {
        message.add_bloomfilter(new_filter_vc[i]);
    }
    return;
}

bool GossipBloomfilterSuperNode::HaveSendToSevice(const SuperNodeMsgKey& gossip_key) {

    static const uint32_t kMaxMessageQueueSize = 10240u;
    static std::unordered_set<SuperNodeMsgKey,SuperNodeMsgHash> super_to_service_msg_set;
    static std::mutex super_to_service_msg_set_mutex_;

    std::unique_lock<std::mutex> lock(super_to_service_msg_set_mutex_);
    auto iter = super_to_service_msg_set.find(gossip_key);
    if (iter != super_to_service_msg_set.end()) {        
        return true;
    } else {
        super_to_service_msg_set.insert(gossip_key);
    }
    // (Charlie): avoid memory crash
    if (super_to_service_msg_set.size() >= kMaxMessageQueueSize) {
        super_to_service_msg_set.clear();
    }
    return false;
}

void GossipBloomfilterSuperNode::BroadcastWithNoFilter(
    const std::string& local_id,
    transport::protobuf::RoutingMessage& message,
    const std::vector<kadmlia::NodeInfoPtr>& neighbors) {
    TOP_DEBUG("GossipBloomfilterSuperNode Broadcast msg_hash:%u neighbors size %d",
              message.gossip().msg_hash(),
              neighbors.size());

    BlockSyncManager::Instance()->NewBroadcastMessage(message);
    auto gossip_max_hop_num = kGossipDefaultMaxHopNum;
    if (message.gossip().max_hop_num() > 0) {
        gossip_max_hop_num = message.gossip().max_hop_num();
    }
    if (gossip_max_hop_num <= message.hop_num()) {
        TOP_INFO("msg_hash:%u message.type(%d) hop_num(%d) larger than gossip_max_hop_num(%d)",
                 message.gossip().msg_hash(),
                 message.type(),
                 message.hop_num(),
                 gossip_max_hop_num);
        return;
    }

    if (ThisNodeIsEvil(message)) {
        TOP_WARN2("msg_hash:%u this node(%s) is evil", message.gossip().msg_hash(), HexEncode(local_id).c_str());
        return;
    }
    Send(message, neighbors);
}

}  // namespace gossip

}  // namespace top
#endif