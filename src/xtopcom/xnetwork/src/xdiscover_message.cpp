// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xp2p/xdiscover_message.h"

NS_BEG3(top, network, p2p)

constexpr std::chrono::seconds xtop_discover_message::ttl;

xtop_discover_message::xtop_discover_message(xbyte_buffer_t pd,
                                             common::xnode_id_t sender_id,
                                             xdiscover_message_type_t const t)
    : m_payload{ std::move(pd) }, m_sender_id{ std::move(sender_id) }, m_type{ t }
    , m_ts{ std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() + ttl).time_since_epoch()).count() }
{
}

xbyte_buffer_t const &
xtop_discover_message::payload() const noexcept {
    return m_payload;
}

bool
xtop_discover_message::empty() const noexcept {
    return m_sender_id.empty() || m_payload.empty() || m_type == xdiscover_message_type_t::invalid;
}


common::xnode_id_t const &
xtop_discover_message::sender_node_id() const noexcept {
    return m_sender_id;
}

xdiscover_message_type_t
xtop_discover_message::type() const noexcept {
    return m_type;
}

bool
xtop_discover_message::expired() const noexcept {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() > m_ts;
}

NS_END3

