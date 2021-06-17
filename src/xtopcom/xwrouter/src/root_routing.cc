// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/root/root_routing.h"

#include "xkad/proto/kadmlia.pb.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/node_detection_manager.h"
#include "xkad/routing_table/root_routing_table.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/top_string_util.h"
#include "xtransport/udp_transport/transport_util.h"
#include "xwrouter/register_message_handler.h"
#include "xwrouter/wrouter_utils/wrouter_utils.h"

namespace top {

using namespace kadmlia;

namespace wrouter {

static const int32_t kGetNodesTimeout = 3;
static const uint32_t kGetNodesSize = 8;

RootRouting::RootRouting(std::shared_ptr<transport::Transport> transport, kadmlia::LocalNodeInfoPtr local_node_ptr) : RootRoutingTable(transport, local_node_ptr) {
    WrouterRegisterMessageHandler(kKadFindNodesRequest,
                                  [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) { HandleFindNodesRequest(message, packet); });
    WrouterRegisterMessageHandler(kKadFindNodesResponse,
                                  [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) { HandleFindNodesResponse(message, packet); });
    WrouterRegisterMessageHandler(kKadHandshake, [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) { HandleHandshake(message, packet); });
    WrouterRegisterMessageHandler(kKadBootstrapJoinRequest,
                                  [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) { HandleBootstrapJoinRequest(message, packet); });
    WrouterRegisterMessageHandler(kKadBootstrapJoinResponse,
                                  [this](transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) { HandleBootstrapJoinResponse(message, packet); });
}

RootRouting::~RootRouting() {
    // WrouterUnregisterMessageHandler(kRootMessage);
    WrouterUnregisterMessageHandler(kKadFindNodesRequest);
    WrouterUnregisterMessageHandler(kKadFindNodesResponse);
    WrouterUnregisterMessageHandler(kKadHandshake);
    WrouterUnregisterMessageHandler(kKadBootstrapJoinRequest);
    WrouterUnregisterMessageHandler(kKadBootstrapJoinResponse);
}

bool RootRouting::Init() {
    if (!RootRoutingTable::Init()) {
        xerror("RootRoutingTable::Init failed");
        return false;
    }
    assert(get_local_node_info()->kadmlia_key()->xnetwork_id() == kRoot);
    get_local_node_info()->set_kadmlia_key(global_xid);

    xinfo("root routing table Init success");
    return true;
}

void RootRouting::HandleMessage(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (message.type() != kRootMessage) {
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        xwarn("connect request in data is empty.");
        return;
    }

    protobuf::RootMessage root_message;
    if (!root_message.ParseFromString(message.data())) {
        xwarn("ConnectRequest ParseFromString from string failed!");
        return;
    }

    switch (root_message.message_type()) {
    case kCompleteNodeRequest:
        return HandleFindNodesFromOthersRequest(message, packet);
    case kCompleteNodeResponse:
        return HandleFindNodesFromOthersResponse(message, packet);
    default:
        xwarn("invalid root message type[%d].", root_message.message_type());
        break;
    }
}

// get elect nodes from root-network base elect-data
int RootRouting::CacheElectNodesAsync(const std::string & des_kroot_id, base::ServiceType des_service_type, GetRootNodesV2AsyncCallback cb) {
    xdbg("bluerootasync routing start");
    transport::protobuf::RoutingMessage message;
    SetFreqMessage(message);
    message.set_is_root(true);
    message.set_des_service_type(kRoot);
    message.set_des_node_id(des_kroot_id);
    message.set_type(kRootMessage);
    message.set_id(CallbackManager::MessageId());
    message.set_xid(global_xid->Get());
#ifndef NDEBUG
    auto debug_info = base::StringUtil::str_fmt(
        "root routing get nodes, [id: %u] [src: %s], [des: %s] ", message.id(), get_local_node_info()->kad_key().c_str(), des_kroot_id.c_str());
    message.set_debug(debug_info);
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("root_get_nodes_begin", message);
#endif

    protobuf::RootCacheElectNodesRequest get_nodes_req;
    get_nodes_req.set_des_service_type(des_service_type.value());
    get_nodes_req.set_count(kGetNodesSize);
    std::string data;
    if (!get_nodes_req.SerializeToString(&data)) {
        xwarn("RootCacheElectNodesRequest SerializeToString failed!");
        return kKadFailed;
    }

    protobuf::RootMessage root_message;
    root_message.set_message_type(kCacheElectNodesRequest);
    root_message.set_data(data);
    std::string root_data;
    if (!root_message.SerializeToString(&root_data)) {
        xinfo("RootMessage SerializeToString failed!");
        return kKadFailed;
    }
    message.set_data(root_data);
    xdbg("%s bluerootasync sending message", transport::FormatMsgid(message).c_str());
    SendToClosestNode(message, false);

    using namespace std::placeholders;
    auto this_ptr = std::dynamic_pointer_cast<RootRouting>(shared_from_this());
    auto callback = std::bind(&RootRouting::OnCacheElectNodesAsync, this_ptr, cb, des_kroot_id, des_service_type, _1, _2, _3);
    CallbackManager::Instance()->Add(message.id(), kGetNodesTimeout, callback, 1);
    return kKadSuccess;
}

void RootRouting::OnCacheElectNodesAsync(GetRootNodesV2AsyncCallback cb,
                                         std::string des_kroot_id,
                                         base::ServiceType des_service_type,
                                         int status,
                                         transport::protobuf::RoutingMessage & message,
                                         base::xpacket_t & packet) {
    xdbg("%s bluerootasync routing callback", transport::FormatMsgid(message).c_str());
    if (status == kKadSuccess) {
        std::vector<kadmlia::NodeInfoPtr> nodes;
        do {
            if (!message.has_data() || message.data().empty()) {
                xwarn("%s message has no data!", transport::FormatMsgid(message).c_str());
                break;
            }
            protobuf::RootMessage get_nodes_res;
            if (!get_nodes_res.ParseFromString(message.data())) {
                xwarn("%s message ParseFromString failed!", transport::FormatMsgid(message).c_str());
                break;
            }
            if (!get_nodes_res.has_data() && get_nodes_res.data().empty()) {
                xwarn("%s message root message has no data!", transport::FormatMsgid(message).c_str());
                break;
            }
            protobuf::RootCacheElectNodesResponse nodes_res;
            if (!nodes_res.ParseFromString(get_nodes_res.data())) {
                xwarn("%s message root message ParseFromString!", transport::FormatMsgid(message).c_str());
                break;
            }
            for (int i = 0; i < nodes_res.nodes_size(); ++i) {
                if (base::GetKadmliaKey(nodes_res.nodes(i).id())->GetServiceType() != des_service_type) {
                    xdbg("[RootRouting::OnCacheElectNodesAsync] not this service type? des:%s get:%s",
                         des_service_type.info().c_str(),
                         base::GetKadmliaKey(nodes_res.nodes(i).id())->GetServiceType().info().c_str());
                    continue;
                }
                NodeInfoPtr node_ptr;
                node_ptr.reset(new NodeInfo(nodes_res.nodes(i).id()));
                node_ptr->public_ip = nodes_res.nodes(i).public_ip();
                node_ptr->public_port = nodes_res.nodes(i).public_port();
                node_ptr->xid = des_kroot_id;
                node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
                nodes.push_back(node_ptr);
            }

            cb(des_service_type, nodes);
            return;
        } while (0);
    } else {
        xdbg("%s OnCacheElectNodesAsync timeout", transport::FormatMsgid(message).c_str());
    }
}

static const int32_t kGFindNodesFromOthersTimeout = 3;
// todo make return type bool to some code
bool RootRouting::FindNodesFromOthers(base::ServiceType const & service_type,
                                      std::string const & election_xip2,
                                      OnCompleteElectRoutingTableCallback cb,
                                      base::KadmliaKeyPtr const & root_kad_key) {
    xdbg("[RootRouting::FindNodesFromOthers] service_type:%s election_xip2:%s kad_key:%s", service_type.info().c_str(), election_xip2.c_str(), root_kad_key->Get().c_str());
    if (node_id_map_.find(root_kad_key->Get()) != node_id_map_.end()) {
        xdbg("[RootRouting::FindNodesFromOthers] actually has this node check first.");
        return false;
    }
    transport::protobuf::RoutingMessage message;
    SetFreqMessage(message);
    message.set_is_root(true);
    message.set_des_service_type(kRoot);
    message.set_des_node_id(root_kad_key->Get());
    message.set_type(kRootMessage);
    message.set_id(CallbackManager::MessageId());
    message.set_xid(global_xid->Get());

    protobuf::RootMessage root_message;
    root_message.set_message_type(kCompleteNodeRequest);

    // root_message.set_data(data);
    std::string root_data;
    if (!root_message.SerializeToString(&root_data)) {
        xinfo("RootMessage SerializeToString failed!");
        return true;
    }
    message.set_data(root_data);

    SendToClosestNode(message, false);

    using namespace std::placeholders;
    auto callback = std::bind(&RootRouting::OnFindNodesFromOthers, this, service_type, election_xip2, cb, _1, _2, _3);
    CallbackManager::Instance()->Add(message.id(), kGFindNodesFromOthersTimeout, callback, 1);
    return true;
}

// root routing table used only
void RootRouting::OnFindNodesFromOthers(base::ServiceType const & service_type,
                                        std::string const & election_xip2,
                                        OnCompleteElectRoutingTableCallback cb,
                                        int status,
                                        transport::protobuf::RoutingMessage & message,
                                        base::xpacket_t & packet) {
    if (message.des_node_id() != get_local_node_info()->kad_key()) {
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        xwarn("%s message has no data!", transport::FormatMsgid(message).c_str());
        return;
    }
    protobuf::RootMessage root_message;
    if (!root_message.ParseFromString(message.data())) {
        xwarn("%s root_message ParseFromString failed!", transport::FormatMsgid(message).c_str());
        return;
    }
    if (!root_message.has_data() && root_message.data().empty()) {
        xwarn("%s root message has no data!", transport::FormatMsgid(message).c_str());
        return;
    }
    protobuf::RootCompleteNodeResponse get_nodes_res;
    if (!get_nodes_res.ParseFromString(root_message.data())) {
        xwarn("%s get_nodes_res message ParseFromString!", transport::FormatMsgid(message).c_str());
        return;
    }
    if (!get_nodes_res.has_nodes()) {
        xwarn("%s get_nodes_res message has no data!", transport::FormatMsgid(message).c_str());
        return;
    }

    auto target_node_ptr = get_nodes_res.nodes();
    kadmlia::NodeInfoPtr node_ptr;
    node_ptr.reset(new NodeInfo(election_xip2));
    node_ptr->public_ip = target_node_ptr.public_ip();
    node_ptr->public_port = target_node_ptr.public_port();
    xdbg("[RootRouting::OnFindNodesFromOthers] find node %s %s:%d", election_xip2.c_str(), node_ptr->public_ip.c_str(), node_ptr->public_port);
    cb(service_type, election_xip2, node_ptr);
}

// root routing table used_only
void RootRouting::HandleFindNodesFromOthersRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (!message.has_data() || message.data().empty()) {
        xwarn("[RootRouting::HandleFindNodesFromOthersRequest] HandleCacheElectNodesRequest has no data!");
        return;
    }

