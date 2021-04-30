// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/message_handler/wrouter_message_handler.h"

#include <vector>
#include <string>
#include <utility>
#include <map>

#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/top_log.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/routing_table/callback_manager.h"
#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_table.h"
#include "xkad/routing_table/node_detection_manager.h"
#include "xkad/routing_table/client_node_manager.h"
#include "xkad/routing_table/local_node_info.h"
#include "xwrouter/register_routing_table.h"
#include "xpbase/base/multirelay_log.h"
#include "xtransport/udp_transport/transport_util.h"

namespace top {

using namespace kadmlia;  // NOLINT

namespace wrouter {

WrouterMessageHandler::WrouterMessageHandler() {
    AddBaseHandlers();
}

WrouterMessageHandler::~WrouterMessageHandler() {
    for (uint32_t index = 0; index < array_handlers_.size(); ++index) {
        array_handlers_[index] = nullptr;
    }
}

WrouterMessageHandler* WrouterMessageHandler::Instance() {
    static WrouterMessageHandler ins;
    return &ins;
}

void WrouterMessageHandler::HandleMessage(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    if (message.hop_num() >= kHopToLive) {
        std::string nodes;
        for (int i = 0; i < message.hop_nodes_size(); ++i) {
            nodes += HexSubstr(message.hop_nodes(i).node_id()) + " -> ";
        }
        TOP_WARN("stop send msg because hop to live is max: %d [%s] des[%s] "
             "message_type[%d] nodes[%s]",
             kHopToLive,
             HexSubstr(message.src_node_id()).c_str(),
             HexSubstr(message.des_node_id()).c_str(),
             message.type(), nodes.c_str());
        return;
    }

    //TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter handle message", message);
    CheckBitVPNClientMessage(message);
    CheckNatDetectMessage(message);

    std::string version;
    if (message.has_version_tag()) {
        transport::protobuf::VersionTag version_tag = message.version_tag();
        version = version_tag.version();
    }

    if (message.type() >= MsgHandlerMaxSize) {
        TOP_WARN("invalid message.type(%d), beyond %d", message.type(), MsgHandlerMaxSize);
        return;
    }
    transport::HandlerProc call = array_handlers_[message.type()];
    if (!call) {
        TOP_WARN("invalid message.type(%d), callback not registered", message.type());
        return;
    }
    //TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE("wrouter func called", message);
    call(message, packet);
}

void WrouterMessageHandler::HandleSyncMessage(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    TOP_DEBUG("HandleSyncMessage msgtype:%d", message.type());
    transport::HandlerProc call = array_handlers_[message.type()];
    if (!call) {
        TOP_WARN("invalid message.type(%d), callback not registered", message.type());
        return;
    }
    call(message, packet);
}

void WrouterMessageHandler::AddBaseHandlers() {
    AddHandler(kKadConnectRequest, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        HandleConnectRequest(message, packet);
    });
    AddHandler(kKadDropNodeRequest, [this](
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet) {
        HandleNodeQuit(message, packet);
    });
    AddHandler(kKadHandshake, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        HandleHandshake(message, packet);
    });
    AddHandler(kKadBootstrapJoinRequest, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        HandleBootstrapJoinRequest(message, packet);
    });
    AddHandler(kKadBootstrapJoinResponse, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        HandleBootstrapJoinResponse(message, packet);
    });
    AddHandler(kKadFindNodesRequest, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        HandleFindNodesRequest(message, packet);
    });
    AddHandler(kKadFindNodesResponse, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        HandleFindNodesResponse(message, packet);
    });
    AddHandler(kKadHeartbeatRequest, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        HandleHeartbeatRequest(message, packet);
    });
    AddHandler(kKadHeartbeatResponse, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        HandleHeartbeatResponse(message, packet);
    });
    AddHandler(kKadAck, [](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
    });
    AddHandler(kKadNatDetectRequest, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        nat_manager_->PushMessage(message, packet);
    });
    AddHandler(kKadNatDetectResponse, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        nat_manager_->PushMessage(message, packet);
    });
    AddHandler(kKadNatDetectHandshake2Node, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        nat_manager_->PushMessage(message, packet);
    });
    AddHandler(kKadNatDetectHandshake2Boot, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        nat_manager_->PushMessage(message, packet);
    });
    AddHandler(kKadNatDetectFinish, [this](transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet){
        nat_manager_->PushMessage(message, packet);
    });
}

