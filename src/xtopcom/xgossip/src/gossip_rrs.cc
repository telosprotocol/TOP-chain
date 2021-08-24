// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgossip/include/gossip_rrs.h"

#include "xgossip/include/block_sync_manager.h"
#include "xgossip/include/gossip_utils.h"
#include "xgossip/include/mesages_with_bloomfilter.h"
// #include "xwrouter/multi_routing/service_node_cache.h"

namespace top {
namespace gossip {

GossipRRS::GossipRRS(transport::TransportPtr transport_ptr) : GossipInterface{transport_ptr} {
}
GossipRRS::~GossipRRS() {
}

void GossipRRS::Broadcast(uint64_t local_hash64, transport::protobuf::RoutingMessage & message, std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> prt_neighbor) {
    auto neighbors = *prt_neighbor;
    auto const hop_num = message.hop_num();
    BlockSyncManager::Instance()->NewBroadcastMessage(message);
    auto gossip_max_hop_num = kGossipDefaultMaxHopNum;
    if (message.gossip().max_hop_num() > 0) {
        gossip_max_hop_num = message.gossip().max_hop_num();
    }
    if (gossip_max_hop_num <= message.hop_num()) {
        xkinfo("[GossipRRS]message.type(%d) hop_num(%d) larger than gossip_max_hop_num(%d)", message.type(), hop_num, gossip_max_hop_num);
        return;
    }

    MessageKey msg_key(message.msg_hash());
    if (MessageWithBloomfilter::Instance()->StopGossip(msg_key, kGossipRRSStopTimes)) {
        xkinfo("[GossipRRS]stop gossip for message.type(%d) stop_time(%d),hop_num(%d)", message.type(), kGossipRRSStopTimes, hop_num);
        return;
    }

    if ((message.has_data() || message.gossip().has_block()) && hop_num < kGossipRRSSwitchLayerHopNum) {
        auto bloomfilter = MessageWithBloomfilter::Instance()->GetMessageBloomfilter(message);
        assert(bloomfilter);
        if (!bloomfilter) {
            return;
        }
        bloomfilter->Add(local_hash64);

        std::vector<kadmlia::NodeInfoPtr> tmp_neighbors;
        tmp_neighbors = GetRandomNodes(neighbors, kGossipRRSNeighborNum);

        std::vector<kadmlia::NodeInfoPtr> rest_random_neighbors;

        for (auto iter = tmp_neighbors.begin(); iter != tmp_neighbors.end(); ++iter) {
            if ((*iter)->hash64 == 0) {
                xkinfo("[GossipRRS]node:%s hash64 empty, invalid", HexEncode((*iter)->node_id).c_str());
                continue;
            }

            if (bloomfilter->Contain((*iter)->hash64)) {
                continue;
            }

            rest_random_neighbors.push_back(*iter);
        }

        if (rest_random_neighbors.empty()) {
            xkinfo("[GossipRRS]stop Broadcast, rest_random_neighbors empty, broadcast failed, msg.hop_num(%d), msg.type(%d)", message.hop_num(), message.type());
            return;
        }

        if (message.hop_num() > kGossipRRSBloomfilterIgnoreLevel) {
            for (auto iter = rest_random_neighbors.begin(); iter != rest_random_neighbors.end(); ++iter) {
                bloomfilter->Add((*iter)->hash64);
            }
        }

        const std::vector<uint64_t> & bloomfilter_vec = bloomfilter->Uint64Vector();
        message.clear_bloomfilter();
        for (uint32_t i = 0; i < bloomfilter_vec.size(); ++i) {
            message.add_bloomfilter(bloomfilter_vec[i]);
        }

        xkinfo("[GossipRRS][do_send] Broadcast %d neighbors msg header_hash:%s", rest_random_neighbors.size(), message.gossip().header_hash().c_str());
        if (message.has_is_root() && message.is_root()) {
            MutableSend(message, rest_random_neighbors);
        } else {
            Send(message, rest_random_neighbors);
        }
    } else {
        BroadcastHash(message, neighbors);
    }
}

void GossipRRS::BroadcastHash(transport::protobuf::RoutingMessage & message, std::vector<kadmlia::NodeInfoPtr> & neighbors) {
    transport::protobuf::RoutingMessage header_message(message);
    if (header_message.has_data() || header_message.gossip().has_block()) {
        // have not been cleared yet.
        auto gossip_header = header_message.mutable_gossip();
        gossip_header->clear_block();
        header_message.clear_msg_hash();
        header_message.clear_data();
        header_message.clear_bloomfilter();
    }
    std::vector<kadmlia::NodeInfoPtr> random_neighbors;
    random_neighbors = GetRandomNodes(neighbors, kGossipRRSNeighborNum);
    xkinfo("[GossipRRS][do_send] Broadcast %d neighbors only header_hash:%s", random_neighbors.size(), header_message.gossip().header_hash().c_str());
    if (header_message.has_is_root() && header_message.is_root()) {
        header_message.clear_src_node_id();
        header_message.clear_des_node_id();
        MutableSendHash(header_message, random_neighbors);
    } else {
        Send(header_message, random_neighbors);
    }
}

}  // namespace gossip
}  // namespace top