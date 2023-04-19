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
#include "xmetrics/xmetrics.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/endpoint_util.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
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
  : transport_ptr_{transport_ptr}, local_node_ptr_{local_node_ptr}, destroy_(false) {
}

ElectRoutingTable::~ElectRoutingTable() {
    xdbg("~ElectRoutingTable %p start", this);
    {
        std::unique_lock<std::mutex> lock(m_nodes_mutex);
#if defined(DEBUG)
        for (auto const & _pair : m_nodes) {
            xdbg("~ElectRoutingTable %s, %p", _pair.first.c_str(), _pair.second.get());
        }
#endif
        m_nodes.clear();
    }
    m_expected_kad_keys.clear();
    m_index_map.clear();
#if defined(DEBUG)
    for (auto const & xip2 : m_xip2_for_shuffle) {
        xdbg("~ElectRoutingTable %s", xip2.c_str());
    }
#endif
    m_xip2_for_shuffle.clear();
    xdbg("~ElectRoutingTable %p end", this);
}

bool ElectRoutingTable::Init() {
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

int ElectRoutingTable::SendData(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port) {
    std::string data;
    if (!message.SerializeToString(&data)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    transport::UdpPropertyPtr udp_property;
    return transport_ptr_->SendDataWithProp(data, peer_ip, peer_port, udp_property, message.priority());
}

int ElectRoutingTable::SendPing(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port) {
    std::string msg;
    if (!message.SerializeToString(&msg)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    xbyte_buffer_t data{msg.begin(), msg.end()};
    return get_transport()->SendPing(data, peer_ip, peer_port);
}

int ElectRoutingTable::SendData(transport::protobuf::RoutingMessage & message, NodeInfoPtr node) {
    std::string data;
    if (!message.SerializeToString(&data)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    return transport_ptr_->SendDataWithProp(data, node->public_ip, node->public_port, node->udp_property, message.priority());
}

std::size_t ElectRoutingTable::nodes_size() {
    std::unique_lock<std::mutex> lock(m_nodes_mutex);
    return m_nodes.size();
}

void ElectRoutingTable::PrintRoutingTable() {
    if (destroy_) {
        return;
    }
    XMETRICS_PACKET_INFO("p2p_kad_info",
                         "local_nodeid",
                         get_local_node_info()->kad_key(),
                         "service_type",
                         get_local_node_info()->kadmlia_key()->GetServiceType().info().c_str(),
                         "node_size",
                         m_nodes.size(),
                         "broadcast_node_size",
                         m_broadcast_nodes.size(),
                         "unknown_node_size",
                         m_expected_kad_keys.size(),
                         "public_ip",
                         get_local_node_info()->public_ip(),
                         "public_port",
                         get_local_node_info()->public_port());
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
    //         HexSubstr(get_local_node_info()->kad_key()).c_str(),
    //         get_local_node_info()->local_ip().c_str(),
    //         get_local_node_info()->local_port(),
    //         HexSubstr(node->node_id).c_str(),
    //         node->local_ip.c_str(),
    //         node->local_port);
    // }
}

void ElectRoutingTable::GetRandomNodes(std::vector<NodeInfoPtr> & vec, size_t size) {
    auto shuffled_xip2 = get_shuffled_xip2();
    auto all_nodes = nodes();
    std::size_t min_size = std::min(shuffled_xip2.size(), size);
    for (std::size_t _i = 0; _i < min_size; ++_i) {
        if (all_nodes[shuffled_xip2[_i]] != nullptr) {
            vec.push_back(all_nodes[shuffled_xip2[_i]]);
        }
    }
}
NodeInfoPtr ElectRoutingTable::GetOneRandomNode() {
    auto shuffled_xip2 = get_shuffled_xip2();
    auto all_nodes = nodes();
    for (std::size_t _i = 0; _i < shuffled_xip2.size(); ++_i) {
        if (all_nodes[shuffled_xip2[_i]] != nullptr) {
            return all_nodes[shuffled_xip2[_i]];
        }
    }
    return nullptr;
}

std::unordered_map<std::string, NodeInfoPtr> ElectRoutingTable::nodes(bool cover_old_version) {
    if (cover_old_version) {
        std::unique_lock<std::mutex> lock(m_broadcast_nodes_mutex);
        return m_broadcast_nodes;
    } else {
        std::unique_lock<std::mutex> lock(m_nodes_mutex);
        return m_nodes;
    }
}

std::unordered_map<std::string, std::size_t> ElectRoutingTable::index_map(bool cover_old_version) {
    if (cover_old_version) {
        return m_broadcast_index_map;
    } else {
        return m_index_map;
    }
}

std::vector<std::string> ElectRoutingTable::get_shuffled_xip2(bool cover_old_version) {
    static std::random_device rd;
    static std::mt19937 g(rd());
    if (cover_old_version) {
        std::unique_lock<std::mutex> lock(m_broadcast_xip2_for_shuffle_mutex);
        std::shuffle(m_broadcast_xip2_for_shuffle.begin(), m_broadcast_xip2_for_shuffle.end(), g);
        return m_broadcast_xip2_for_shuffle;
    } else {
        std::unique_lock<std::mutex> lock(m_xip2_for_shuffle_mutex);
        std::shuffle(m_xip2_for_shuffle.begin(), m_xip2_for_shuffle.end(), g);
        return m_xip2_for_shuffle;
    }
}

std::size_t ElectRoutingTable::get_self_index() {
    return m_self_index;
}

NodeInfoPtr ElectRoutingTable::GetNode(const std::string & id) {
    std::unique_lock<std::mutex> lock(m_nodes_mutex);
    if (m_nodes.find(id) != m_nodes.end() && m_nodes.at(id)->public_port) {
        return m_nodes.at(id);
    }
    xdbg("elect routing table get node failed ,id: %s nodes.size:%zu", id.c_str(), m_nodes.size());

#if defined(DEBUG)
    for (auto const & _p : m_nodes) {
        xdbg("now have: %s %s %u", _p.first.c_str(), _p.second->public_ip.c_str(), _p.second->public_port);
    }
#endif

    return nullptr;
}

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
std::map<std::string, base::KadmliaKeyPtr> const & ElectRoutingTable::GetAllNodesRootKeyMap() const noexcept {
    return m_all_nodes_root_kay_keys;
}

// map<election_xip2_str,node_id_root_kad_key>
void ElectRoutingTable::SetElectionNodesExpected(std::map<std::string, base::KadmliaKeyPtr> const & elect_root_kad_keys_map,
                                                 std::map<std::string, NodeInfoPtr> const & last_round_nodes_map) {
    std::size_t index = 1;
    {
        std::unique_lock<std::mutex> lock(m_nodes_mutex);
        m_expected_kad_keys = elect_root_kad_keys_map;
        m_all_nodes_root_kay_keys = elect_root_kad_keys_map;
        for (auto const & p : elect_root_kad_keys_map) {
            NodeInfoPtr node_ptr;
            node_ptr.reset(new NodeInfo(p.first));
            if (p.second->Get() == base::GetRootKadmliaKey(global_node_id)->Get()) {
                m_self_index = index;
                xdbg("[ElectRoutingTable::SetElectionNodesExpected] Set self index %zu.", m_self_index);
                m_expected_kad_keys.erase(p.first);
            }
            m_nodes.insert(std::make_pair(p.first, node_ptr));
            m_xip2_for_shuffle.push_back(p.first);
            m_index_map.insert(std::make_pair(p.first, index++));
            xdbg("[ElectRoutingTable::SetElectionNodesExpected] get node %s %s", p.first.c_str(), p.second->Get().c_str());
        }
    }
    std::unique_lock<std::mutex> lock_nodes(m_broadcast_nodes_mutex);
    std::unique_lock<std::mutex> lock_shuffled(m_broadcast_xip2_for_shuffle_mutex);
    m_broadcast_nodes = m_nodes;
    m_broadcast_index_map = m_index_map;
    m_broadcast_xip2_for_shuffle = m_xip2_for_shuffle;
    for (auto const & _p : last_round_nodes_map) {
        if (_p.second != nullptr) {
            xdbg("[ElectRoutingTable::SetElectionNodesExpected] get last round node %s %s %u", _p.first.c_str(), _p.second->public_ip.c_str(), _p.second->public_port);
        } else {
            xdbg("[ElectRoutingTable::SetElectionNodesExpected] get last round node %s without node_info", _p.first.c_str());
        }
        m_broadcast_nodes.insert(_p);
        m_broadcast_index_map.insert(std::make_pair(_p.first, index++));
        m_broadcast_xip2_for_shuffle.push_back(_p.first);
    }
}

void ElectRoutingTable::EraseElectionNodesExpected(std::vector<base::KadmliaKeyPtr> const & kad_keys) {
    for (auto const & kad_key : kad_keys) {
        auto node_id = kad_key->Get();
        DoEraseElectionNodesExpected(node_id);
    }
}

void ElectRoutingTable::DoEraseElectionNodesExpected(std::string const & node_id) {
    std::unique_lock<std::mutex> lock(m_expected_kad_keys_mutex);
    if (m_expected_kad_keys.find(node_id) != m_expected_kad_keys.end()) {
        xdbg("[ElectRoutingTable::EraseElectionNodesExpected] Erase node %s", node_id.c_str());
        m_expected_kad_keys.erase(node_id);
    }
}

std::map<std::string, top::base::KadmliaKeyPtr> ElectRoutingTable::GetElectionNodesExpected() {
    std::unique_lock<std::mutex> lock(m_expected_kad_keys_mutex);
    return m_expected_kad_keys;
}

void ElectRoutingTable::HandleElectionNodesInfoFromRoot(std::map<std::string, kadmlia::NodeInfoPtr> const & nodes) {
    {
        xinfo("[ElectRoutingTable::HandleElectionNodesInfoFromRoot] node size:%zu local_service_type:%lld", nodes.size(), get_local_node_info()->service_type().value());
        std::vector<base::KadmliaKeyPtr> erase_keys;
        std::unique_lock<std::mutex> lock(m_nodes_mutex);
        for (auto const & p : nodes) {
            NodeInfoPtr node_ptr = m_nodes[p.first];
            // node_ptr.reset(new NodeInfo(_p.first));
            auto & root_node_info = p.second;
            node_ptr->public_ip = root_node_info->public_ip;
            node_ptr->public_port = root_node_info->public_port;
            node_ptr->service_type = get_local_node_info()->service_type();
            node_ptr->xid = root_node_info->xid;
            node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
            xdbg("[ElectRoutingTable::HandleElectionNodesInfoFromRoot] %s %s:%d, %lld",
                 node_ptr->node_id.c_str(),
                 node_ptr->public_ip.c_str(),
                 node_ptr->public_port,
                 node_ptr->service_type.value());
            erase_keys.push_back(base::GetKadmliaKey(node_ptr->node_id));
        }
        EraseElectionNodesExpected(erase_keys);
    }
    UpdateBroadcastNodeInfo();
}

void ElectRoutingTable::OnFindNodesFromRootRouting(std::string const & election_xip2, kadmlia::NodeInfoPtr const & node_info) {
    {
        std::unique_lock<std::mutex> lock(m_nodes_mutex);
        if (m_nodes.find(election_xip2) != m_nodes.end()) {
            NodeInfoPtr node_ptr = m_nodes[election_xip2];
            // node_ptr->local_ip = node_info->local_ip;
            // node_ptr->local_port = node_info->local_port;
            node_ptr->public_ip = node_info->public_ip;
            node_ptr->public_port = node_info->public_port;
            xdbg("[ElectRoutingTable::OnFindNodesFromRootRouting] get election_xip2: %s %s:%d", election_xip2.c_str(), node_ptr->public_ip.c_str(), node_ptr->public_port);
            DoEraseElectionNodesExpected(election_xip2);
        }
    }
    UpdateBroadcastNodeInfo();
}

void ElectRoutingTable::UpdateBroadcastNodeInfo() {
    std::unique_lock<std::mutex> nodes_lock(m_nodes_mutex);
    std::unique_lock<std::mutex> broadcast_nodes_lock(m_broadcast_nodes_mutex);
    for (auto const & p : m_nodes) {
        auto const & xip = p.first;
        auto const & node_info_ptr = p.second;
        assert(m_broadcast_nodes.find(xip) != m_broadcast_nodes.end());
        m_broadcast_nodes[xip] = node_info_ptr;
    }
}

void ElectRoutingTable::getLastRoundElectNodesInfo(std::vector<std::pair<std::string, NodeInfoPtr>> & nodes_info) {
    auto index = m_nodes.size();  // origin size
    for (auto const & _p : m_broadcast_index_map) {
        if (_p.second <= index) {
            continue;
        } else {
            // last round index:
            std::string last_election_xip2 = _p.first;
            assert(m_broadcast_nodes.find(last_election_xip2) != m_broadcast_nodes.end());
            auto last_round_node_info_ptr = m_broadcast_nodes[last_election_xip2];
            if (last_round_node_info_ptr == nullptr) {
                xdbg("[ElectRoutingTable::getLastRoundElectNodesInfo] lack of complete nodes info, can't response request");
                nodes_info.clear();
                return;
            }
            nodes_info.push_back(std::make_pair(last_election_xip2, last_round_node_info_ptr));
        }
    }
    return;
}

void ElectRoutingTable::OnAddLastRoundElectNodes(std::vector<std::pair<std::string, NodeInfoPtr>> const & nodes) {
    auto index = m_broadcast_index_map.size() + 1;
    for (auto const & _p : nodes) {
        if (_p.second != nullptr) {
            xdbg("[ElectRoutingTable::SetElectionNodesExpected] get last round node %s %s %u", _p.first.c_str(), _p.second->public_ip.c_str(), _p.second->public_port);
        } else {
            xdbg("[ElectRoutingTable::SetElectionNodesExpected] get last round node %s without node_info", _p.first.c_str());
        }
        m_broadcast_nodes.insert(_p);
        m_broadcast_index_map.insert(std::make_pair(_p.first, index++));
        m_broadcast_xip2_for_shuffle.push_back(_p.first);
    }
}

}  // namespace kadmlia

}  // namespace top
