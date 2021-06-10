// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/root/root_routing.h"

#include "xkad/proto/kadmlia.pb.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/node_detection_manager.h"
#include "xkad/routing_table/root_routing_table.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xpbase/base/sem.h"
#include "xpbase/base/top_string_util.h"
#include "xtransport/udp_transport/transport_util.h"
#include "xwrouter/register_message_handler.h"
#include "xwrouter/wrouter_utils/wrouter_utils.h"

namespace top {

using namespace kadmlia;

namespace wrouter {

static const int32_t kGetNodesTimeout = 3;
static const uint32_t kGetNodesSize = 8;

RootRouting::RootRouting(std::shared_ptr<transport::Transport> transport, kadmlia::LocalNodeInfoPtr local_node_ptr)
  : RootRoutingTable(transport, local_node_ptr), root_id_set_mutex_() {
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
    get_local_node_info()->set_is_root(true);
    if (!RootRoutingTable::Init()) {
        TOP_ERROR("RootRoutingTable::Init failed");
        return false;
    }
    assert(get_local_node_info()->kadmlia_key()->xnetwork_id() == kRoot);
    get_local_node_info()->set_kadmlia_key(global_xid);

    // AddNetworkRootId(get_local_node_info()->id());

    TOP_INFO("root routing table Init success");
    return true;
}

void RootRouting::HandleMessage(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (message.type() != kRootMessage) {
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        TOP_WARN("connect request in data is empty.");
        return;
    }

    protobuf::RootMessage root_message;
    if (!root_message.ParseFromString(message.data())) {
        TOP_WARN("ConnectRequest ParseFromString from string failed!");
        return;
    }

    switch (root_message.message_type()) {
    case kGetNodesRequest:
        return HandleFindNodesFromOthersRequest(message, packet);
    case kGetNodesResponse:
        return HandleFindNodesFromOthersResponse(message, packet);
    case kGetElectNodesRequest:
        return HandleGetElectNodesRequest(message, packet);
    case kGetElectNodesResponse:
        return HandleGetElectNodesResponse(message, packet);
    default:
        TOP_WARN("invalid root message type[%d].", root_message.message_type());
        break;
    }
}

// get elect nodes from root-network base elect-data
int RootRouting::GetRootNodesV2Async(const std::string & des_kroot_id, base::ServiceType des_service_type, GetRootNodesV2AsyncCallback cb) {
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
        "root routing get nodes, [id: %u] [src: %s], [des: %s] ", message.id(), HexEncode(get_local_node_info()->id()).c_str(), HexEncode(des_kroot_id).c_str());
    message.set_debug(debug_info);
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("root_get_nodes_begin", message);
#endif

    protobuf::RootGetElectNodesRequest get_nodes_req;
    get_nodes_req.set_des_service_type(des_service_type.value());
    get_nodes_req.set_count(kGetNodesSize);
    std::string data;
    if (!get_nodes_req.SerializeToString(&data)) {
        TOP_WARN("RootGetElectNodesRequest SerializeToString failed!");
        return kKadFailed;
    }

    protobuf::RootMessage root_message;
    root_message.set_message_type(kGetElectNodesRequest);
    root_message.set_data(data);
    std::string root_data;
    if (!root_message.SerializeToString(&root_data)) {
        TOP_INFO("RootMessage SerializeToString failed!");
        return kKadFailed;
    }
    message.set_data(root_data);
    xdbg("%s bluerootasync sending message", transport::FormatMsgid(message).c_str());
    SendToClosestNode(message, false);

