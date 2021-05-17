// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cstdint>
#include <string>

#include "xbasic/xsimple_message.hpp"
#include "xbase/xns_macro.h"


NS_BEG2(top, network)

enum class xenum_socket_message_type : std::uint8_t
{
    invalid,

    /**
     * @brief Kademlia message, like PING, PONG, etc.
     *
     */
    kademlia,

    /**
     * @brief Gossip message.
     *
     */
    gossip,

    /**
     * @brief socket message type count.  Internal use only.
     */
    count
};

using xsocket_message_type_t = xenum_socket_message_type;

NS_END2

NS_BEG1(top)

extern
template
class xtop_simple_message<network::xsocket_message_type_t>;

NS_END1

NS_BEG2(top, network)

using xsocket_message_t = xsimple_message_t<xsocket_message_type_t>;

NS_END2

