// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgossip/include/gossip_dispatcher.h"

#include "xgossip/include/gossip_utils.h"
#include "xgossip/include/mesages_with_bloomfilter.h"
#include "xpbase/base/kad_key/get_kadmlia_key.h"

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

void GossipDispatcher::Broadcast(uint64_t local_hash64, transport::protobuf::RoutingMessage & message, std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> neighbors) {
    return;
}
void GossipDispatcher::Broadcast(transport::protobuf::RoutingMessage & message, kadmlia::ElectRoutingTablePtr & routing_table) {
    CheckDiffNetwork(message);

    auto gossip_max_hop_num = kGossipDefaultMaxHopNum;
    if (message.gossip().max_hop_num() > 0) {
        gossip_max_hop_num = message.gossip().max_hop_num();
    }
    if (gossip_max_hop_num <= message.hop_num()) {
        xdbg("msg_hash:%u msg_type:%d hop_num:%d larger than gossip_max_hop_num:%d", message.gossip().msg_hash(), message.type(), message.hop_num(), gossip_max_hop_num);
        return;
    }

    auto kad_key_ptr = base::GetKadmliaKey(message.des_node_id());
    base::ServiceType service_type = kad_key_ptr->GetServiceType();
    MessageKey msg_key(0, message.gossip().msg_hash(), service_type);
    if (MessageWithBloomfilter::Instance()->StopGossip(msg_key, 1)) {
        xkinfo("[debug] stop gossip but got % " PRIu64 " % " PRIu64 ":", message.gossip().min_dis(), message.gossip().max_dis());
        xdbg("stop gossip for msg_hash:%u msg_type:%d stop_times:%d", message.gossip().msg_hash(), message.type(), 1);
        return;
    }

    std::vector<gossip::DispatchInfos> select_nodes;
    GenerateDispatchInfos(message, routing_table, select_nodes);
    if (select_nodes.empty()) {
        xdbg("stop broadcast, select_nodes empty,msg_hash:%u msg_type:%d hop_num:%d", message.gossip().msg_hash(), message.type(), message.hop_num());
        return;
    }

    // auto gossip_switch_layer_hop_num = kGossipLayerSwitchLayerHopNum;

    SendDispatch(message, select_nodes);
}

