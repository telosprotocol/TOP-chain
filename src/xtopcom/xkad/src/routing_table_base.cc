// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xpacket.h"
#include "xbase/xutl.h"
#include "xgossip/include/gossip_utils.h"
#include "xkad/routing_table/callback_manager.h"
#include "xkad/routing_table/elect_routing_table.h"
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

RoutingTableBase::RoutingTableBase(std::shared_ptr<transport::Transport> transport_ptr, std::shared_ptr<LocalNodeInfo> local_node_ptr)
  : transport_ptr_{transport_ptr,},local_node_ptr_{ local_node_ptr} {
}

void RoutingTableBase::GetRangeNodes(uint32_t min_index, uint32_t max_index, std::vector<NodeInfoPtr> & vec) {
    assert(false);
    return;
}


int RoutingTableBase::SendData(const xbyte_buffer_t & data, const std::string & peer_ip, uint16_t peer_port, uint16_t priority) {
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

int RoutingTableBase::SendData(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port) {
    std::string msg;
    if (!message.SerializeToString(&msg)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    xbyte_buffer_t data{msg.begin(), msg.end()};
    return SendData(data, peer_ip, peer_port, message.priority());
}

int RoutingTableBase::SendPing(transport::protobuf::RoutingMessage & message, const std::string & peer_ip, uint16_t peer_port) {
    std::string msg;
    if (!message.SerializeToString(&msg)) {
        xdbg("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    xbyte_buffer_t data{msg.begin(), msg.end()};
    return get_transport()->SendPing(data, peer_ip, peer_port);
}

int RoutingTableBase::SendData(transport::protobuf::RoutingMessage & message, NodeInfoPtr node) {
    if (node->same_vlan) {
        return SendData(message, node->local_ip, node->local_port);
    }

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

}  // namespace kadmlia

}  // namespace top
