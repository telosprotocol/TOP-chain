// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/register_message_handler.h"

#include "xwrouter/message_handler/wrouter_message_handler.h"

namespace top {

namespace wrouter {

void WrouterRegisterMessageHandler(int msg_type, transport::HandlerProc handler_proc) {
    WrouterMessageHandler::Instance()->AddHandler(msg_type, handler_proc);
}

void WrouterUnregisterMessageHandler(int msg_type) {
    WrouterMessageHandler::Instance()->RemoveHandler(msg_type);
}

}  // namespace wrouter

}  // namespace top