    if (node_id_map_.find(message.des_node_id()) != node_id_map_.end()) {
        auto target_node_info = node_id_map_.at(message.des_node_id());
        transport::protobuf::RoutingMessage res_message;
        res_message.set_is_root(true);
        res_message.set_des_node_id(message.src_node_id());
        res_message.set_type(kRootMessage);
        res_message.set_id(message.id());

        protobuf::RootCompleteNodeResponse get_nodes_res;
        protobuf::NodeInfo * node_info = get_nodes_res.mutable_nodes();
        node_info->set_public_ip(target_node_info->public_ip);
        node_info->set_public_port(target_node_info->public_port);
        std::string data;
        if (!get_nodes_res.SerializeToString(&data)) {
            xwarn("RootCacheElectNodesResponse SerializeToString failed!");
            return;
        }
        protobuf::RootMessage root_message;
        root_message.set_message_type(kCompleteNodeResponse);
        root_message.set_data(data);

        std::string root_data;
        if (!root_message.SerializeToString(&root_data)) {
            return;
        }
        res_message.set_data(root_data);

        SendToClosestNode(res_message);
    } else {
        xdbg("root routing continue sendtoclosest");
        SendToClosestNode(message);
    }
}
// root routing table used_only
void RootRouting::HandleFindNodesFromOthersResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (message.des_node_id() != get_local_node_info()->kad_key()) {
        return SendToClosestNode(message);
    }
    CallbackManager::Instance()->Callback(message.id(), message, packet);
}

}  // namespace wrouter

}  // namespace top
