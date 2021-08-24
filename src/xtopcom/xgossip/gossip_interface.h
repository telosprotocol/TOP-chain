// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xpacket.h"
#include "xkad/routing_table/elect_routing_table.h"
#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/root_routing_table.h"
#include "xtransport/proto/transport.pb.h"
#include "xtransport/transport.h"
#include "xtransport/udp_transport/transport_util.h"

#include <stdint.h>

#include <memory>

namespace top {

namespace base {
class Uint64BloomFilter;
};

namespace gossip {

struct DispatchInfos {
    kadmlia::NodeInfoPtr nodes;
    uint64_t sit1;
    uint64_t sit2;

    DispatchInfos(kadmlia::NodeInfoPtr _nodes, uint64_t _sit1, uint64_t _sit2) : nodes{_nodes}, sit1{_sit1}, sit2{_sit2} {
    }
    uint64_t & get_sit1() {
        return sit1;
    }
    uint64_t & get_sit2() {
        return sit2;
    }
};

class GossipInterface {
public:
    virtual void Broadcast(uint64_t local_hash64, transport::protobuf::RoutingMessage & message, std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> neighbors) {
        assert(false);
        return;
    }
    virtual void Broadcast(transport::protobuf::RoutingMessage & message, kadmlia::ElectRoutingTablePtr & routing_table) {
        assert(false);
        return;
    }

protected:
    GossipInterface(transport::TransportPtr transport_ptr) : transport_ptr_(transport_ptr) {
    }
    virtual ~GossipInterface() {
    }

    uint64_t GetDistance(const std::string & src, const std::string & des);
    void Send(transport::protobuf::RoutingMessage & message, const std::vector<kadmlia::NodeInfoPtr> & nodes);
    void MutableSend(transport::protobuf::RoutingMessage & message, const std::vector<kadmlia::NodeInfoPtr> & nodes);
    void MutableSendHash(transport::protobuf::RoutingMessage & message, const std::vector<kadmlia::NodeInfoPtr> & nodes);
    // todo next version delete GetNeighborCount.(only used in old broadcast)
    uint32_t GetNeighborCount(transport::protobuf::RoutingMessage & message);
    std::vector<kadmlia::NodeInfoPtr> GetRandomNodes(std::vector<kadmlia::NodeInfoPtr> & neighbors, uint32_t number_to_get) const;

    // void SendLayered(transport::protobuf::RoutingMessage & message, const std::vector<kadmlia::NodeInfoPtr> & nodes);
    void SendDispatch(transport::protobuf::RoutingMessage & message, const std::vector<gossip::DispatchInfos> & dispatch_nodes);
    void CheckDiffNetwork(transport::protobuf::RoutingMessage & message);

    transport::TransportPtr transport_ptr_;

private:
    DISALLOW_COPY_AND_ASSIGN(GossipInterface);
};

typedef std::shared_ptr<GossipInterface> GossipInterfacePtr;

}  // namespace gossip

}  // namespace top
