// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#include "xgossip/include/gossip_bloomfilter_merge.h"

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

namespace top {

namespace gossip {

static const int kCleanTimerInterval = 10 * 1000 * 1000;  // 10s
static const int kCleanItemTimeout = 30;  // 30s

GossipBloomfilterMerge::GossipBloomfilterMerge(transport::TransportPtr transport_ptr)
        : GossipInterface(transport_ptr) {}

GossipBloomfilterMerge::~GossipBloomfilterMerge() {
    //assert(0);
}

void GossipBloomfilterMerge::Broadcast(
        uint64_t local_hash64,
        transport::protobuf::RoutingMessage& message,
        std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> prt_neighbors) {
    auto neighbors = *prt_neighbors;
    TOP_NETWORK_DEBUG_FOR_REDIS(message, "recv_count");

    TOP_DEBUG("GossipBloomfilter Broadcast neighbors size %d", neighbors.size());

//    BlockSyncManager::Instance()->NewBroadcastMessage(message);
    std::unique_lock<std::mutex> lock(m_mutex);
    {
//        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_gossip_map.find(message.gossip().msg_hash()) != m_gossip_map.end()) {
            if (message.hop_num() > 50) {
                TOP_WARN("message.type(%d) hop_num(%d) beyond max_hop", message.type(), message.hop_num());
                return;
            }
        } else {
            m_gossip_map[message.gossip().msg_hash()] = GossipInfo{
                    std::chrono::steady_clock::now(), std::string(gossip::kGossipBloomfilterSize/8, (char)0)};
        }
    }

    if (ThisNodeIsEvil(message)) {
        TOP_WARN2("this node(%s) is evil", HexEncode(global_xid->Get()).c_str());
        return;
    }
/*    bool stop_gossip = false;
    auto bloomfilter = MessageWithBloomfilter::Instance()->GetMessageBloomfilter(
            message,
            stop_gossip);
    if (stop_gossip) {
        TOP_NETWORK_DEBUG_FOR_REDIS(message, "hop_num");
        TOP_DEBUG("stop gossip for message.type(%d) hop_num(%d)", message.type(), message.hop_num());
        return;
    }*/
    std::vector<uint64_t> new_bloomfilter_vec;
    new_bloomfilter_vec.reserve(message.bloomfilter_size());
    for (auto i = 0; i < message.bloomfilter_size(); ++i) {
        new_bloomfilter_vec.push_back(message.bloomfilter(i));
    }
    std::string bf_tmp;
    for (uint32_t i=0;i<new_bloomfilter_vec.size();i++) {
        bf_tmp += std::string((char*)&new_bloomfilter_vec[i], sizeof(uint64_t));
    }
    TOP_DEBUG("messeage gossip bf:%s,hop:%d\n", HexEncode(bf_tmp).c_str(), message.hop_num());

    std::shared_ptr<base::Uint64BloomFilter> bloomfilter;
    if (new_bloomfilter_vec.empty()) {
        // readonly, mark static can improve performance
        std::vector<uint64_t> construct_vec(gossip::kGossipBloomfilterSize / 64, 0ull);
        bloomfilter = std::make_shared<base::Uint64BloomFilter>(
                construct_vec,
                gossip::kGossipBloomfilterHashNum);
    } else {
        bloomfilter = std::make_shared<base::Uint64BloomFilter>(
                new_bloomfilter_vec,
                gossip::kGossipBloomfilterHashNum);
    }

    assert(bloomfilter);
    if (!bloomfilter) {
        TOP_WARN2("bloomfilter invalid");
        return;
    }
    bloomfilter->Add(base::xhash64_t::digest(global_xid->Get()));

    std::string merge_bf;
    {
//        std::unique_lock<std::mutex> lock(m_mutex);
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
        for (uint32_t i=0;i<bloomfilter_vec.size();i++) {
            bf_tmp += std::string((char*)&bloomfilter_vec[i], sizeof(uint64_t));
        }
        TOP_DEBUG("merged gossip bf:%s,hop:%d\n", HexEncode(bf_tmp).c_str(), message.hop_num());
    }