void WrouterMessageHandler::HandleNodeQuit(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = GetRoutingTable(
            message.des_service_type(),
            message.has_is_root() && message.is_root());
    if (!routing_table) {
        TOP_WARN("service type[%llu] [%d] has not register routing table.",
            message.des_service_type(), message.is_root());
        return;
    }
    routing_table->HandleNodeQuit(message, packet);
}


void WrouterMessageHandler::AddHandler(int msg_type, transport::HandlerProc handler_proc) {
    assert(handler_proc);
    assert(msg_type < MsgHandlerMaxSize);
    assert(!array_handlers_[msg_type]);

    array_handlers_[msg_type] = handler_proc;

}

void WrouterMessageHandler::AddRequestType(int msg_type, int request_type) {
    if (request_type != kRequestMsg && request_type != kResponseMsg) {
        request_type = kNone;
    }
    std::unique_lock<std::mutex> lock(map_request_type_mutex_);
    map_request_type_[msg_type] = request_type;  // just cover
}

int WrouterMessageHandler::GetRequestType(int msg_type) {
    std::unique_lock<std::mutex> lock(map_request_type_mutex_);
    auto it = map_request_type_.find(msg_type);
    if (it != map_request_type_.end()) {
        return it->second;
    }
    return kNone;
}

void WrouterMessageHandler::RemoveHandler(int msg_type) {
    assert(msg_type < MsgHandlerMaxSize);
    array_handlers_[msg_type] = nullptr;
}

void WrouterMessageHandler::RemoveRequestType(int msg_type) {
    std::unique_lock<std::mutex> lock(map_request_type_mutex_);
    auto it = map_request_type_.find(msg_type);
    if (it != map_request_type_.end()) {
        map_request_type_.erase(it);
        return;
    }
}

// check bitvpn 0.5.0(just for now) message 
void WrouterMessageHandler::CheckBitVPNClientMessage(
        transport::protobuf::RoutingMessage& message) {
    std::string version;
    if (!message.has_version_tag()) {
        return;
    }

    transport::protobuf::VersionTag version_tag = message.version_tag();
    version = version_tag.version();
    if (version.compare("0.5.0") != 0)  {
        return;
    }

    if (!message.has_src_service_type() || !message.has_des_service_type()){
        // usually this is the first node which recv client msg,meaning node is the relay node
        message.set_src_service_type(top::kEdgeXVPN);
        message.set_des_service_type(top::kEdgeXVPN);
        message.set_client_id(message.src_node_id());
        message.set_relay_flag(false);
        TOP_DEBUG("client version 0.5.0 msg come, set service_type %d, "
                "set client_id and relay_flag",
                top::kEdgeXVPN);
    }
}

void WrouterMessageHandler::CheckNatDetectMessage(transport::protobuf::RoutingMessage& message) {
    switch (message.type()) {
    case kKadNatDetectRequest:
    case kKadNatDetectResponse:
    case kKadNatDetectHandshake2Node:
    case kKadNatDetectHandshake2Boot:
    case kKadNatDetectFinish:
        message.set_src_service_type(top::kRoot);
        message.set_des_service_type(top::kRoot);
        TOP_DEBUG("bluenat nat detect set to kRoot", top::kRoot);
        break;
    }
}

void WrouterMessageHandler::HandleHeartbeatRequest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = GetRoutingTable(
        message.des_node_id(),
        message.has_is_root() && message.is_root());
    if (!routing_table) {
        TOP_WARN("heartbeat msg.des[%s], msg.is_root[%d] has not register routing table.",
            HexEncode(message.des_node_id()).c_str(), message.is_root());
        return;
    }
    routing_table->HandleHeartbeatRequest(message, packet);
}

void WrouterMessageHandler::HandleHeartbeatResponse(
    transport::protobuf::RoutingMessage& message,
    base::xpacket_t& packet) {
    RoutingTablePtr routing_table = GetRoutingTable(
        message.des_node_id(),
        message.has_is_root() && message.is_root());
    if (!routing_table) {
        TOP_WARN("heartbeat msg.des[%s], msg.is_root[%d] has not register routing table.",
            HexEncode(message.des_node_id()).c_str(), message.is_root());
        return;
    }
    routing_table->HandleHeartbeatResponse(message, packet);
}

void WrouterMessageHandler::HandleFindNodesRequest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    // TODO(smaug) handle kRoot parse
    RoutingTablePtr routing_table = GetRoutingTable(
        message.des_node_id(),
        message.has_is_root() && message.is_root());
    if (!routing_table) {
        TOP_WARN2("HandleFindNodesRequest msg.is_root(%d) msg.src_node_id(%s) msg.des_node_id(%s)",
                message.is_root(),
                HexEncode(message.src_node_id()).c_str(),
                HexEncode(message.des_node_id()).c_str());
        return;
    }
    routing_table->HandleFindNodesRequest(message, packet);
}

