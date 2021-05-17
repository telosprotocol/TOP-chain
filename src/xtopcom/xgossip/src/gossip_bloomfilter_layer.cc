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
    // BlockSyncManager::Instance()->NewBroadcastMessage(message);
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
    if (MessageWithBloomfilter::Instance()->StopGossip(msg_key, kGossipLayerStopTimes)) {
        TOP_DEBUG("stop gossip for msg_hash:%u msg_type:%d stop_times:%d", message.gossip().msg_hash(), message.type(), kGossipLayerStopTimes);
        return;
    }
    auto bloomfilter = MessageWithBloomfilter::Instance()->GetMessageBloomfilter(message);

    assert(bloomfilter);
    if (!bloomfilter) {
        TOP_WARN2("bloomfilter invalid");
        return;
    }

    // multi-service_type may filtered
    if (message.hop_num() >= kGossipLayerBloomfilterIgnoreLevel && message.hop_num() != 0) {
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

    if ((message.hop_num() + 1) > kGossipLayerBloomfilterIgnoreLevel) {
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

    auto gossip_switch_layer_hop_num = kGossipLayerSwitchLayerHopNum;
    // if (message.gossip().switch_layer_hop_num() > 0) {
    //     gossip_switch_layer_hop_num = message.gossip().switch_layer_hop_num();
    // }
    if (message.hop_num() >= gossip_switch_layer_hop_num) {
        SendLayered(message, select_nodes);
    } else {
        Send(message, select_nodes);
    }
}

void GossipBloomfilterLayer::SelectNodes(transport::protobuf::RoutingMessage & message,
                                         kadmlia::RoutingTablePtr & routing_table,
                                         std::shared_ptr<base::Uint64BloomFilter> & bloomfilter,
                                         std::vector<kadmlia::NodeInfoPtr> & select_nodes) {
    uint64_t min_dis = message.gossip().min_dis();
    uint64_t max_dis = message.gossip().max_dis();
    if (max_dis <= 0) {
        max_dis = std::numeric_limits<uint64_t>::max();
    }
    uint64_t left_min = message.gossip().left_min();
    uint64_t right_max = message.gossip().right_max();
    uint32_t left_overlap = message.gossip().left_overlap();
    uint32_t right_overlap = message.gossip().right_overlap();
    if (left_overlap > 0 && left_overlap < 20) {
        uint64_t tmp_min_dis = min_dis;
        double rate = (double)left_overlap / 10;
        uint64_t step = static_cast<uint64_t>((tmp_min_dis - left_min) * rate);
        if (step > tmp_min_dis) {
            min_dis = 0;
        } else {
            min_dis = tmp_min_dis - step;
        }
    }
    if (right_overlap > 0 && right_overlap < 20) {
        uint64_t tmp_max_dis = max_dis;
        double rate = (double)right_overlap / 10;
        uint64_t step = static_cast<uint64_t>((right_max - max_dis) * rate);
        if (std::numeric_limits<uint64_t>::max() - step > tmp_max_dis) {
            max_dis = tmp_max_dis + step;
        } else {
            max_dis = std::numeric_limits<uint64_t>::max();
        }
    }

    std::vector<kadmlia::NodeInfoPtr> nodes;
    routing_table->GetRangeNodes(min_dis, max_dis, nodes);
    if (nodes.empty()) {
        TOP_WARN("getrangenodes empty");
        return;
    }

    TOP_DEBUG("selectnodes hop_num:%u min_dis:%llu max_dis:%llu size:%u", message.hop_num(), min_dis, max_dis, nodes.size());

    uint32_t filtered = 0;
    std::random_shuffle(nodes.begin(), nodes.end());
    for (auto iter = nodes.begin(); iter != nodes.end(); ++iter) {
        if (select_nodes.size() >= kGossipLayerNeighborNum) {
            break;
        }

        if ((*iter)->hash64 == 0) {
            continue;
        }

        if (bloomfilter->Contain((*iter)->hash64)) {
            ++filtered;
            continue;
        }

        select_nodes.push_back(*iter);
    }
    std::sort(select_nodes.begin(), select_nodes.end(), [](const kadmlia::NodeInfoPtr & left, const kadmlia::NodeInfoPtr & right) -> bool { return left->hash64 < right->hash64; });
}

}  // namespace gossip

}  // namespace top
