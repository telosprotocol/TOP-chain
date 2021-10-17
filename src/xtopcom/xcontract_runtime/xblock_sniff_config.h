// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"

#include <cstdint>
#include <string>

NS_BEG2(top, contract_runtime)

enum class enum_sniff_broadcast_type : std::uint32_t {
    invalid = static_cast<uint32_t>(common::xnode_type_t::invalid),
    rec = static_cast<uint32_t>(common::xnode_type_t::rec),
    zec = static_cast<uint32_t>(common::xnode_type_t::zec),
    storage = static_cast<uint32_t>(common::xnode_type_t::storage),
    all = static_cast<uint32_t>(common::xnode_type_t::all),
};
using xsniff_broadcast_type_t = enum_sniff_broadcast_type;

enum class enum_sniff_broadcast_policy : uint8_t { invalid, full_block, all_block };
using xsniff_broadcast_policy_t = enum_sniff_broadcast_policy;

struct xtop_sniff_broadcast_config {
    xsniff_broadcast_type_t type{xsniff_broadcast_type_t::invalid};
    xsniff_broadcast_policy_t policy{xsniff_broadcast_policy_t::invalid};

    xtop_sniff_broadcast_config() = default;
    xtop_sniff_broadcast_config(xtop_sniff_broadcast_config const &) = default;
    xtop_sniff_broadcast_config & operator=(xtop_sniff_broadcast_config const &) = default;
    xtop_sniff_broadcast_config(xtop_sniff_broadcast_config &&) = default;
    xtop_sniff_broadcast_config & operator=(xtop_sniff_broadcast_config &&) = default;
    ~xtop_sniff_broadcast_config() = default;

    xtop_sniff_broadcast_config(xsniff_broadcast_type_t type, xsniff_broadcast_policy_t policy);
};
using xsniff_broadcast_config_t = xtop_sniff_broadcast_config;

struct xtop_sniff_timer_config {
    uint32_t interval{0};
    std::string action{};

    xtop_sniff_timer_config() = default;
    xtop_sniff_timer_config(xtop_sniff_timer_config const &) = default;
    xtop_sniff_timer_config & operator=(xtop_sniff_timer_config const &) = default;
    xtop_sniff_timer_config(xtop_sniff_timer_config &&) = default;
    xtop_sniff_timer_config & operator=(xtop_sniff_timer_config &&) = default;
    ~xtop_sniff_timer_config() = default;

    xtop_sniff_timer_config(uint32_t interval, std::string action);
};
using xsniff_timer_config_t = xtop_sniff_timer_config;

struct xtop_sniff_block_config {
    common::xaccount_address_t sniff_address;
    common::xaccount_address_t action_address;
    std::string action{};

    xtop_sniff_block_config() = default;
    xtop_sniff_block_config(xtop_sniff_block_config const &) = default;
    xtop_sniff_block_config & operator=(xtop_sniff_block_config const &) = default;
    xtop_sniff_block_config(xtop_sniff_block_config &&) = default;
    xtop_sniff_block_config & operator=(xtop_sniff_block_config &&) = default;
    ~xtop_sniff_block_config() = default;

    xtop_sniff_block_config(common::xaccount_address_t const & sniff_address, common::xaccount_address_t const & action_address, std::string action);
};
using xsniff_block_config_t = xtop_sniff_block_config;

enum class enum_sniff_type: uint32_t {
    invalid = 0x00000000,
    broadcast = 0x00000001,
    timer = 0x00000002,
    block = 0x00000004,
};
using xsniff_type_t = enum_sniff_type;

NS_END2
