// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xnode_id.h"
#include "xnetwork/xmessage.h"
#include "xnetwork/xmessage_transmission_property.h"

#include <cstdint>
#include <functional>

NS_BEG2(top, network)

/**
 * @brief Message in network module.  It's not thread-safe.
 */
class xtop_auxiliary_network_message final : public xhashable_t<xtop_auxiliary_network_message>
{
private:
    xmessage_t m_message{};
    common::xnode_id_t m_sender_id{};
    common::xnode_id_t m_receiver_id{};
    xtransmission_property_t m_transmission_property{};

public:
    xtop_auxiliary_network_message()                                                   = default;
    xtop_auxiliary_network_message(xtop_auxiliary_network_message const &)             = default;
    xtop_auxiliary_network_message & operator=(xtop_auxiliary_network_message const &) = default;
    xtop_auxiliary_network_message(xtop_auxiliary_network_message &&)                  = default;
    xtop_auxiliary_network_message & operator=(xtop_auxiliary_network_message &&)      = default;
    ~xtop_auxiliary_network_message() override                                         = default;

    xtop_auxiliary_network_message(xmessage_t msg,
                                   common::xnode_id_t snd_id,
                                   common::xnode_id_t recv_id,
                                   xtransmission_property_t trans_prop) noexcept;

    bool
    operator==(xtop_auxiliary_network_message const & other) const noexcept;

    bool
    operator!=(xtop_auxiliary_network_message const & other) const noexcept;

    xmessage_t const &
    message() const noexcept;

    common::xnode_id_t const &
    sender_id() const noexcept;

    common::xnode_id_t const &
    receiver_id() const noexcept;

    xtransmission_property_t const &
    transmission_property() const noexcept;

    xtransmission_property_t &
    transmission_property() noexcept;

    std::uint64_t
    hash() const override;

    bool
    empty() const noexcept;
};

using xnetwork_message_t = xtop_auxiliary_network_message;


NS_END2


NS_BEG1(std)

template <>
struct hash<top::network::xnetwork_message_t> final
{
    std::size_t
    operator()(top::network::xnetwork_message_t const & network_message) const noexcept;
};

NS_END1
