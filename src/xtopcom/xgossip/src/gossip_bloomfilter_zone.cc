// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#include "xgossip/include/gossip_bloomfilter_zone.h"

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
#include "xwrouter/register_routing_table.h"

namespace top {

namespace gossip {

static const int kCleanTimerInterval = 10 * 1000 * 1000;  // 10s
static const int kCleanItemTimeout = 30;  // 30s

GossipBloomfilterZone::GossipBloomfilterZone(transport::TransportPtr transport_ptr)
        : GossipInterface(transport_ptr) {}

GossipBloomfilterZone::~GossipBloomfilterZone() {
}
// get 4 nodes of different zones
int GossipBloomfilterZone::GetGossipNodes(const std::shared_ptr<base::Uint64BloomFilter>& bloomfilter,
        const std::vector<kadmlia::NodeInfoPtr>& neighbors,
        std::vector<kadmlia::NodeInfoPtr>& tmp_neighbors)
{
    int zone = 0;
    for (int i = 0; i < 4; i++) {
        int pushed = 0;
        for (auto iter = neighbors.begin(); iter != neighbors.end(); ++iter) {
            if ((*iter)->hash64 == 0) {
                TOP_WARN("node:%s hash64 empty, invalid", HexEncode((*iter)->node_id).c_str());
                continue;
            }

            uint8_t first_char = (*iter)->node_id[3];  // start from third byte
            first_char = first_char >> 6;
            if (first_char == i) {
                tmp_neighbors.push_back(*iter);
                pushed = 1;
                break;
            }
        }
        if (pushed == 0)
            zone = i;
    }
    return zone;
}
// get nodes within current zone
// node id start from third byte
int GossipBloomfilterZone::GetZoneNodes(const std::shared_ptr<base::Uint64BloomFilter>& bloomfilter,
        const std::vector<kadmlia::NodeInfoPtr>& neighbors,
        std::vector<kadmlia::NodeInfoPtr>& tmp_neighbors,
        const transport::protobuf::RoutingMessage& message)
{
    auto routing_table = wrouter::GetRoutingTable(kRoot, true);
    top::kadmlia::LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    
    uint8_t current_group = local_node->id()[3];  // start from third byte
    current_group = current_group >> 6; 
    for (auto iter = neighbors.begin(); iter != neighbors.end(); ++iter) {
        if ((*iter)->hash64 == 0) {
            TOP_WARN("node:%s hash64 empty, invalid", HexEncode((*iter)->node_id).c_str());
            continue;
        }
        uint8_t first_char = (*iter)->node_id[3];
        first_char = first_char >> 6;
        if (first_char != current_group)
            continue;

        if (bloomfilter->Contain((*iter)->hash64 ^ message.gossip().msg_hash())) {
            continue;
        }

        tmp_neighbors.push_back(*iter);
    }
    TOP_DEBUG("after filter, all size:%d, remain:%d", neighbors.size(), tmp_neighbors.size());
    return tmp_neighbors.size();
}
int GossipBloomfilterZone::SendMessage(const std::shared_ptr<base::Uint64BloomFilter>& bloomfilter,
        transport::protobuf::RoutingMessage& message,
        std::vector<kadmlia::NodeInfoPtr>& rest_random_neighbors) {
    // add sending nodes to bf
    for (auto iter = rest_random_neighbors.begin(); iter != rest_random_neighbors.end(); ++iter) {
        bloomfilter->Add((*iter)->hash64 ^ message.gossip().msg_hash());
    }

    const std::vector<uint64_t>& bloomfilter_vec = bloomfilter->Uint64Vector();
    message.clear_bloomfilter();
    for (uint32_t i = 0; i < bloomfilter_vec.size(); ++i) {
        message.add_bloomfilter(bloomfilter_vec[i]);
    }

    std::string bf_str;
    for (uint32_t i = 0; i < bloomfilter_vec.size(); i++)
        bf_str += std::string((char*)&bloomfilter_vec[i], sizeof(uint64_t));
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_gossip_map[message.gossip().msg_hash()] = GossipInfo {
                std::chrono::steady_clock::now(), bf_str};
    }

    TOP_DEBUG("send %d neighbors, bf:%s", rest_random_neighbors.size(), HexEncode(bf_str).c_str());
    if (message.has_is_root() && message.is_root()) {
        MutableSend(message, rest_random_neighbors);
        TOP_DEBUG("send gossip of kRoot");
    } else {
        Send(message, rest_random_neighbors);
    }

