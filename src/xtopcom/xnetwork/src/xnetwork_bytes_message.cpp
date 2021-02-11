// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xnetwork_bytes_message.h"

NS_BEG2(top, network)

xtop_network_bytes_message::xtop_network_bytes_message(xbyte_buffer_t pd,
                                                       xendpoint_t ep) noexcept
    : m_payload{ std::move(pd) }, m_endpoint{ std::move(ep) }
{
}

xbyte_buffer_t const &
xtop_network_bytes_message::payload() const noexcept {
    return m_payload;
}

xendpoint_t const &
xtop_network_bytes_message::endpoint() const noexcept {
    return m_endpoint;
}

NS_END2
