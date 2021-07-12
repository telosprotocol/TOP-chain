// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xbase/xrouter.h"
#include "xgossip/gossip_interface.h"
#include "xwrouter/message_handler/xwrouter_xid_handler.h"
#include "xwrouter/wrouter_utils/wrouter_utils.h"

#include <memory>

namespace top {

namespace transport {
class MultiThreadHandler;
class Transport;
typedef std::shared_ptr<Transport> TransportPtr;

namespace protobuf {
class RoutingMessage;
};
};  // namespace transport

namespace wrouter {

class Wrouter {
public:
    static Wrouter * Instance();
    void Init(base::xcontext_t & context, const uint32_t thread_id, transport::TransportPtr transport_ptr);
    int32_t send(transport::protobuf::RoutingMessage & message);
    virtual int32_t recv(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);

    int32_t HandleOwnSyncPacket(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);

private:
    Wrouter();
    ~Wrouter();
    int32_t HandleOwnPacket(transport::protobuf::RoutingMessage & message, base::xpacket_t & packet);

private:
    std::shared_ptr<WrouterXidHandler> wxid_handler_;
    DISALLOW_COPY_AND_ASSIGN(Wrouter);
};

}  // namespace wrouter

}  // namespace top