    TOP_NETWORK_DEBUG_FOR_REDIS(message, "hop_num");
    return 0;
}
// send to four zone nodes
int GossipBloomfilterZone::SendZoneMessage(const std::shared_ptr<base::Uint64BloomFilter>& bloomfilter,
        transport::protobuf::RoutingMessage& message,
        std::vector<kadmlia::NodeInfoPtr>& rest_random_neighbors) {
    // add sending nodes to bf
    auto routing_table = wrouter::GetRoutingTable(kRoot, true);
    top::kadmlia::LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    uint8_t current_group = local_node->id()[3];
    current_group = current_group >> 6; 
    for (auto iter = rest_random_neighbors.begin(); iter != rest_random_neighbors.end(); ++iter) {
        base::Uint64BloomFilter bloomfilter_new(*bloomfilter);
        uint8_t first_char = (*iter)->node_id[3];
        first_char = first_char >> 6;
        if (first_char == current_group)
            bloomfilter_new.Add(base::xhash64_t::digest(global_xid->Get()) ^ message.gossip().msg_hash());

        const std::vector<uint64_t>& bloomfilter_vec = bloomfilter_new.Uint64Vector();
        message.clear_bloomfilter();
        for (uint32_t i = 0; i < bloomfilter_vec.size(); ++i) {
            message.add_bloomfilter(bloomfilter_vec[i]);
        }

        std::string bf_str;
        for (uint32_t i=0;i<bloomfilter_vec.size();i++) {
            bf_str += std::string((char*)&bloomfilter_vec[i], sizeof(uint64_t));
        }

        TOP_DEBUG("send to next,bf:%s", HexEncode(bf_str).c_str());

        std::vector<kadmlia::NodeInfoPtr> send_nodes;
        send_nodes.push_back(*iter);
        if (message.has_is_root() && message.is_root()) {
            MutableSend(message, send_nodes);
            TOP_DEBUG("send gossip of kRoot");
        } else {
            Send(message, send_nodes);
        }
    }
    TOP_NETWORK_DEBUG_FOR_REDIS(message, "hop_num");
    return 0;
}
// send to three nodes, carry fourth zone info for relay
int GossipBloomfilterZone::SendLessZoneMessage(const std::shared_ptr<base::Uint64BloomFilter>& bloomfilter,
        transport::protobuf::RoutingMessage& message,
        std::vector<kadmlia::NodeInfoPtr>& rest_random_neighbors,
        int zone) {
    // add sending nodes to bf
    auto routing_table = wrouter::GetRoutingTable(kRoot, true);
    top::kadmlia::LocalNodeInfoPtr local_node = routing_table->get_local_node_info();
    uint8_t current_group = local_node->id()[3];
    current_group = current_group >> 6; 
    for (auto iter = rest_random_neighbors.begin(); iter != rest_random_neighbors.end(); ++iter) {
        base::Uint64BloomFilter bloomfilter_new(*bloomfilter);
        uint8_t first_char = (*iter)->node_id[3];
        first_char = first_char >> 6;
        if (first_char == current_group)
            bloomfilter_new.Add(base::xhash64_t::digest(global_xid->Get()) ^ message.gossip().msg_hash());

        const std::vector<uint64_t>& bloomfilter_vec = bloomfilter_new.Uint64Vector();
        message.clear_bloomfilter();
        for (uint32_t i = 0; i < bloomfilter_vec.size(); ++i) {
            message.add_bloomfilter(bloomfilter_vec[i]);
        }

        std::string bf_str;
        for (uint32_t i=0;i<bloomfilter_vec.size();i++) {
            bf_str += std::string((char*)&bloomfilter_vec[i], sizeof(uint64_t));
        }

        TOP_DEBUG("send to next,bf:%s", HexEncode(bf_str).c_str());
        message.mutable_gossip()->clear_switch_layer_hop_num();
        if (first_char >> 1 == zone >> 1) {  // same zone
            message.mutable_gossip()->set_switch_layer_hop_num(zone);
        }
        std::vector<kadmlia::NodeInfoPtr> send_nodes;
        send_nodes.push_back(*iter);
        if (message.has_is_root() && message.is_root()) {
            MutableSend(message, send_nodes);
            TOP_DEBUG("send gossip of kRoot");
        } else {
            Send(message, send_nodes);
        }
    }
    TOP_NETWORK_DEBUG_FOR_REDIS(message, "hop_num");
    return 0;
}
int GossipBloomfilterZone::SendFourthZone(transport::protobuf::RoutingMessage& message,
        std::vector<kadmlia::NodeInfoPtr>& neighbors) {
    transport::protobuf::RoutingMessage& message_new(message);
    for (auto iter = neighbors.begin(); iter != neighbors.end(); ++iter) {
        if ((*iter)->hash64 == 0) {
            TOP_WARN("node:%s hash64 empty, invalid", HexEncode((*iter)->node_id).c_str());
            continue;
        }
        uint8_t first_char = (*iter)->node_id[3];
        first_char = first_char >> 6;
        if (first_char == message_new.gossip().switch_layer_hop_num()) {
            message_new.mutable_gossip()->clear_switch_layer_hop_num();
            message_new.clear_bloomfilter();
            TOP_DEBUG("relay gossip to fourth zone ok:%u", message.gossip().msg_hash());

            std::vector<kadmlia::NodeInfoPtr> send_nodes;
            send_nodes.push_back(*iter);
            if (message_new.has_is_root() && message_new.is_root()) {
                MutableSend(message_new, send_nodes);
                TOP_DEBUG("send gossip of kRoot");
            } else {
                Send(message_new, send_nodes);
            }
            break;
        }
    }

    return 0;
}

void GossipBloomfilterZone::Broadcast(
        uint64_t local_hash64,
        transport::protobuf::RoutingMessage& message,
        std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> prt_neighbors) {
    auto neighbors = *prt_neighbors;
    TOP_NETWORK_DEBUG_FOR_REDIS(message, "recv_count");
    TOP_INFO("gossip zone:%u, size:%d,hop:%d", message.gossip().msg_hash(), neighbors.size(), message.hop_num());
//    std::unique_lock<std::mutex> lock(m_mutex);
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_gossip_map.find(message.gossip().msg_hash()) != m_gossip_map.end()) {
            if (message.hop_num() > 50) {
                TOP_INFO("message.type(%d) hop_num(%d) max_hop_num(%d)", message.type(), message.hop_num(), message.gossip().max_hop_num());
                return;
            }
        } else {
            m_gossip_map[message.gossip().msg_hash()] = GossipInfo{
                    std::chrono::steady_clock::now(), std::string(gossip::kGossipBloomfilterSize/8, (char)0)};
        }
    }

    if (message.hop_num() == 0) {
        message.mutable_gossip()->clear_switch_layer_hop_num();
    }
    if (message.gossip().has_switch_layer_hop_num()) {
        TOP_WARN2("need to relay to fourth zone:%d", message.gossip().switch_layer_hop_num());
        SendFourthZone(message, neighbors);
    }

    std::vector<uint64_t> new_bloomfilter_vec;
    new_bloomfilter_vec.reserve(message.bloomfilter_size());
    for (auto i = 0; i < message.bloomfilter_size(); ++i) {
        new_bloomfilter_vec.push_back(message.bloomfilter(i));
    }
    std::string bf_tmp;
    for (uint32_t i=0;i<new_bloomfilter_vec.size();i++)
        bf_tmp += std::string((char*)&new_bloomfilter_vec[i], sizeof(uint64_t));
    TOP_DEBUG("messeage gossip bf:%s,hop:%d\n", HexEncode(bf_tmp).c_str(), message.hop_num());

    std::shared_ptr<base::Uint64BloomFilter> bloomfilter;
    if (new_bloomfilter_vec.empty()) {
        std::vector<uint64_t> construct_vec(gossip::kGossipBloomfilterSize / 64, 0ull);
        bloomfilter = std::make_shared<base::Uint64BloomFilter>(construct_vec, gossip::kGossipBloomfilterHashNum);
    } else {
        bloomfilter = std::make_shared<base::Uint64BloomFilter>(new_bloomfilter_vec, gossip::kGossipBloomfilterHashNum);
    }

    assert(bloomfilter);
    if (!bloomfilter) {
        TOP_WARN2("bloomfilter invalid");
        return;
    }

    if (message.hop_num() == 0) {  // current node
        std::vector<kadmlia::NodeInfoPtr> tmp_neighbors;

        int zone = GetGossipNodes(bloomfilter, neighbors, tmp_neighbors);
        if (tmp_neighbors.size() >= 4) {  // send to 4 different zone
            TOP_DEBUG("using gossip zone,msg_id:%d", message.gossip().msg_hash());
            SendZoneMessage(bloomfilter, message, tmp_neighbors);
        } else if (tmp_neighbors.size() == 3) {  // need to relay to the fourth zone, rarely happen
            TOP_DEBUG("using gossip zone, relay to fourth zone, msg_id:%d", message.gossip().msg_hash());
            SendLessZoneMessage(bloomfilter, message, tmp_neighbors, zone);
        } else {
            TOP_WARN("not enough nodes to gossip.");
        }
        return;
    }

    // not current node 
    bloomfilter->Add(base::xhash64_t::digest(global_xid->Get()) ^ message.gossip().msg_hash());