    using namespace std::placeholders;
    auto this_ptr = std::dynamic_pointer_cast<RootRouting>(shared_from_this());
    auto callback = std::bind(&RootRouting::OnGetRootNodesV2Async, this_ptr, cb, des_kroot_id, des_service_type, _1, _2, _3);
    CallbackManager::Instance()->Add(message.id(), kGetNodesTimeout, callback, 1);
    return kKadSuccess;
}

void RootRouting::OnGetRootNodesV2Async(GetRootNodesV2AsyncCallback cb,
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
                TOP_WARN("%s message has no data!", transport::FormatMsgid(message).c_str());
                break;
            }
            protobuf::RootMessage get_nodes_res;
            if (!get_nodes_res.ParseFromString(message.data())) {
                TOP_WARN("%s message ParseFromString failed!", transport::FormatMsgid(message).c_str());
                break;
            }
            if (!get_nodes_res.has_data() && get_nodes_res.data().empty()) {
                TOP_WARN("%s message root message has no data!", transport::FormatMsgid(message).c_str());
                break;
            }
            protobuf::RootGetNodesResponse nodes_res;
            if (!nodes_res.ParseFromString(get_nodes_res.data())) {
                TOP_WARN("%s message root message ParseFromString!", transport::FormatMsgid(message).c_str());
                break;
            }
            for (int i = 0; i < nodes_res.nodes_size(); ++i) {
                if (base::GetKadmliaKey(nodes_res.nodes(i).id())->GetServiceType() != des_service_type) {
                    xdbg("Charles Debug not this service type? des:%s get:%s",
                         des_service_type.info().c_str(),
                         base::GetKadmliaKey(nodes_res.nodes(i).id())->GetServiceType().info().c_str());
                    continue;
                }
                NodeInfoPtr node_ptr;
                node_ptr.reset(new NodeInfo(nodes_res.nodes(i).id()));
                node_ptr->public_ip = nodes_res.nodes(i).public_ip();
                node_ptr->public_port = nodes_res.nodes(i).public_port();
                node_ptr->local_ip = nodes_res.nodes(i).local_ip();
                node_ptr->local_port = nodes_res.nodes(i).local_port();
                node_ptr->xid = des_kroot_id;
                node_ptr->hash64 = base::xhash64_t::digest(node_ptr->node_id);
                nodes.push_back(node_ptr);
            }

            cb(des_service_type, nodes);
            return;
        } while (0);
    } else {
        TOP_DEBUG("%s OnGetRootNodesV2Async timeout", transport::FormatMsgid(message).c_str());
    }
}

void RootRouting::HandleGetElectNodesRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    assert(false);
    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("handle request", message);
    if (message.des_node_id() != get_local_node_info()->id()) {
        bool closest = false;
        if (ClosestToTarget(message.des_node_id(), closest) != kKadSuccess) {
            TOP_WARN("root routing closesttotarget goes wrong");
            return;
        }
        if (!closest) {
            TOP_DEBUG("root routing continue sendtoclosest");
            return SendToClosestNode(message);
        }
        TOP_INFO("this is the closest node(%s) of msg.des_node_id(%s)", HexEncode(get_local_node_info()->id()).c_str(), HexEncode(message.des_node_id()).c_str());
    } else {
        TOP_DEBUG("this is the des node(%s)", HexEncode(get_local_node_info()->id()).c_str());
    }

    if (!message.has_data() || message.data().empty()) {
        TOP_WARN("HandleGetElectNodesRequest has no data!");
        return;
    }

    protobuf::RootMessage root_message;
    if (!root_message.ParseFromString(message.data())) {
        TOP_WARN("RootMessage ParseFromString from string failed!");
        return;
    }

    protobuf::RootGetElectNodesRequest get_nodes_req;
    if (!get_nodes_req.ParseFromString(root_message.data())) {
        TOP_WARN("RootGetElectNodesRequest ParseFromString failed!");
        return;
    }
    base::ServiceType des_service_type = base::ServiceType(get_nodes_req.des_service_type());

    // todo charles check this .
#if 0
    auto routing_table = GetRoutingTable(des_service_type, false);
    
    std::vector<NodeInfoPtr> nodes;
    if (!routing_table) {
        TOP_WARN("GetRoutingTable failed for service_type:%llu", des_service_type.value());
        return;
    }
    auto local_node_ptr = routing_table->get_local_node_info();
    if (!local_node_ptr) {
        assert(false);
    }
    nodes = routing_table->GetClosestNodes(local_node_ptr->id(), get_nodes_req.count() - 1);

    transport::protobuf::RoutingMessage res_message;
