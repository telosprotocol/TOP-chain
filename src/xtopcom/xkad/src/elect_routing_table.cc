// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkad/routing_table/elect_routing_table.h"

#include "xbase/xpacket.h"
#include "xbase/xutl.h"
#include "xgossip/include/gossip_utils.h"
#include "xkad/routing_table/callback_manager.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/node_detection_manager.h"
#include "xkad/routing_table/nodeid_utils.h"
#include "xmetrics/xmetrics.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/endpoint_util.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/rand_util.h"
#include "xpbase/base/top_string_util.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/uint64_bloomfilter.h"
#include "xtransport/udp_transport/transport_util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <bitset>
#include <chrono>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <unordered_map>

namespace top {

namespace kadmlia {

static const int32_t kHeartbeatPeriod = 30 * 1000 * 1000;  // 2s

ElectRoutingTable::ElectRoutingTable(std::shared_ptr<transport::Transport> transport_ptr, std::shared_ptr<LocalNodeInfo> local_node_ptr)
  : RoutingTableBase{transport_ptr, local_node_ptr}, destroy_(false) {
}

bool ElectRoutingTable::Init() {
    // todo charles local port && local ip might be unused? try delete it .
    uint16_t local_port = transport_ptr_->local_port();
    local_node_ptr_->set_local_port(local_port);

    // {
    //     std::unique_lock<std::mutex> lock_hash(node_hash_map_mutex_);
    //     NodeInfoPtr node_ptr;
    //     node_ptr.reset(new NodeInfo(local_node_ptr_->id()));
    //     node_ptr->local_ip = local_node_ptr_->local_ip();
    //     node_ptr->local_port = local_node_ptr_->local_port();
    //     node_ptr->public_ip = local_node_ptr_->public_ip();
    //     node_ptr->public_port = local_node_ptr_->public_port();
    //     node_ptr->xid = local_node_ptr_->xid();
    //     node_ptr->hash64 = local_node_ptr_->hash64();
    //     TOP_DEBUG("add node_hash_map node_id:%s hash64:%llu public_endpoint:%s:%u",
    //               HexEncode(node_ptr->node_id).c_str(),
    //               node_ptr->hash64,
    //               node_ptr->public_ip.c_str(),
    //               node_ptr->public_port);
    //     node_hash_map_->insert(std::make_pair(node_ptr->hash64, node_ptr));
    // }

    timer_heartbeat_ = std::make_shared<base::TimerRepeated>(timer_manager_, "ElectRoutingTable::PrintRoutingTable");
    timer_heartbeat_->Start(kHeartbeatPeriod, kHeartbeatPeriod, std::bind(&ElectRoutingTable::PrintRoutingTable, this));

    using namespace std::placeholders;
    HeartbeatManagerIntf::Instance()->Register(std::to_string((long)this), std::bind(&ElectRoutingTable::OnHeartbeatFailed, this, _1, _2));

    return true;
}

bool ElectRoutingTable::UnInit() {
    destroy_ = true;

    timer_heartbeat_ = nullptr;
    return true;
}

std::size_t ElectRoutingTable::nodes_size() {
    return m_nodes.size();
}

std::vector<NodeInfoPtr> ElectRoutingTable::GetClosestNodes(const std::string & target_id, uint32_t number_to_get) {
    if(m_nodes.find(target_id)!=m_nodes.end()){
        return std::vector<NodeInfoPtr>{m_nodes.at(target_id)};
    }
    std::vector<NodeInfoPtr> res_nodes;
    GetRandomNodes(res_nodes, number_to_get);
    return res_nodes;
}

void ElectRoutingTable::PrintRoutingTable() {
    if (destroy_) {
        return;
    }
    XMETRICS_PACKET_INFO("p2p_kad_info",
                         "local_nodeid",
                         local_node_ptr_->id(),
                         "service_type",
                         local_node_ptr_->kadmlia_key()->GetServiceType().info().c_str(),
                         "xnetwork_id",
                         local_node_ptr_->kadmlia_key()->xnetwork_id(),
                         "zone_id",
                         local_node_ptr_->kadmlia_key()->zone_id(),
                         "cluster_id",
                         local_node_ptr_->kadmlia_key()->cluster_id(),
                         "group_id",
                         local_node_ptr_->kadmlia_key()->group_id(),
                         "node_size",
                         m_nodes.size(),
                         "public_ip",
                         local_node_ptr_->public_ip(),
                         "public_port",
                         local_node_ptr_->public_port());
}

void ElectRoutingTable::OnHeartbeatFailed(const std::string & ip, uint16_t port) {
    // std::vector<NodeInfoPtr> failed_nodes;
    // {
    //     NodesLock lock(nodes_mutex_);
    //     for (auto it = nodes_.begin(); it != nodes_.end();) {
    //         NodeInfoPtr node = *it;
    //         if (!node) {
    //             TOP_WARN("node empty, invalid");
    //             it = EraseNode(it);
    //             continue;
    //         }
    //         if (node->public_ip == ip && node->public_port == port) {
    //             failed_nodes.push_back(node);
    //         }
    //         ++it;
    //     }
    // }

    // for (auto & node : failed_nodes) {
    //     DropNode(node);
    //     TOP_WARN_NAME(
    //         "[%ld] node heartbeat error after tried: %d times.ID:[%s],"
    //         "IP:[%s],Port[%d] to ID:[%s],IP[%s],Port[%d] drop it.",
    //         (long)this,
    //         node->heartbeat_count,
    //         HexSubstr(local_node_ptr_->id()).c_str(),
    //         local_node_ptr_->local_ip().c_str(),
    //         local_node_ptr_->local_port(),
    //         HexSubstr(node->node_id).c_str(),
    //         node->local_ip.c_str(),
    //         node->local_port);
    // }
}

void ElectRoutingTable::SetFreqMessage(transport::protobuf::RoutingMessage & message) {
    message.set_hop_num(0);
    message.set_src_service_type(local_node_ptr_->service_type().value());
    message.set_src_node_id(local_node_ptr_->id());
    message.set_xid(global_xid->Get());
    message.set_priority(enum_xpacket_priority_type_routine);
    message.set_id(CallbackManager::MessageId());
    if (message.broadcast()) {
        auto gossip = message.mutable_gossip();
        // gossip->set_neighber_count(gossip::kGossipSendoutMaxNeighbors);
        gossip->set_neighber_count(4);
        gossip->set_stop_times(gossip::kGossipSendoutMaxTimes);
        // gossip->set_gossip_type(gossip::kGossipBloomfilterAndLayered);
        gossip->set_gossip_type(gossip::kGossipBloomfilter);
        // gossip->set_max_hop_num(kHopToLive);
        gossip->set_max_hop_num(10);
        gossip->set_evil_rate(0);
        gossip->set_switch_layer_hop_num(gossip::kGossipSwitchLayerCount);
        gossip->set_ign_bloomfilter_level(gossip::kGossipBloomfilterIgnoreLevel);
    }
}

int ElectRoutingTable::AddNode(NodeInfoPtr node) {
    assert(false);
    return kKadSuccess;
}

bool ElectRoutingTable::CanAddNode(NodeInfoPtr node) {
    xdbg("node_id(%s), pub(%s:%d)", HexSubstr(node->node_id).c_str(), node->public_ip.c_str(), node->public_port);
    if (node->node_id == local_node_ptr_->id()) {
        xdbg("local node");
        return false;
    }

    if (node->node_id.size() != kNodeIdSize) {
        xdbg("node id size is invalid![%d] should[%d]", node->node_id.size(), kNodeIdSize);
        return false;
    }

    if (node->public_ip.empty() || node->public_port <= 0) {
        xdbg("node[%s] public ip or public port invalid!", HexEncode(node->node_id).c_str());
        return false;
    }

    return true;
}

int ElectRoutingTable::DropNode(NodeInfoPtr node) {
    assert(false);
    // {
    //     NodesLock vec_lock(nodes_mutex_);
    //     for (auto iter = nodes_.begin(); iter != nodes_.end(); ++iter) {
    //         if ((*iter)->node_id == node->node_id) {
    //             EraseNode(iter);
    //             break;
    //         }
    //     }
    // }

    // {
    //     std::unique_lock<std::mutex> set_lock(node_id_map_mutex_);
    //     auto iter = node_id_map_.find(node->node_id);
    //     if (iter != node_id_map_.end()) {
    //         node_id_map_.erase(iter);
    //     }
    // }

    // {
    //     std::unique_lock<std::mutex> lock_hash(node_hash_map_mutex_);
    //     auto iter = node_hash_map_->find(node->hash64);
    //     if (iter != node_hash_map_->end()) {
    //         node_hash_map_->erase(iter);
    //     }
    // }

    // {
    //     std::unique_lock<std::mutex> lock(use_nodes_mutex_);
    //     no_lock_for_use_nodes_.reset();
    //     no_lock_for_use_nodes_ = std::make_shared<std::vector<NodeInfoPtr>>(nodes());
    // }

    return kKadSuccess;
}

NodeInfoPtr ElectRoutingTable::GetRandomNode() {
    // if (nodes_.empty()) {
    //     return nullptr;
    // }
    // return nodes_[RandomUint32() % nodes_.size()];
    assert(false);
    return nullptr;
}

bool ElectRoutingTable::GetRandomNodes(std::vector<NodeInfoPtr> & vec, size_t size) {
    // if (nodes_.empty()) {
    //     return false;
    // }

    // if (nodes_.size() <= size) {
    //     vec = nodes_;
    //     return true;
    // }

    // auto nsize = nodes_.size();
    // std::set<uint32_t> index_set;
    // for (size_t i = 0; i < 2 * size; ++i) {
    //     if (index_set.size() >= size) {
    //         break;
    //     }
    //     auto index = RandomUint32() % nsize;
    //     index_set.insert(index);
    // }
    // for (auto & item : index_set) {
    //     vec.push_back(nodes_[item]);
    // }
    // return true;

    assert(false);
    return false;
}

std::unordered_map<std::string, NodeInfoPtr> ElectRoutingTable::nodes() {
    return m_nodes;
}

// void ElectRoutingTable::GetRangeNodes(const uint64_t & min, const uint64_t & max, std::vector<NodeInfoPtr> & vec) {
    // if (min == 0 && max == std::numeric_limits<uint64_t>::max()) {
    //     vec = nodes();
    //     return;
    // }

