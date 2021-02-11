// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xp2p/xdistance.hpp"

#include <cmath>
#include <cstdint>
#include <memory>

NS_BEG2(top, network)


/**
 * \brief Message priority
 */
    enum class xenum_deliver_priority : std::uint8_t
{
    normal,
    highest = 0xFF
};
using xdeliver_priority_t = xenum_deliver_priority;


/**
 * \brief Message deliver protocol.
 */
enum class xenum_deliver_protocol : std::uint8_t
{
    udp,
    rudp,
    tcp
};
using xdeliver_protocol_t = xenum_deliver_protocol;


/**
 * \brief Message reliability property
 */
struct xtop_deliver_reliability final
{
    xdeliver_protocol_t protocol{ xdeliver_protocol_t::udp };
    std::uint8_t retry_count{ 0 };

    xtop_deliver_reliability()                                             = default;
    xtop_deliver_reliability(xtop_deliver_reliability const &)             = default;
    xtop_deliver_reliability & operator=(xtop_deliver_reliability const &) = default;
    xtop_deliver_reliability(xtop_deliver_reliability &&)                  = default;
    xtop_deliver_reliability & operator=(xtop_deliver_reliability &&)      = default;
    ~xtop_deliver_reliability()                                            = default;

    xtop_deliver_reliability(xdeliver_protocol_t const p, std::uint8_t const c)
        : protocol{ p }, retry_count{ c }
    {}
};
using xdeliver_reliability_t = xtop_deliver_reliability;


/**
 * \brief Message deliver property
 */
struct xtop_deliver_property final
{
    xdeliver_priority_t priority{ xdeliver_priority_t::normal };
    std::shared_ptr<xdeliver_reliability_t> reliability{ nullptr };

    xtop_deliver_property()                                          = default;
    xtop_deliver_property(xtop_deliver_property const &)             = default;
    xtop_deliver_property & operator=(xtop_deliver_property const &) = default;
    xtop_deliver_property(xtop_deliver_property &&)                  = default;
    xtop_deliver_property & operator=(xtop_deliver_property &&)      = default;
    ~xtop_deliver_property()                                         = default;

    xtop_deliver_property(xdeliver_priority_t const dp, std::shared_ptr<xdeliver_reliability_t> r)
        : priority{ dp }, reliability{ r }
    {}
};
using xdeliver_property_t = xtop_deliver_property;


enum class xenum_spread_mode : std::uint8_t
{
    invalid,
    pt2pt,
    broadcast
};
using xspread_mode_t = xenum_spread_mode;

const std::uint8_t ttl_factor_upper_bound{ static_cast<std::uint8_t>(std::log2(p2p::xdistance_t::upper_bound())) };

struct xtop_spread_property final
{
    xspread_mode_t spread_mode{ xspread_mode_t::pt2pt };
    std::uint8_t ttl_factor{ ttl_factor_upper_bound };

    xtop_spread_property()                                         = default;
    xtop_spread_property(xtop_spread_property const &)             = default;
    xtop_spread_property & operator=(xtop_spread_property const &) = default;
    xtop_spread_property(xtop_spread_property &&)                  = default;
    xtop_spread_property & operator=(xtop_spread_property &&)      = default;
    ~xtop_spread_property()                                        = default;

    xtop_spread_property(xspread_mode_t const m, std::uint8_t const ttl)
        : spread_mode{ m }, ttl_factor{ ttl }
    {}
};
using xspread_property_t = xtop_spread_property;

struct xtop_transmission_property final
{
    xdeliver_property_t deliver_property{};
    xspread_property_t spread_property{};

    xtop_transmission_property()                                               = default;
    xtop_transmission_property(xtop_transmission_property const &)             = default;
    xtop_transmission_property & operator=(xtop_transmission_property const &) = default;
    xtop_transmission_property(xtop_transmission_property &&)                  = default;
    xtop_transmission_property & operator=(xtop_transmission_property &&)      = default;
    ~xtop_transmission_property()                                              = default;

    xtop_transmission_property(xdeliver_property_t dp, xspread_property_t sp)
        : deliver_property{ std::move(dp) }, spread_property{ std::move(sp) }
    {}
};
using xtransmission_property_t = xtop_transmission_property;

NS_END2