#    ifndef NDEBUG
    if (message.has_debug()) {
        res_message.set_debug(message.debug());
    }
#    endif

    SetFreqMessage(res_message);
    res_message.set_src_service_type(message.des_service_type());
    res_message.set_des_service_type(kRoot);
    res_message.set_des_node_id(message.src_node_id());
    res_message.set_type(kRootMessage);
    res_message.set_id(message.id());
    protobuf::RootGetElectNodesResponse get_nodes_res;
    if (local_node_ptr->public_port() > 0) {
        protobuf::NodeInfo* node_info = get_nodes_res.add_nodes();
        node_info->set_id(local_node_ptr->id());
        node_info->set_public_ip(local_node_ptr->public_ip());
        node_info->set_public_port(local_node_ptr->public_port());
        node_info->set_local_ip(local_node_ptr->local_ip());
        node_info->set_local_port(local_node_ptr->local_port());
    } else {
        TOP_WARN("public_port invalid: %d of this node:%s",
                local_node_ptr->public_port(),
                HexEncode(local_node_ptr->id()).c_str());
    }

    auto tmp_ready_nodes = 0;
    for (uint32_t i = 0; i < nodes.size(); ++i) {
        if (static_cast<uint32_t>(get_nodes_res.nodes_size()) >= get_nodes_req.count()) {
            break;
        }

        if (nodes[i]->node_id == message.des_node_id()) {
            continue;
        }
        if (nodes[i]->xid == message.xid()) {
            continue;
        }
        if (nodes[i]->public_port <= 0) {
            continue;
        }
        auto tmp_kad_key = base::GetKadmliaKey(nodes[i]->node_id);
        if (tmp_kad_key->GetServiceType() != des_service_type) {
            continue;
        }
        protobuf::NodeInfo* node_info = get_nodes_res.add_nodes();
        node_info->set_id(nodes[i]->node_id);
        node_info->set_public_ip(nodes[i]->public_ip);
        node_info->set_public_port(nodes[i]->public_port);
        node_info->set_local_ip(nodes[i]->local_ip);
        node_info->set_local_port(nodes[i]->local_port);
        ++tmp_ready_nodes;
    }
    TOP_DEBUG("nodes:%d ready_nodes:%d filtered:%d", nodes.size(), tmp_ready_nodes, nodes.size()-tmp_ready_nodes);

    std::string data;
    if (!get_nodes_res.SerializeToString(&data)) {
        TOP_WARN("RootGetElectNodesResponse SerializeToString failed!");
        return;
    }

    TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE(
            std::string("response: ") + std::to_string(get_nodes_res.nodes_size()),
            message);
    protobuf::RootMessage root_res_message;
    root_res_message.set_message_type(kGetElectNodesResponse);
    root_res_message.set_data(data);
    std::string root_data;
    if (!root_res_message.SerializeToString(&root_data)) {
        TOP_WARN("RootMessage SerializeToString failed!");
        return;
    }

    res_message.set_data(root_data);

    TOP_DEBUG("send response of msg.des: %s size: %d",
              HexEncode(message.des_node_id()).c_str(),
              tmp_ready_nodes);
    SendToClosestNode(res_message);
#endif
    return;
}

void RootRouting::HandleGetElectNodesResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    assert(false);
    if (message.des_node_id() != get_local_node_info()->id()) {
        return SendToClosestNode(message);
    }

    TOP_DEBUG("response arrive");
    CallbackManager::Instance()->Callback(message.id(), message, packet);
}