void GossipDispatcher::GenerateDispatchInfos(transport::protobuf::RoutingMessage & message, kadmlia::ElectRoutingTablePtr & routing_table, std::vector<DispatchInfos> & select_nodes) {
    uint64_t sit1 = message.gossip().min_dis();
    uint64_t sit2 = message.gossip().max_dis();
    xkinfo("[debug] got % " PRIu64 " % " PRIu64 ":", sit1, sit2);

    uint32_t overlap = message.gossip().left_overlap();

    static std::random_device rd;
    static std::mt19937 g(rd());

    auto const & nodes_map = routing_table->nodes();
    std::map<std::string, std::size_t> index_map;
    std::vector<std::string> xip2_for_shuffle;
    std::size_t index = 0;
    for (auto _p : nodes_map) {
        xip2_for_shuffle.push_back(_p.first);
        index_map.insert(std::make_pair(_p.first, index));
        index++;
    }

    if (message.hop_num() == 0) {
        SET_INDEX_SENT(index_map.at(routing_table->get_local_node_info()->id()), sit1, sit2);
    }
    std::shuffle(xip2_for_shuffle.begin(), xip2_for_shuffle.end(), g);
    std::size_t shuffle_i;
    std::vector<std::string> select_xip2;
    for (shuffle_i = 0; shuffle_i < xip2_for_shuffle.size(); ++shuffle_i) {
        std::size_t node_index = index_map[xip2_for_shuffle[shuffle_i]];
        if (IS_INDEX_SENT(node_index, sit1, sit2))
            continue;
        SET_INDEX_SENT(node_index, sit1, sit2);
        select_xip2.push_back(xip2_for_shuffle[shuffle_i]);
        if (select_xip2.size() > kGossipLayerNeighborNum)
            break;
    }
    for (auto _xip2 : select_xip2) {
        select_nodes.push_back(DispatchInfos(nodes_map.at(_xip2), sit1, sit2));
    }
    if (select_nodes.empty())
        return;

    for (std::size_t _index = 0; _index < select_nodes.size(); ++_index) {
        xkinfo("[debug] tmp_before % " PRIu64 " % " PRIu64 ":", select_nodes[_index].sit1, select_nodes[_index].sit2);
    }

    for (; shuffle_i < xip2_for_shuffle.size(); ++shuffle_i) {
        std::size_t node_index = index_map[xip2_for_shuffle[shuffle_i]];
        if (IS_INDEX_SENT(node_index, sit1, sit2))
            continue;
        static uint32_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        static std::mt19937 rng(seed);
        std::size_t send_node_index = rng() % select_nodes.size();
        // std::size_t overlap_flag = rng() % 3;
        for (std::size_t _index = 0; _index < select_nodes.size(); ++_index) {
            if (_index != send_node_index /*&& overlap_flag*/) {
                SET_INDEX_SENT(node_index, select_nodes[_index].get_sit1(), select_nodes[_index].get_sit2());
            }
        }
    }

    for (std::size_t _index = 0; _index < select_nodes.size(); ++_index) {
        xkinfo("[debug] tmp_after % " PRIu64 " % " PRIu64 ":", select_nodes[_index].sit1, select_nodes[_index].sit2);
    }

#if 0

    std::vector<kadmlia::NodeInfoPtr> nodes_for_shuffle;
    for (auto _p : nodes_map) {
        nodes_for_shuffle.push_back(_p.second);
    }
    //  std::vector<kadmlia::NodeInfoPtr> nodes_for_shuffle = routing_table->nodes();
    // static std::vector<uint64_t> sorted_nodes = routing_table->get_all_sorted_nodes();
     std::map<uint64_t, std::size_t> hash2index = routing_table->get_all_sorted_nodes();

    if (message.hop_num() == 0) {
        SET_INDEX_SENT(hash2index.at(routing_table->get_local_node_info()->hash64()), sit1, sit2);
    }

    std::shuffle(nodes_for_shuffle.begin(), nodes_for_shuffle.end(), g);

    std::size_t shuffle_i;
    std::vector<std::size_t> selected_shuffle_index;
    for (shuffle_i = 0; shuffle_i < nodes_for_shuffle.size(); ++shuffle_i) {
        auto node_index = hash2index[nodes_for_shuffle[shuffle_i]->hash64];
        // if(nodes_for_shuffle[i].recv_flag) continue;
        if (IS_INDEX_SENT(node_index, sit1, sit2))
            continue;
        SET_INDEX_SENT(node_index, sit1, sit2);
        selected_shuffle_index.push_back(shuffle_i);
        if (selected_shuffle_index.size() >= kGossipLayerNeighborNum)
            break;
    }

    for (auto _index : selected_shuffle_index) {
        select_nodes.push_back(DispatchInfos(nodes_for_shuffle[_index], sit1, sit2));
    }
    if (select_nodes.empty())
        return;

    for(std::size_t _index = 0;_index <select_nodes.size();++_index){
        xkinfo("[debug] tmp_before % " PRIu64 " % " PRIu64 ":", select_nodes[_index].sit1, select_nodes[_index].sit2);
    }

    for (; shuffle_i < nodes_for_shuffle.size(); ++shuffle_i) {
        auto node_index = hash2index[nodes_for_shuffle[shuffle_i]->hash64];
        if (IS_INDEX_SENT(node_index, sit1, sit2))
            continue;
        static uint32_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        static std::mt19937 rng(seed);
        std::size_t send_node_index = rng() % select_nodes.size();
        // std::size_t overlap_flag = rng() % 3;
        for (std::size_t _index = 0; _index < select_nodes.size(); ++_index) {
            if (_index != send_node_index /*&& overlap_flag*/) {
                SET_INDEX_SENT(node_index, select_nodes[_index].get_sit1(), select_nodes[_index].get_sit2());
            }
        }
    }
    
    for(std::size_t _index = 0;_index <select_nodes.size();++_index){
        xkinfo("[debug] tmp_after % " PRIu64 " % " PRIu64 ":", select_nodes[_index].sit1, select_nodes[_index].sit2);
    }
#endif
    return;
}

}  // namespace gossip
}  // namespace top
