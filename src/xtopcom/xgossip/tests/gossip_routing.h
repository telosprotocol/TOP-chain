// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <thread>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/kad_key/get_kadmlia_key.h"
#include "xkad/routing_table/routing_table.h"
#include "xtransport/proto/transport.pb.h"
#include "xkad/routing_table/callback_manager.h"

namespace top {

namespace gossip {

class GossipBaseRouting : public kadmlia::RoutingTable {
public:
   GossipBaseRouting(
           std::shared_ptr<transport::Transport> transport,
           kadmlia::LocalNodeInfoPtr local_node_ptr,
           const uint32_t RoutingMaxNodesSiz)
           : kadmlia::RoutingTable(transport, kNodeIdSize, local_node_ptr),
             RoutingMaxNodesSize_(RoutingMaxNodesSiz) {}
    virtual ~GossipBaseRouting() {}

    virtual int AddNode(kadmlia::NodeInfoPtr node) override;
protected:
    virtual bool NewNodeReplaceOldNode(kadmlia::NodeInfoPtr node, bool remove) override;

private:
    uint32_t RoutingMaxNodesSize_;

protected:
    DISALLOW_COPY_AND_ASSIGN(GossipBaseRouting);
};

typedef std::shared_ptr<GossipBaseRouting> GossipBaseRoutingPtr;

kadmlia::RoutingTablePtr CreateGossipRoutingTableForTest(
        transport::TransportPtr transport,
        uint32_t routing_max_node,
        uint32_t xnetwork_id,
        uint8_t zone_id,
        uint8_t cluster_id,
        uint8_t group_id);

void CreateMessageForTest(transport::protobuf::RoutingMessage& message, const std::string& src, const std::string& des);


kadmlia::RoutingTablePtr CreateGossipRootRoutingTableForTest(transport::TransportPtr transport, uint32_t size, const base::Config& config);

}  // namespace gossip

}  // namespace top
