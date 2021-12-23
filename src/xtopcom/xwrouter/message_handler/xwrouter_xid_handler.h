// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xbase/xrouter.h"
#include "xkad/routing_table/callback_manager.h"
#include "xwrouter/wrouter_utils/wrouter_utils.h"

#include <memory>

namespace top {

namespace transport {
class MultiThreadHandler;
class Transport;
typedef std::shared_ptr<Transport> TransportPtr;

namespace protobuf {
class RoutingMessage;
}
}  // namespace transport

namespace kadmlia {
class ElectRoutingTable;
typedef std::shared_ptr<ElectRoutingTable> ElectRoutingTablePtr;
class RootRoutingTable;
typedef std::shared_ptr<RootRoutingTable> RootRoutingTablePtr;
struct NodeInfo;
typedef std::shared_ptr<NodeInfo> NodeInfoPtr;
}  // namespace kadmlia

namespace gossip {
class GossipInterface;
}

namespace wrouter {

class WrouterXidHandler {
public:
    WrouterXidHandler(transport::TransportPtr transport_ptr,
                      std::shared_ptr<gossip::GossipInterface> bloom_gossip_ptr,
                      std::shared_ptr<gossip::GossipInterface> bloom_layer_gossip_ptr,
                      std::shared_ptr<gossip::GossipInterface> gossip_rrs_ptr,
                      std::shared_ptr<gossip::GossipInterface> gossip_dispatcher_ptr);

    ~WrouterXidHandler();

public:
    void SendPacket(transport::protobuf::RoutingMessage & message, std::error_code & ec);
    int32_t RecvPacket(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);

private:
    kadmlia::RootRoutingTablePtr FindRootRoutingTable();

    kadmlia::ElectRoutingTablePtr FindElectRoutingTable(base::ServiceType service_type);

    base::ServiceType ParserServiceType(const std::string & kad_key);

    bool RumorPacketCheck(transport::protobuf::RoutingMessage & message);
    bool BroadcastPacketCheck(transport::protobuf::RoutingMessage & message);

    void SendRumor(transport::protobuf::RoutingMessage & message, std::error_code & ec);
    void SendBroadcast(transport::protobuf::RoutingMessage & message, std::error_code & ec);
    void SendGeneral(transport::protobuf::RoutingMessage & message, std::error_code & ec);

    // judge packet arrive the dest or not
    int32_t JudgeOwnPacket(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);

    // int32_t GossipBroadcast(transport::protobuf::RoutingMessage & message, kadmlia::RoutingTablePtr & routing_table);
    void SendData(transport::protobuf::RoutingMessage & message, const std::vector<kadmlia::NodeInfoPtr> & neighbors, uint32_t next_size, bool broadcast_stride, std::error_code & ec);
    bool HandleSystemMessage(transport::protobuf::RoutingMessage & message);

private:
    DISALLOW_COPY_AND_ASSIGN(WrouterXidHandler);

    transport::TransportPtr transport_ptr_;
    std::shared_ptr<gossip::GossipInterface> bloom_gossip_ptr_;
    std::shared_ptr<gossip::GossipInterface> bloom_layer_gossip_ptr_;
    std::shared_ptr<gossip::GossipInterface> gossip_rrs_ptr_;
    std::shared_ptr<gossip::GossipInterface> gossip_dispatcher_ptr_;
};

}  // namespace wrouter

}  // namespace top
