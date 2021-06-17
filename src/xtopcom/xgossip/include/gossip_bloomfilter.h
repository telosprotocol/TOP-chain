// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xgossip/gossip_interface.h"
#include "xtransport/transport.h"

namespace top {

namespace gossip {

class GossipBloomfilter : public GossipInterface {
public:
    explicit GossipBloomfilter(transport::TransportPtr transport_ptr);
    virtual ~GossipBloomfilter();
    void Broadcast(uint64_t local_hash64, transport::protobuf::RoutingMessage & message, std::shared_ptr<std::vector<kadmlia::NodeInfoPtr>> neighbors) override;

private:
    DISALLOW_COPY_AND_ASSIGN(GossipBloomfilter);
};

}  // namespace gossip

}  // namespace top