//  auto routing_table = wrouter::GetRoutingTable(kRoot, true);
//    top::kadmlia::LocalNodeInfoPtr local_node = routing_table->get_local_node_info();

    std::string merge_bf;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        merge_bf = m_gossip_map[message.gossip().msg_hash()].bloomfilter;
    }
    TOP_DEBUG("local gossip bf:%s,hop:%d\n", HexEncode(merge_bf).c_str(), message.hop_num());

    std::vector<uint64_t> local_bloomfilter_vec;
    for (uint32_t i = 0; i < gossip::kGossipBloomfilterSize/64; ++i) {
        local_bloomfilter_vec.push_back(*(uint64_t*)(merge_bf.c_str() + 8*i));
    }
    bloomfilter->MergeMore(local_bloomfilter_vec);

    // print merged bf
    {
        const std::vector<uint64_t>& bloomfilter_vec = bloomfilter->Uint64Vector();
        bf_tmp.clear();
        for (uint32_t i=0;i<bloomfilter_vec.size();i++)
            bf_tmp += std::string((char*)&bloomfilter_vec[i], sizeof(uint64_t));
        TOP_DEBUG("merged gossip bf:%s,hop:%d\n", HexEncode(bf_tmp).c_str(), message.hop_num());
    }

    std::vector<kadmlia::NodeInfoPtr> tmp_neighbors;
    GetZoneNodes(bloomfilter, neighbors, tmp_neighbors, message);

    std::vector<kadmlia::NodeInfoPtr> rest_random_neighbors;
    rest_random_neighbors = GetRandomNodes(tmp_neighbors, GetNeighborCount(message));
    if (rest_random_neighbors.empty()) {
        TOP_INFO("stop Broadcast, msg.hop_num(%d), msg.type(%d)", message.hop_num(), message.type());
        const std::vector<uint64_t>& bloomfilter_vec = bloomfilter->Uint64Vector();
        std::string bf_str;
        for (uint32_t i=0;i<bloomfilter_vec.size();i++)
            bf_str += std::string((char*)&bloomfilter_vec[i], sizeof(uint64_t));

        std::unique_lock<std::mutex> lock(m_mutex);
        m_gossip_map[message.gossip().msg_hash()] = GossipInfo {
                std::chrono::steady_clock::now(), bf_str};
        return;
    }

    SendMessage(bloomfilter, message, rest_random_neighbors);

}

