// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkad/routing_table/node_info.h"
#include "xtransport/udp_transport/xudp_socket.h"

namespace top {
namespace kadmlia {

static const int kHeartbeatFirstTimeout = 8;  // seconds
static const int kHeartbeatSecondTimeout = 2;  // seconds
static const int kHeartbeatErrorMaxCount = 12;

NodeInfo::NodeInfo() {
}

NodeInfo::NodeInfo(const NodeInfo& other)
        : node_id(other.node_id),
            bucket_index(other.bucket_index),
            public_ip(other.public_ip),
            public_port(other.public_port),
            local_ip(other.local_ip),
            local_port(other.local_port),
            connection_id(other.connection_id),
            detection_count(other.detection_count),
            detection_delay_count(other.detection_delay_count),
            service_type(other.service_type),
            xid(other.xid)
            {
    hash64 = base::xhash64_t::digest(node_id);
	udp_property.reset(new top::transport::UdpProperty());	
}

NodeInfo::NodeInfo(const std::string& id) : node_id(id) {
	udp_property.reset(new top::transport::UdpProperty());	
}

NodeInfo::~NodeInfo() {
	udp_property = nullptr;
}

NodeInfo& NodeInfo::operator=(const NodeInfo& other) {
    if (this == &other) {
        return *this;
    }
    node_id = other.node_id;
    bucket_index = other.bucket_index;
    public_ip = other.public_ip;
    public_port = other.public_port;
    local_ip = other.local_ip;
    local_port =  other.local_port;
    connection_id = other.connection_id;
    detection_count = other.detection_count;
    detection_delay_count = other.detection_delay_count;
    service_type = other.service_type;
    xid = other.xid;
    //hash64 = base::xhash64_t::digest(xid);
    hash64 = base::xhash64_t::digest(node_id);
    return *this;
}

bool NodeInfo::operator < (const NodeInfo& other) const {
    return node_id < other.node_id;
}

bool NodeInfo::IsPublicNode() const {
    return public_ip == local_ip && public_port == local_port;
}

std::string NodeInfo::string() {
    return node_id;
}

}  // namespace kadmlia
}  // namespace top
