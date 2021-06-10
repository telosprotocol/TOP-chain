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
#include "xkad/routing_table/node_detection_manager.h"
#include "xkad/routing_table/local_node_info.h"
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

}  // namespace wrouter

}  // namespace top