void GossipBloomfilterZone::BroadcastWithNoFilter(
        const std::string& local_id,
        transport::protobuf::RoutingMessage& message,
        const std::vector<kadmlia::NodeInfoPtr>& neighbors) {
    TOP_DEBUG("GossipBloomfilter Broadcast neighbors size %d", neighbors.size());

    BlockSyncManager::Instance()->NewBroadcastMessage(message);
    if (message.gossip().max_hop_num() > 0 &&
            message.gossip().max_hop_num() <= message.hop_num()) {
        TOP_INFO("message.type(%d) hop_num(%d) larger than gossip_max_hop_num(%d)",
                message.type(),
                message.hop_num(),
                message.gossip().max_hop_num());
        return;
    }

    if (ThisNodeIsEvil(message)) {
        TOP_WARN2("this node(%s) is evil", HexEncode(local_id).c_str());
        return;
    }
    Send(message, neighbors);
}

void GossipBloomfilterZone::CleanTimerProc() {
    std::unique_lock<std::mutex> lock(m_mutex);
    auto tp_now = std::chrono::steady_clock::now();
    auto it = m_gossip_map.begin();
    while (it != m_gossip_map.end()) {
        if (tp_now - it->second.timepoint > std::chrono::seconds(kCleanItemTimeout)) {
            it = m_gossip_map.erase(it);
            continue;
        } else {
            ++it;
        }
    }
}

void GossipBloomfilterZone::Start() {
    auto timer_proc = std::bind(&GossipBloomfilterZone::CleanTimerProc, this);
    m_clean_timer.Start(kCleanTimerInterval, kCleanTimerInterval, timer_proc);
}

}  // namespace gossip

}  // namespace top
#endif