// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>

namespace top {

    namespace base {
        class xpacket_t;
    }

    namespace transport {
        class MultiThreadHandler;
        class Transport;

        namespace protobuf {
            class RoutingMessage;
        }

        using on_receive_callback_t = std::function<void(transport::protobuf::RoutingMessage & message, base::xpacket_t &)>;

        class UdpProperty;
        using UdpPropertyPtr = std::shared_ptr<UdpProperty>;
    }

}