    // std::unique_lock<std::mutex> lock(node_hash_map_mutex_);
    // auto minit = node_hash_map_->lower_bound(min);  // the first item not less than
    // auto maxit = node_hash_map_->upper_bound(max);  // the first item greater than
    // for (auto it = minit; it != maxit && it != node_hash_map_->end(); ++it) {
    //     vec.push_back(it->second);
    // }
//     return;
// }

// include min_index, include max_index. [,]
// void ElectRoutingTable::GetRangeNodes(uint32_t min_index, uint32_t max_index, std::vector<NodeInfoPtr> & vec) {
    // if (min_index > max_index) {
    //     return;
    // }
    // if (min_index >= node_hash_map_->size() || max_index < 0) {
    //     return;
    // }
    // if (max_index >= node_hash_map_->size()) {
    //     max_index = node_hash_map_->size() - 1;
    // }
    // if (min_index == 0 && max_index == node_hash_map_->size() - 1) {
    //     vec = nodes();
    //     return;
    // }

    // std::unique_lock<std::mutex> lock(node_hash_map_mutex_);
    // auto ibegin = node_hash_map_->begin();
    // auto nxit_min = std::next(ibegin, min_index);
    // auto nxit_max = std::next(ibegin, max_index + 1);

    // for (; nxit_min != nxit_max; ++nxit_min) {
    //     vec.push_back(nxit_min->second);
    // }
//     return;
// }

NodeInfoPtr ElectRoutingTable::GetNode(const std::string & id) {
    // auto iter = node_id_map_.find(id);
    // if (iter != node_id_map_.end()) {
    //     return iter->second;
    // }

    return nullptr;
}

bool ElectRoutingTable::HasNode(NodeInfoPtr node) {
    // return iter != node_id_map_.end();
    return false;
}

// NodeInfoPtr ElectRoutingTable::FindLocalNode(const std::string node_id) {
//     std::unique_lock<std::mutex> lock(node_id_map_mutex_);
//     auto iter = node_id_map_.find(node_id);
//     if (iter != node_id_map_.end()) {
//         return iter->second;
//     }
//     return nullptr;
// }

bool ElectRoutingTable::CloserToTarget(const std::string & id1, const std::string & id2, const std::string & target_id) {
    for (int i = 0; i < kNodeIdSize; ++i) {
        unsigned char result1 = id1[i] ^ target_id[i];
        unsigned char result2 = id2[i] ^ target_id[i];
        if (result1 != result2) {
            return result1 < result2;
        }
    }
    return false;
}

// map<election_xip2_str,node_id_root_kad_key>
void ElectRoutingTable::SetElectionNodesExpected(std::map<std::string, base::KadmliaKeyPtr> const & elect_root_kad_keys_map) {
    m_expected_kad_keys = elect_root_kad_keys_map;
    for (auto _p : m_expected_kad_keys) {
        NodeInfoPtr node_ptr;
        node_ptr.reset(new NodeInfo(_p.first));
        m_nodes.insert(std::make_pair(_p.first, node_ptr));
        xdbg("Charles Debug GetSameNetworkNodesV3 SET %s %s", _p.first.c_str(), _p.second->Get().c_str());
    }
}

void ElectRoutingTable::EraseElectionNodesExpected(std::vector<base::KadmliaKeyPtr> const & kad_keys) {
    for (auto _kad_key : kad_keys) {
        auto node_id = _kad_key->Get();
        xdbg("Charles Debug GetSameNetworkNodesV3 Already find node %s", node_id.c_str());
        if (m_expected_kad_keys.find(node_id) != m_expected_kad_keys.end()) {
            xdbg("Charles Debug GetSameNetworkNodesV3 Erase node %s", node_id.c_str());
            m_expected_kad_keys.erase(node_id);
        }
    }
}

std::map<std::string, top::base::KadmliaKeyPtr> ElectRoutingTable::GetElectionNodesExpected() {
    return m_expected_kad_keys;
}

void ElectRoutingTable::HandleElectionNodesInfoFromRoot(std::map<std::string, kadmlia::NodeInfoPtr> const & nodes) {
    xdbg("Charles Debug GetSameNetworkNodesV3 %zu local_service_type:%lld", nodes.size(), local_node_ptr_->service_type().value());
    std::vector<base::KadmliaKeyPtr> erase_keys;
    for (auto _p : nodes) {
        NodeInfoPtr node_ptr = m_nodes[_p.first];
        // node_ptr.reset(new NodeInfo(_p.first));
        auto & root_node_info = _p.second;
        xdbg("Charles Debug GetSameNetworkNodesV3 %s ", _p.first.c_str());
        node_ptr->local_ip = root_node_info->local_ip;
        node_ptr->local_port = root_node_info->local_port;
        node_ptr->public_ip = root_node_info->public_ip;
        node_ptr->public_port = root_node_info->public_port;
        node_ptr->service_type = local_node_ptr_->service_type();
        node_ptr->xid = root_node_info->xid;
        node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
        xdbg("Charles Debug GetSameNetworkNodesV3 %s %s:%d, %lld", node_ptr->node_id.c_str(), node_ptr->public_ip.c_str(), node_ptr->public_port, node_ptr->service_type.value());
        if (CanAddNode(node_ptr)) {
            if (node_ptr->node_id != local_node_ptr_->id()) {
                xdbg("HandleElectionNodesInfoFromRoot[%d] get node(%s:%d-%d)",
                     local_node_ptr_->service_type().value(),
                     node_ptr->public_ip.c_str(),
                     node_ptr->public_port,
                     node_ptr->service_type.value());
                AddNode(node_ptr);
                erase_keys.push_back(base::GetKadmliaKey(node_ptr->node_id));
                continue;
            }

            // node_detection_ptr_->AddDetectionNode(node_ptr);
        }
    }
    EraseElectionNodesExpected(erase_keys);
}

void ElectRoutingTable::OnFindNodesFromRootRouting(std::string const & election_xip2, kadmlia::NodeInfoPtr const & node_info) {
    if (m_nodes.find(election_xip2) != m_nodes.end()) {
        NodeInfoPtr node_ptr = m_nodes[election_xip2];
        node_ptr->local_ip = node_info->local_ip;
        node_ptr->local_port = node_info->local_port;
        node_ptr->public_ip = node_info->public_ip;
        node_ptr->public_port = node_info->public_port;
        xdbg("Charles Debug ElectRoutingTable::OnFindNodesFromRootRouting get election_xip2: %s %s:%d", election_xip2.c_str(), node_ptr->local_ip.c_str(), node_ptr->local_port);
    }
}

}  // namespace kadmlia

}  // namespace top
