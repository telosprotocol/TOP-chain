// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xpacket.h"
#include "xkad/proto/kadmlia.pb.h"
#include "xtransport/transport_fwd.h"

#include <functional>

namespace top {
namespace transport {
typedef std::function<void(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet)> HandlerProc;
}
namespace wrouter {

void WrouterRegisterMessageHandler(int msg_type, transport::HandlerProc handler_proc);
void WrouterUnregisterMessageHandler(int msg_type);

}  // namespace wrouter

}  // namespace top