    std::vector<kadmlia::NodeInfoPtr> tmp_neighbors;
    uint32_t filtered = 0;
    for (auto iter = neighbors.begin(); iter != neighbors.end(); ++iter) {
        if ((*iter)->hash64 == 0) {
            TOP_WARN("node:%s hash64 empty, invalid", HexEncode((*iter)->node_id).c_str());
            continue;
        }

        if (bloomfilter->Contain((*iter)->hash64)) {
            ++filtered;
            continue;
        }

        tmp_neighbors.push_back(*iter);
    }
    TOP_DEBUG("after filter, remaining size %d, filter %d nodes", tmp_neighbors.size(), filtered);

    std::vector<kadmlia::NodeInfoPtr> rest_random_neighbors;
    rest_random_neighbors = GetRandomNodes(tmp_neighbors, GetNeighborCount(message));
    if (rest_random_neighbors.empty()) {
        TOP_INFO("stop Broadcast, msg.hop_num(%d), msg.type(%d)", message.hop_num(), message.type());
        const std::vector<uint64_t>& bloomfilter_vec = bloomfilter->Uint64Vector();
        std::string bf_str;
        for (uint32_t i=0;i<bloomfilter_vec.size();i++) {
            bf_str += std::string((char*)&bloomfilter_vec[i], sizeof(uint64_t));
        }
        m_gossip_map[message.gossip().msg_hash()] = GossipInfo {
                std::chrono::steady_clock::now(), bf_str};
        return;
    }

    // add sending nodes to bf
    for (auto iter = rest_random_neighbors.begin(); iter != rest_random_neighbors.end(); ++iter) {
        bloomfilter->Add((*iter)->hash64);
    }


    const std::vector<uint64_t>& bloomfilter_vec = bloomfilter->Uint64Vector();
    message.clear_bloomfilter();
    for (uint32_t i = 0; i < bloomfilter_vec.size(); ++i) {
        message.add_bloomfilter(bloomfilter_vec[i]);
    }

    std::string bf_str;
    for (uint32_t i=0;i<bloomfilter_vec.size();i++) {
        bf_str += std::string((char*)&bloomfilter_vec[i], sizeof(uint64_t));
    }
    {
//        std::unique_lock<std::mutex> lock(m_mutex);
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
}

void GossipBloomfilterMerge::BroadcastWithNoFilter(
        const std::string& local_id,
        transport::protobuf::RoutingMessage& message,
        const std::vector<kadmlia::NodeInfoPtr>& neighbors) {
    TOP_DEBUG("GossipBloomfilter Broadcast neighbors size %d", neighbors.size());

    BlockSyncManager::Instance()->NewBroadcastMessage(message);
    auto gossip_max_hop_num = kGossipDefaultMaxHopNum;
    if (message.gossip().max_hop_num() > 0) {
        gossip_max_hop_num = message.gossip().max_hop_num();
    }
    if (gossip_max_hop_num <= message.hop_num()) {
        TOP_INFO("message.type(%d) hop_num(%d) larger than gossip_max_hop_num(%d)",
                message.type(),
                message.hop_num(),
                gossip_max_hop_num);
        return;
    }

    if (ThisNodeIsEvil(message)) {
        TOP_WARN2("this node(%s) is evil", HexEncode(local_id).c_str());
        return;
    }
    Send(message, neighbors);
}

void GossipBloomfilterMerge::CleanTimerProc() {
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

void GossipBloomfilterMerge::Start() {
    auto timer_proc = std::bind(&GossipBloomfilterMerge::CleanTimerProc, this);
    m_clean_timer.Start(kCleanTimerInterval, kCleanTimerInterval, timer_proc);
}

}  // namespace gossip

}  // namespace top
#endif