// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xvnetwork_message.h"

#include <type_traits>

NS_BEG2(top, vnetwork)

xtop_vnetwork_message::xtop_vnetwork_message(common::xnode_address_t const & snd,
                                             common::xnode_address_t const & recv,
                                             xmessage_t msg,
                                             common::xlogic_time_t const logic_time)
    : m_sender{ snd }, m_receiver{ recv }
    , m_message{ std::move(msg) }
    , m_logic_time{ logic_time }
{
}

common::xnode_address_t const &
xtop_vnetwork_message::sender() const noexcept {
    return m_sender;
}

common::xnode_address_t const &
xtop_vnetwork_message::receiver() const noexcept {
    return m_receiver;
}

void xtop_vnetwork_message::receiver(common::xnode_address_t new_recv) noexcept {
    m_receiver = std::move(new_recv);
}

xmessage_t const &
xtop_vnetwork_message::message() const noexcept {
    return m_message;
}

xbyte_buffer_t const &
xtop_vnetwork_message::message_payload() const noexcept {
    return m_message.payload();
}

common::xmessage_id_t
xtop_vnetwork_message::message_id() const noexcept {
    return m_message.id();
}

common::xlogic_time_t
xtop_vnetwork_message::logic_time() const noexcept {
    return m_logic_time;
}

bool
xtop_vnetwork_message::empty() const noexcept {
    return m_sender.empty() || m_message.empty();
}

xtop_vnetwork_message::hash_result_type
xtop_vnetwork_message::hash() const {
    return m_message.hash();
}

NS_END2
