// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xgossip/gossip_interface.h"
#include "xtransport/transport.h"

namespace top {

namespace gossip {

class GossipDispatcher : public GossipInterface {
public:
    explicit GossipDispatcher(transport::TransportPtr transport_ptr);
    virtual ~GossipDispatcher();
    virtual void Broadcast(uint64_t local_hash64, transport::protobuf::RoutingMessage & message, std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> neighbors);
    virtual void Broadcast(transport::protobuf::RoutingMessage & message, kadmlia::ElectRoutingTablePtr & routing_table);

private:
    void GenerateDispatchInfos(transport::protobuf::RoutingMessage & message, kadmlia::ElectRoutingTablePtr & routing_table, std::vector<DispatchInfos> & select_nodes);
};

}  // namespace gossip

}  // namespace top