void WrouterMessageHandler::HandleFindNodesResponse(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = GetRoutingTable(
        message.des_node_id(),
        message.has_is_root() && message.is_root());
    if (!routing_table) {
        TOP_WARN2("HandleFindNodesResponse msg.is_root(%d) msg.src_node_id(%s) msg.des_node_id(%s)",
                message.is_root(),
                HexEncode(message.src_node_id()).c_str(),
                HexEncode(message.des_node_id()).c_str());

        return;
    }
    routing_table->HandleFindNodesResponse(message, packet);
}

int WrouterMessageHandler::SendData(
        const transport::protobuf::RoutingMessage& message,
        const std::string& peer_ip,
        uint16_t peer_port) {
    RoutingTablePtr routing_table = GetRoutingTable(
        message.des_service_type(),
        message.has_is_root() && message.is_root());
    if (!routing_table) {
        TOP_WARN("routing table not registered[%llu]", message.des_service_type());
        return kKadFailed;
    }

    auto transport_ptr = routing_table->get_transport();
    if (!transport_ptr) {
        TOP_WARN("service type[%llu] has not register udp transport.", message.des_service_type());
        return kKadFailed;
    }

    std::string msg;
    if (!message.SerializeToString(&msg)) {
        TOP_INFO("RoutingMessage SerializeToString failed!");
        return kKadFailed;
    }
    xbyte_buffer_t xdata{msg.begin(), msg.end()};

    return transport_ptr->SendData(xdata, peer_ip, peer_port);
}

void WrouterMessageHandler::HandleBootstrapJoinRequest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = GetRoutingTable(
        message.des_service_type(),
        message.has_is_root() && message.is_root());
    if (!routing_table) {
        TOP_WARN("service type[%llu][%d] has not register routing table.",
                message.des_service_type(), message.is_root());
        return;
    }
    routing_table->HandleBootstrapJoinRequest(message, packet);
}

void WrouterMessageHandler::HandleBootstrapJoinResponse(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    TOP_DEBUG("join response coming,[%d]", message.is_root());
    RoutingTablePtr routing_table = GetRoutingTable(
        message.des_service_type(),
        message.has_is_root() && message.is_root());
    if (!routing_table) {
        TOP_WARN("service type[%llu] has not register routing table.", message.des_service_type());
        return;
    }
    routing_table->HandleBootstrapJoinResponse(message, packet);
}

void WrouterMessageHandler::HandleHandshake(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    RoutingTablePtr routing_table = GetRoutingTable(
        message.des_node_id(),
        message.has_is_root() && message.is_root());
    if (!routing_table) {
        TOP_WARN2("HandleHandshake msg.is_root(%d) msg.src_node_id(%s) msg.des_node_id(%s)",
                message.is_root(),
                HexEncode(message.src_node_id()).c_str(),
                HexEncode(message.des_node_id()).c_str());
        return;
    }
    routing_table->HandleHandshake(message, packet);
}

void WrouterMessageHandler::HandleConnectRequest(
        transport::protobuf::RoutingMessage& message,
        base::xpacket_t& packet) {
    std::string relay_routing_id;
    // from relay_routing
    RoutingTablePtr routing_table = GetRoutingTable(
            message.des_node_id(),
            message.has_is_root() && message.is_root());
    if (!routing_table) {
        // mostly this is relay routing
        protobuf::ConnectReq conn_req;
        if (!conn_req.ParseFromString(message.data())) {
            TOP_WARN2("ConnectRequest ParseFromString from string failed!");
            return;
        }
        relay_routing_id = conn_req.relay_routing_id();
        routing_table = GetRoutingTable(
                relay_routing_id,
                message.has_is_root() && message.is_root());
    }

    if (!routing_table) {
        TOP_WARN2("HandleConnectRequest msg.is_root(%d) msg.src_node_id(%s) msg.des_node_id(%s) msg.relay_routing_id(%s)",
                message.is_root(),
                HexEncode(message.src_node_id()).c_str(),
                HexEncode(message.des_node_id()).c_str(),
                HexEncode(relay_routing_id).c_str());
        return;
    }
    routing_table->HandleConnectRequest(message, packet);
}

}  // namespace wrouter

}  // namespace top
