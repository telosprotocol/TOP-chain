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
  : transport_ptr_{transport_ptr}, local_node_ptr_{local_node_ptr}, destroy_(false) {
}

bool ElectRoutingTable::Init() {
    // todo charles local port && local ip might be unused? try delete it .
    uint16_t local_port = get_transport()->local_port();
    get_local_node_info()->set_local_port(local_port);

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

int ElectRoutingTable::SendData(const xbyte_buffer_t & data, const std::string & peer_ip, uint16_t peer_port, uint16_t priority) {
    uint8_t local_buf[kUdpPacketBufferSize];
    base::xpacket_t packet(base::xcontext_t::instance(), local_buf, sizeof(local_buf), 0, 0, false);
    _xip2_header header;
    memset(&header, 0, sizeof(header));
    header.flags |= priority;
    packet.get_body().push_back((uint8_t *)&header, enum_xip2_header_len);
    packet.get_body().push_back((uint8_t *)data.data(), data.size());  // NOLINT
    packet.set_to_ip_addr(peer_ip);
    packet.set_to_ip_port(peer_port);
    return transport_ptr_->SendData(packet);
}

int ElectRoutingTable::SendData(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port) {
    std::string msg;
    if (!message.SerializeToString(&msg)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    xbyte_buffer_t data{msg.begin(), msg.end()};
    return SendData(data, peer_ip, peer_port, message.priority());
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
    // if (node->same_vlan) {
    //     return SendData(message, node->local_ip, node->local_port);
    // }

    std::string msg;
    if (!message.SerializeToString(&msg)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    xbyte_buffer_t data{msg.begin(), msg.end()};
    //	return SendData(data, peer_ip, peer_port, message.priority());

    uint8_t local_buf[kUdpPacketBufferSize];
    base::xpacket_t packet(base::xcontext_t::instance(), local_buf, sizeof(local_buf), 0, 0, false);
    _xip2_header header;
    memset(&header, 0, sizeof(header));
    header.flags |= message.priority();
    packet.get_body().push_back((uint8_t *)&header, enum_xip2_header_len);
    packet.get_body().push_back((uint8_t *)data.data(), data.size());  // NOLINT
    packet.set_to_ip_addr(node->public_ip);
    packet.set_to_ip_port(node->public_port);
    xdbg("xkad send message.type:%d size:%d", message.type(), packet.get_size());
    return transport_ptr_->SendDataWithProp(packet, node->udp_property);
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
                         get_local_node_info()->id(),
                         "service_type",
                         get_local_node_info()->kadmlia_key()->GetServiceType().info().c_str(),
                         "node_size",
                         m_nodes.size(),
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
    //         HexSubstr(get_local_node_info()->id()).c_str(),
    //         get_local_node_info()->local_ip().c_str(),
    //         get_local_node_info()->local_port(),
    //         HexSubstr(node->node_id).c_str(),
    //         node->local_ip.c_str(),
    //         node->local_port);
    // }
}

void ElectRoutingTable::SetFreqMessage(transport::protobuf::RoutingMessage & message) {
    message.set_hop_num(0);
    message.set_src_service_type(get_local_node_info()->service_type().value());
    message.set_src_node_id(get_local_node_info()->id());
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

void ElectRoutingTable::GetRandomNodes(std::vector<NodeInfoPtr> & vec, size_t size) {
    std::size_t min_size = std::min(m_xip2_for_shuffle.size(), size);
    for (std::size_t _i = 0; _i < min_size; ++_i)
        vec.push_back(m_nodes[m_xip2_for_shuffle[_i]]);
}

std::unordered_map<std::string, NodeInfoPtr> ElectRoutingTable::nodes() {
    std::unique_lock<std::mutex> lock(m_nodes_mutex);
    return m_nodes;
}

std::unordered_map<std::string, std::size_t> ElectRoutingTable::index_map() {
    return m_index_map;
}

std::vector<std::string> ElectRoutingTable::get_shuffled_xip2() {
    static std::random_device rd;
    static std::mt19937 g(rd());

    std::shuffle(m_xip2_for_shuffle.begin(), m_xip2_for_shuffle.end(), g);
    return m_xip2_for_shuffle;
}

std::size_t ElectRoutingTable::get_self_index() {
    return m_self_index;
}

NodeInfoPtr ElectRoutingTable::GetNode(const std::string & id) {
    if (m_nodes.find(id) != m_nodes.end() && m_nodes.at(id)->public_port) {
        return m_nodes.at(id);
    }
    xdbg("elect routing table get node failed ,id: %s", id.c_str());

    return nullptr;
}

bool ElectRoutingTable::HasNode(NodeInfoPtr node) {
    // return iter != node_id_map_.end();
    return false;
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
void ElectRoutingTable::SetElectionNodesExpected(std::map<std::string, base::KadmliaKeyPtr> const & elect_root_kad_keys_map) {
    std::unique_lock<std::mutex> lock(m_nodes_mutex);
    m_expected_kad_keys = elect_root_kad_keys_map;
    std::size_t index = 1;
    for (auto _p : elect_root_kad_keys_map) {
        NodeInfoPtr node_ptr;
        node_ptr.reset(new NodeInfo(_p.first));
        if (_p.second->Get() == base::GetRootKadmliaKey(global_node_id)->Get()) {
            m_self_index = index;
            xdbg("[ElectRoutingTable::SetElectionNodesExpected] Set self index %zu.", m_self_index);
            m_expected_kad_keys.erase(_p.first);
        }
        m_nodes.insert(std::make_pair(_p.first, node_ptr));
        m_xip2_for_shuffle.push_back(_p.first);
        m_index_map.insert(std::make_pair(_p.first, index++));
        xdbg("[ElectRoutingTable::SetElectionNodesExpected] get node %s %s", _p.first.c_str(), _p.second->Get().c_str());
    }
}

void ElectRoutingTable::EraseElectionNodesExpected(std::vector<base::KadmliaKeyPtr> const & kad_keys) {
    for (auto _kad_key : kad_keys) {
        auto node_id = _kad_key->Get();
        if (m_expected_kad_keys.find(node_id) != m_expected_kad_keys.end()) {
            xdbg("[ElectRoutingTable::EraseElectionNodesExpected] Erase node %s", node_id.c_str());
            m_expected_kad_keys.erase(node_id);
        }
    }
}

std::map<std::string, top::base::KadmliaKeyPtr> ElectRoutingTable::GetElectionNodesExpected() {
    return m_expected_kad_keys;
}

void ElectRoutingTable::HandleElectionNodesInfoFromRoot(std::map<std::string, kadmlia::NodeInfoPtr> const & nodes) {
    xinfo("[ElectRoutingTable::HandleElectionNodesInfoFromRoot] node size:%zu local_service_type:%lld", nodes.size(), get_local_node_info()->service_type().value());
    std::vector<base::KadmliaKeyPtr> erase_keys;
    std::unique_lock<std::mutex> lock(m_nodes_mutex);
    for (auto _p : nodes) {
        NodeInfoPtr node_ptr = m_nodes[_p.first];
        // node_ptr.reset(new NodeInfo(_p.first));
        auto & root_node_info = _p.second;
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

void ElectRoutingTable::OnFindNodesFromRootRouting(std::string const & election_xip2, kadmlia::NodeInfoPtr const & node_info) {
    std::unique_lock<std::mutex> lock(m_nodes_mutex);
    if (m_nodes.find(election_xip2) != m_nodes.end()) {
        NodeInfoPtr node_ptr = m_nodes[election_xip2];
        // node_ptr->local_ip = node_info->local_ip;
        // node_ptr->local_port = node_info->local_port;
        node_ptr->public_ip = node_info->public_ip;
        node_ptr->public_port = node_info->public_port;
        xdbg("[ElectRoutingTable::OnFindNodesFromRootRouting] get election_xip2: %s %s:%d", election_xip2.c_str(), node_ptr->public_ip.c_str(), node_ptr->public_port);
        m_expected_kad_keys.erase(election_xip2);
    }
}

}  // namespace kadmlia

}  // namespace top
