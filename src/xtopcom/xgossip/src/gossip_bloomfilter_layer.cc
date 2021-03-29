// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgossip/include/gossip_bloomfilter_layer.h"

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
#include "xpbase/base/kad_key/get_kadmlia_key.h"

namespace top {

namespace gossip {

GossipBloomfilterLayer::GossipBloomfilterLayer(transport::TransportPtr transport_ptr)
        : GossipInterface(transport_ptr) {}

GossipBloomfilterLayer::~GossipBloomfilterLayer() {}

void GossipBloomfilterLayer::Broadcast(
        uint64_t local_hash64,
        transport::protobuf::RoutingMessage& message,
        std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> prt_neighbors) {
    return;
}

void GossipBloomfilterLayer::Broadcast(
        transport::protobuf::RoutingMessage& message,
        kadmlia::RoutingTablePtr& routing_table) {
    CheckDiffNetwork(message);
    BlockSyncManager::Instance()->NewBroadcastMessage(message);
    auto gossip_max_hop_num = kGossipDefaultMaxHopNum;
    if (message.gossip().max_hop_num() > 0) {
        gossip_max_hop_num = message.gossip().max_hop_num();
    }
    if (gossip_max_hop_num <= message.hop_num()) {
        TOP_INFO("msg_hash:%u msg_type:%d hop_num:%d larger than gossip_max_hop_num:%d",
                message.gossip().msg_hash(),
                message.type(),
                message.hop_num(),
                gossip_max_hop_num);
        return;
    }

    //when super broadcast,if only use msg_hash different serivce on one point may increase stoptimes
    auto kad_key_ptr  = base::GetKadmliaKey(message.des_node_id());    
    uint64_t service_type = kad_key_ptr->GetServiceType();
    MessageKey msg_key(0,message.gossip().msg_hash(),service_type);
    if (MessageWithBloomfilter::Instance()->StopGossip(msg_key, message.gossip().stop_times())) {
        TOP_DEBUG("stop gossip for msg_hash:%u msg_type:%d stop_times:%d", 
                 message.gossip().msg_hash(),message.type(), message.gossip().stop_times());
        return;
    }
    auto bloomfilter = MessageWithBloomfilter::Instance()->GetMessageBloomfilter(message);

    assert(bloomfilter);
    if (!bloomfilter) {
        TOP_WARN2("bloomfilter invalid");
        return;
    }

    // multi-service_type may filtered
    if (message.hop_num() >= message.gossip().ign_bloomfilter_level() && message.hop_num() != 0) {
        bloomfilter->Add(routing_table->get_local_node_info()->hash64());
    }

    std::vector<kadmlia::NodeInfoPtr> select_nodes;
    SelectNodes(message, routing_table, bloomfilter, select_nodes);
    if (select_nodes.empty()) {
        TOP_DEBUG("stop broadcast, select_nodes empty,msg_hash:%u msg_type:%d hop_num:%d",
                message.gossip().msg_hash(),
                message.type(),
                message.hop_num());
        return;
    }

#ifdef TOP_TESTING_PERFORMANCE
    std::string goed_ids;
    for (uint32_t i = 0; i < message.hop_nodes_size(); ++i) {
        goed_ids += HexEncode(message.hop_nodes(i).node_id()) + ",";
    }
    std::string data = base::StringUtil::str_fmt(
            "GossipBloomfilterLayer Broadcast select_node size %d,"
            "filtered %d nodes [hop num: %d][%s][%s]",
            select_nodes.size(),
            filtered,
            message.hop_num(),
            goed_ids.c_str(),
            bloomfilter->string().c_str());
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE(data, message);
#endif

    if ((message.hop_num() + 1) > message.gossip().ign_bloomfilter_level()) {
        for (auto iter = select_nodes.begin();
                iter != select_nodes.end(); ++iter) {
            bloomfilter->Add((*iter)->hash64);
        }
    }
    const std::vector<uint64_t>& bloomfilter_vec = bloomfilter->Uint64Vector();
    message.clear_bloomfilter();
    for (uint32_t i = 0; i < bloomfilter_vec.size(); ++i) {
        message.add_bloomfilter(bloomfilter_vec[i]);
    }

    auto gossip_switch_layer_hop_num = kGossipSwitchLayerHopNum;
    if (message.gossip().switch_layer_hop_num() > 0) {
        gossip_switch_layer_hop_num = message.gossip().switch_layer_hop_num();
    }
    if (message.hop_num() >= gossip_switch_layer_hop_num) {
        SendLayered(message, select_nodes);
    } else {
        Send(message, select_nodes);
    }
}

}  // namespace gossip

}  // namespace top
