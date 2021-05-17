// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xnetwork/xendpoint.h"
#include "xbase/xns_macro.h"

NS_BEG2(top, network)

class xtop_network_bytes_message final
{
private:
    xbyte_buffer_t m_payload{};
    xendpoint_t m_endpoint{};

public:
    xtop_network_bytes_message()                                               = default;
    xtop_network_bytes_message(xtop_network_bytes_message const &)             = default;
    xtop_network_bytes_message & operator=(xtop_network_bytes_message const &) = default;
    xtop_network_bytes_message(xtop_network_bytes_message &&)                  = default;
    xtop_network_bytes_message & operator=(xtop_network_bytes_message &&)      = default;
    ~xtop_network_bytes_message()                                              = default;

    xtop_network_bytes_message(xbyte_buffer_t pd, xendpoint_t ep) noexcept;

    xbyte_buffer_t const &
    payload() const noexcept;

    xendpoint_t const &
    endpoint() const noexcept;
};

using xnetwork_bytes_message_t = xtop_network_bytes_message;

NS_END2
