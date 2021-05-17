// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <stdint.h>
#include <memory>
#include <vector>

#include "xtransport/transport.h"
#include "xgossip/gossip_interface.h"
#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_table.h"
#include "xpbase/base/top_utils.h"

namespace top {
namespace base {
class Uint64BloomFilter;
}  // namespace base
namespace transport {
namespace protobuf {
class RoutingMessage;
}  // namespace protobuf
}  // namespace transport

namespace gossip {

class GossipBloomfilterLayer : public GossipInterface {
public:
    explicit GossipBloomfilterLayer(transport::TransportPtr transport_ptr);
    virtual ~GossipBloomfilterLayer();
    virtual void Broadcast(
            uint64_t local_hash64,
            transport::protobuf::RoutingMessage& message,
            std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> neighbors);
    virtual void Broadcast(
            transport::protobuf::RoutingMessage& message,
            kadmlia::RoutingTablePtr& routing_table);

private:
    void SelectNodes(transport::protobuf::RoutingMessage & message,
                     kadmlia::RoutingTablePtr & routing_table,
                     std::shared_ptr<base::Uint64BloomFilter> & bloomfilter,
                     std::vector<kadmlia::NodeInfoPtr> & select_nodes);
    DISALLOW_COPY_AND_ASSIGN(GossipBloomfilterLayer);
};

}  // namespace gossip

}  // namespace top
