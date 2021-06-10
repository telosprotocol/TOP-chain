// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xkad/routing_table/root_routing_table.h"
#include "xpbase/base/top_config.h"
#include "xpbase/base/top_utils.h"
#include "xtransport/transport.h"
#include "xwrouter/root/root_message_handler.h"
#include "xwrouter/wrouter_utils/wrouter_utils.h"

namespace top {

namespace wrouter {

enum RootMessageType {
    kGetNodesRequest = 1,
    kGetNodesResponse = 2,
    kGetElectNodesRequest = 3,
    kGetElectNodesResponse = 4,
};

class RootRouting : public kadmlia::RootRoutingTable {
public:
    RootRouting(std::shared_ptr<transport::Transport>, kadmlia::LocalNodeInfoPtr);
    ~RootRouting() override;
    bool Init();
    void HandleMessage(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
    using GetRootNodesV2AsyncCallback = std::function<void(base::ServiceType, const std::vector<kadmlia::NodeInfoPtr> &)>;
    int GetRootNodesV2Async(const std::string & des_kroot_id, base::ServiceType des_service_type, GetRootNodesV2AsyncCallback cb);

private:
    void HandleGetElectNodesRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
    void HandleGetElectNodesResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);

    void OnGetRootNodesV2Async(GetRootNodesV2AsyncCallback cb,
                               std::string des_kroot_id,
                               base::ServiceType des_service_type,
                               int status,
                               transport::protobuf::RoutingMessage & message,
                               base::xpacket_t & packet);

    std::mutex root_id_set_mutex_;

    DISALLOW_COPY_AND_ASSIGN(RootRouting);

public:
    using OnCompleteElectRoutingTableCallback = std::function<void(base::ServiceType const, std::string const, kadmlia::NodeInfoPtr const &)>;
    // void FindNodesFromOthers(OnCompleteElectRoutingTableCallback cb, base::KadmliaKeyPtr const & root_kad_key);
    bool FindNodesFromOthers(base::ServiceType const & service_type,
                             std::string const & election_xip2,
                             OnCompleteElectRoutingTableCallback cb,
                             base::KadmliaKeyPtr const & root_kad_key);
    void OnFindNodesFromOthers(base::ServiceType const & service_type,
                               std::string const & election_xip2,
                               OnCompleteElectRoutingTableCallback cb,
                               int status,
                               transport::protobuf::RoutingMessage & message,
                               base::xpacket_t & packet);

    void HandleFindNodesFromOthersRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
    void HandleFindNodesFromOthersResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);
};

}  // namespace wrouter

}  // namespace top
