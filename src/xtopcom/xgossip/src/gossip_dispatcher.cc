// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgossip/include/gossip_dispatcher.h"

#include "xgossip/include/gossip_utils.h"
#include "xgossip/include/mesages_with_bloomfilter.h"
#include "xpbase/base/kad_key/kadmlia_key.h"

#include <cinttypes>

namespace top {
namespace gossip {

GossipDispatcher::GossipDispatcher(transport::TransportPtr transport_ptr) : GossipInterface{transport_ptr} {
}
GossipDispatcher::~GossipDispatcher() {
}

inline bool IS_INDEX_SENT(std::size_t node_index, uint64_t s1, uint64_t s2) {
    // if (node_index > 64) {
    //     return ((s2 >> (node_index - 65)) & 0x01) == 0x01 ? true : false;
    // } else {
    //     return ((s1 >> (node_index - 1)) & 0x01) == 0x01 ? true : false;
    // }
    return node_index > 64 ? (((s2 >> (node_index - 65)) & 0x01) == 0x01 ? true : false) : (((s1 >> (node_index - 1)) & 0x01) == 0x01 ? true : false);
}

inline void SET_INDEX_SENT(std::size_t node_index, uint64_t & s1, uint64_t & s2) {
    if (node_index > 64) {
        s2 |= ((uint64_t)0x01 << (node_index - 65));
    } else {
        s1 |= ((uint64_t)0x01 << (node_index - 1));
    }
}

void GossipDispatcher::Broadcast(transport::protobuf::RoutingMessage & message, kadmlia::ElectRoutingTablePtr & routing_table) {
    CheckDiffNetwork(message);

    auto gossip_max_hop_num = kGossipDefaultMaxHopNum;
    if (message.gossip().max_hop_num() > 0) {
        gossip_max_hop_num = message.gossip().max_hop_num();
    }
    if (gossip_max_hop_num <= message.hop_num()) {
        xdbg("msg_hash:%u msg_type:%d hop_num:%d larger than gossip_max_hop_num:%d", message.msg_hash(), message.type(), message.hop_num(), gossip_max_hop_num);
        return;
    }

    auto kad_key_ptr = base::GetKadmliaKey(message.des_node_id());

    std::vector<gossip::DispatchInfos> select_nodes;
    GenerateDispatchInfos(message, routing_table, select_nodes);
    if (select_nodes.empty()) {
        xdbg("stop broadcast, select_nodes empty,msg_hash:%u msg_type:%d hop_num:%d", message.msg_hash(), message.type(), message.hop_num());
        return;
    }

    SendDispatch(message, select_nodes);
}

void GossipDispatcher::GenerateDispatchInfos(transport::protobuf::RoutingMessage & message,
                                             kadmlia::ElectRoutingTablePtr & routing_table,
                                             std::vector<DispatchInfos> & select_nodes) {
    uint64_t sit1 = message.gossip().sit1();
    uint64_t sit2 = message.gossip().sit2();
    xdbg("[GossipDispatcher::GenerateDispatchInfos] got % " PRIu64 " % " PRIu64 ":", sit1, sit2);

    MessageKey msg_key(message.msg_hash());
    MessageWithBloomfilter::Instance()->UpdateHandle(msg_key, sit1, sit2);

    xdbg("[GossipDispatcher::GenerateDispatchInfos] after update % " PRIu64 " % " PRIu64 ":", sit1, sit2);

    uint32_t overlap = message.gossip().overlap_rate();

    std::unordered_map<std::string, kadmlia::NodeInfoPtr> const nodes_map = routing_table->nodes();
    std::unordered_map<std::string, std::size_t> const index_map = routing_table->index_map();
    std::vector<std::string> const xip2_for_shuffle = routing_table->get_shuffled_xip2();

    SET_INDEX_SENT(routing_table->get_self_index(), sit1, sit2);
    std::size_t index_i;
    std::vector<std::string> select_xip2;
    for (index_i = 0; index_i < xip2_for_shuffle.size(); ++index_i) {
        if (index_map.find(xip2_for_shuffle[index_i]) != index_map.end()) {
            std::size_t node_index = index_map.at(xip2_for_shuffle[index_i]);
            if (IS_INDEX_SENT(node_index, sit1, sit2))
                continue;
            SET_INDEX_SENT(node_index, sit1, sit2);
            select_xip2.push_back(xip2_for_shuffle[index_i]);
            if (select_xip2.size() > kGossipLayerNeighborNum)
                break;
        } else {
            xwarn("[GossipDispatcher::GenerateDispatchInfos] routing table still uncomplete , missing: %s", xip2_for_shuffle[index_i].c_str());
        }
    }
    for (auto _xip2 : select_xip2) {
        select_nodes.push_back(DispatchInfos(nodes_map.at(_xip2), sit1, sit2));
    }
    if (select_nodes.empty())
        return;

    for (std::size_t _index = 0; _index < select_nodes.size(); ++_index) {
        xdbg("[GossipDispatcher::GenerateDispatchInfos] before % " PRIu64 " % " PRIu64 ":", select_nodes[_index].sit1, select_nodes[_index].sit2);
    }

    for (; index_i < xip2_for_shuffle.size(); ++index_i) {
        if (index_map.find(xip2_for_shuffle[index_i]) != index_map.end()) {
            std::size_t node_index = index_map.at(xip2_for_shuffle[index_i]);
            if (IS_INDEX_SENT(node_index, sit1, sit2))
                continue;
            static uint32_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
            static std::mt19937 rng(seed);
            std::size_t send_node_index = rng() % select_nodes.size();
            for (std::size_t _index = 0; _index < select_nodes.size(); ++_index) {
                if ((_index != send_node_index && (rng() % overlap))) {
                    SET_INDEX_SENT(node_index, select_nodes[_index].get_sit1(), select_nodes[_index].get_sit2());
                }
            }
        } else {
            xwarn("[GossipDispatcher::GenerateDispatchInfos] routing table still uncomplete , missing: %s", xip2_for_shuffle[index_i].c_str());
        }
    }

    for (std::size_t _index = 0; _index < select_nodes.size(); ++_index) {
        xdbg("[GossipDispatcher::GenerateDispatchInfos] after % " PRIu64 " % " PRIu64 ":", select_nodes[_index].sit1, select_nodes[_index].sit2);
    }

    return;
}

}  // namespace gossip
}  // namespace top
