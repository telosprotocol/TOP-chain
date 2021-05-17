// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xnetwork_message.h"

#include <utility>

NS_BEG2(top, network)

xtop_auxiliary_network_message::xtop_auxiliary_network_message(xmessage_t msg,
                                                               common::xnode_id_t snd_id,
                                                               common::xnode_id_t recv_id,
                                                               xtransmission_property_t trans_prop) noexcept
    : m_message{ std::move(msg) }, m_sender_id{ std::move(snd_id) }
    , m_receiver_id{ std::move(recv_id) }
    , m_transmission_property{ std::move(trans_prop) }
{
}

bool
xtop_auxiliary_network_message::operator==(xtop_auxiliary_network_message const & other) const noexcept {
    return hash() == other.hash();
}

bool
xtop_auxiliary_network_message::operator!=(xtop_auxiliary_network_message const & other) const noexcept {
    return !(*this == other);
}

xmessage_t const &
xtop_auxiliary_network_message::message() const noexcept {
    return m_message;
}

common::xnode_id_t const &
xtop_auxiliary_network_message::sender_id() const noexcept {
    return m_sender_id;
}

common::xnode_id_t const &
xtop_auxiliary_network_message::receiver_id() const noexcept {
    return m_receiver_id;
}

xtransmission_property_t &
xtop_auxiliary_network_message::transmission_property() noexcept {
    return const_cast<xtransmission_property_t &>(static_cast<xtop_auxiliary_network_message const &>(*this).transmission_property());
}

xtransmission_property_t const &
xtop_auxiliary_network_message::transmission_property() const noexcept {
    return m_transmission_property;
}

std::uint64_t
xtop_auxiliary_network_message::hash() const {
    return m_message.hash();
}

bool
xtop_auxiliary_network_message::empty() const noexcept {
    return m_message.empty();
}

NS_END2

NS_BEG1(std)

std::size_t
hash<top::network::xnetwork_message_t>::operator()(top::network::xnetwork_message_t const & network_message) const noexcept {
    return static_cast<std::size_t>(network_message.hash());
}

NS_END1

