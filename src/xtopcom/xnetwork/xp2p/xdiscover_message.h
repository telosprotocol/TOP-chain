// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xsimple_message.hpp"
#include "xcommon/xnode_id.h"

#include <chrono>
#include <cstdint>

NS_BEG3(top, network, p2p)

enum class xenum_discover_message_type : std::uint8_t
{
    invalid,
    ping_node,
    pong,
    find_node,
    neighbors
};

using xdiscover_message_type_t = xenum_discover_message_type;

class xtop_discover_message final
{
private:
    static constexpr std::chrono::seconds ttl{ 30 };

    xbyte_buffer_t m_payload{};
    common::xnode_id_t m_sender_id{};
    xdiscover_message_type_t m_type{ xenum_discover_message_type::invalid };
    std::chrono::seconds::rep m_ts{ 0 };

public:
    xtop_discover_message()                                          = default;
    xtop_discover_message(xtop_discover_message const &)             = default;
    xtop_discover_message & operator=(xtop_discover_message const &) = default;
    xtop_discover_message(xtop_discover_message &&)                  = default;
    xtop_discover_message & operator=(xtop_discover_message &&)      = default;
    ~xtop_discover_message()                                         = default;

    xtop_discover_message(xbyte_buffer_t payload,
                          common::xnode_id_t sender_id,
                          xdiscover_message_type_t const type);

    xbyte_buffer_t const &
    payload() const noexcept;

    bool
    empty() const noexcept;

    common::xnode_id_t const &
    sender_node_id() const noexcept;

    xdiscover_message_type_t
    type() const noexcept;

    bool
    expired() const noexcept;
};

using xdiscover_message_t = xtop_discover_message;

NS_END3