static const int32_t kGFindNodesFromOthersTimeout = 3;
// todo make return type bool to some code
bool RootRouting::FindNodesFromOthers(base::ServiceType const & service_type,
                                      std::string const & election_xip2,
                                      OnCompleteElectRoutingTableCallback cb,
                                      base::KadmliaKeyPtr const & root_kad_key) {
    xdbg("Charles Debug RootRouting::FindNodesFromOthers service_type:%s election_xip2:%s kad_key:%s",
         service_type.info().c_str(),
         election_xip2.c_str(),
         root_kad_key->Get().c_str());
    if (node_id_map_.find(root_kad_key->Get()) != node_id_map_.end()) {
        xdbg("Charles Debug RootRouting actually has this node check first.");
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
    root_message.set_message_type(kGetNodesRequest);

    // root_message.set_data(data);
    std::string root_data;
    if (!root_message.SerializeToString(&root_data)) {
        TOP_INFO("RootMessage SerializeToString failed!");
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
    if (message.des_node_id() != get_local_node_info()->id()) {
        return;
    }

    if (!message.has_data() || message.data().empty()) {
        TOP_WARN("%s message has no data!", transport::FormatMsgid(message).c_str());
        return;
    }
    protobuf::RootMessage root_message;
    if (!root_message.ParseFromString(message.data())) {
        TOP_WARN("%s root_message ParseFromString failed!", transport::FormatMsgid(message).c_str());
        return;
    }
    if (!root_message.has_data() && root_message.data().empty()) {
        TOP_WARN("%s root message has no data!", transport::FormatMsgid(message).c_str());
        return;
    }
    protobuf::RootGetNodesResponse get_nodes_res;
    if (!get_nodes_res.ParseFromString(root_message.data())) {
        TOP_WARN("%s get_nodes_res message ParseFromString!", transport::FormatMsgid(message).c_str());
        return;
    }
    if (!get_nodes_res.nodes_size()) {
        TOP_WARN("%s get_nodes_res message has no data!", transport::FormatMsgid(message).c_str());
        return;
    }

    auto target_node_ptr = get_nodes_res.nodes(0);
    kadmlia::NodeInfoPtr node_ptr;
    node_ptr.reset(new NodeInfo(election_xip2));  // todo charles if this right?
    node_ptr->public_ip = target_node_ptr.public_ip();
    node_ptr->public_port = target_node_ptr.public_port();
    xdbg("Charles Debug RoutingTable::OnFindNodesFromOthers find node %s %s:%d", election_xip2.c_str(), node_ptr->public_ip.c_str(), node_ptr->public_port);
    cb(service_type, election_xip2, node_ptr);

    // cb(service_type, election_xip2, node_info);
}

// root routing table used_only
void RootRouting::HandleFindNodesFromOthersRequest(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (!message.has_data() || message.data().empty()) {
        TOP_WARN("HandleGetElectNodesRequest has no data!");
        return;
    }

    if (node_id_map_.find(message.des_node_id()) != node_id_map_.end()) {
        auto target_node_info = node_id_map_.at(message.des_node_id());
        transport::protobuf::RoutingMessage res_message;
        res_message.set_is_root(true);
        res_message.set_des_node_id(message.src_node_id());
        res_message.set_type(kRootMessage);
        res_message.set_id(message.id());

        protobuf::RootGetElectNodesResponse get_nodes_res;
        protobuf::NodeInfo * node_info = get_nodes_res.add_nodes();
        node_info->set_public_ip(target_node_info->public_ip);
        node_info->set_public_port(target_node_info->public_port);
        std::string data;
        if (!get_nodes_res.SerializeToString(&data)) {
            TOP_WARN("RootGetElectNodesResponse SerializeToString failed!");
            return;
        }
        protobuf::RootMessage root_message;
        root_message.set_message_type(kGetNodesResponse);
        root_message.set_data(data);

        std::string root_data;
        if (!root_message.SerializeToString(&root_data)) {
            return;
        }
        res_message.set_data(root_data);

        SendToClosestNode(res_message);
    } else {
        TOP_DEBUG("root routing continue sendtoclosest");
        SendToClosestNode(message);
    }
}
// root routing table used_only
void RootRouting::HandleFindNodesFromOthersResponse(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (message.des_node_id() != get_local_node_info()->id()) {
        return SendToClosestNode(message);
    }
    CallbackManager::Instance()->Callback(message.id(), message, packet);
}

}  // namespace wrouter

}  // namespace top
