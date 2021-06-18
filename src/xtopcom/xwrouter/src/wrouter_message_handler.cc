// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/message_handler/wrouter_message_handler.h"

#include "xkad/routing_table/callback_manager.h"
#include "xkad/routing_table/local_node_info.h"
#include "xkad/routing_table/node_detection_manager.h"
#include "xkad/routing_table/node_info.h"
#include "xkad/routing_table/routing_utils.h"
#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xtransport/udp_transport/transport_util.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace top {

using namespace kadmlia;  // NOLINT

namespace wrouter {

WrouterMessageHandler::WrouterMessageHandler() {
}

WrouterMessageHandler::~WrouterMessageHandler() {
    for (uint32_t index = 0; index < array_handlers_.size(); ++index) {
        array_handlers_[index] = nullptr;
    }
}

WrouterMessageHandler * WrouterMessageHandler::Instance() {
    static WrouterMessageHandler ins;
    return &ins;
}

// used by Wrouter::HandleOwnPacket
void WrouterMessageHandler::HandleMessage(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    if (message.hop_num() >= kHopToLive) {
        std::string nodes;
        for (int i = 0; i < message.hop_nodes_size(); ++i) {
            nodes += HexSubstr(message.hop_nodes(i).node_id()) + " -> ";
        }
        TOP_WARN(
            "stop send msg because hop to live is max: %d [%s] des[%s] "
            "message_type[%d] nodes[%s]",
            kHopToLive,
            HexSubstr(message.src_node_id()).c_str(),
            HexSubstr(message.des_node_id()).c_str(),
            message.type(),
            nodes.c_str());
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
    call(message, packet);
}

void WrouterMessageHandler::HandleSyncMessage(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet) {
    TOP_DEBUG("HandleSyncMessage msgtype:%d", message.type());
    transport::HandlerProc call = array_handlers_[message.type()];
    if (!call) {
        TOP_WARN("invalid message.type(%d), callback not registered", message.type());
        return;
    }
    call(message, packet);
}

void WrouterMessageHandler::AddHandler(int msg_type, transport::HandlerProc handler_proc) {
    assert(handler_proc);
    assert(msg_type < MsgHandlerMaxSize);
    assert(!array_handlers_[msg_type]);

    array_handlers_[msg_type] = handler_proc;
}

void WrouterMessageHandler::RemoveHandler(int msg_type) {
    assert(msg_type < MsgHandlerMaxSize);
    array_handlers_[msg_type] = nullptr;
}

}  // namespace wrouter

}  // namespace top
