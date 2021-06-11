// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/message_handler/xwrouter_handler.h"

#include "xbase/xutl.h"
#include "xgossip/gossip_interface.h"
#include "xgossip/include/gossip_bloomfilter.h"
#include "xgossip/include/gossip_utils.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/uint64_bloomfilter.h"
#include "xpbase/base/xip_parser.h"
#include "xtransport/utils/transport_utils.h"
#include "xwrouter/message_handler/wrouter_message_handler.h"
#include "xwrouter/multi_routing/multi_routing.h"

#include <algorithm>

namespace top {

using namespace kadmlia;
using namespace gossip;

namespace wrouter {

WrouterHandler::WrouterHandler(transport::TransportPtr transport_ptr,
                               std::shared_ptr<gossip::GossipInterface> bloom_gossip_ptr,
                               std::shared_ptr<gossip::GossipInterface> bloom_layer_gossip_ptr,
                               std::shared_ptr<gossip::GossipInterface> gossip_rrs_ptr,
                               std::shared_ptr<gossip::GossipInterface> gossip_dispatcher_ptr)
  : transport_ptr_(transport_ptr)
  , bloom_gossip_ptr_(bloom_gossip_ptr)
  , bloom_layer_gossip_ptr_(bloom_layer_gossip_ptr)
  , gossip_rrs_ptr_(gossip_rrs_ptr)
  , gossip_dispatcher_ptr_(gossip_dispatcher_ptr) {
}

WrouterHandler::~WrouterHandler() {
    transport_ptr_ = nullptr;
    bloom_gossip_ptr_ = nullptr;
    bloom_layer_gossip_ptr_ = nullptr;
    gossip_rrs_ptr_ = nullptr;
}

kadmlia::ElectRoutingTablePtr WrouterHandler::FindElectRoutingTable(base::ServiceType service_type) {
    return MultiRouting::Instance()->GetElectRoutingTable(service_type);
}
kadmlia::RootRoutingTablePtr WrouterHandler::FindRootRoutingTable() {
    return MultiRouting::Instance()->GetRootRoutingTable();
}

}  // namespace wrouter

}  // namespace top
