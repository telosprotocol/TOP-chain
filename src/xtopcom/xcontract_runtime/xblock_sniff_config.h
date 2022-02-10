// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcommon/xlogic_time.h"

#include <cstdint>
#include <string>

NS_BEG2(top, contract_runtime)

enum class enum_sniff_broadcast_type : std::uint32_t {
    invalid = static_cast<uint32_t>(common::xnode_type_t::invalid),
    rec = static_cast<uint32_t>(common::xnode_type_t::rec),
    zec = static_cast<uint32_t>(common::xnode_type_t::zec),
    storage = static_cast<uint32_t>(common::xnode_type_t::storage),
    all = static_cast<uint32_t>(common::xnode_type_t::all_types),
};
using xsniff_broadcast_type_t = enum_sniff_broadcast_type;

enum class enum_sniff_block_type : uint8_t { invalid, full_block, all_block };
using xsniff_block_type_t = enum_sniff_block_type;

struct xtop_sniff_broadcast_config {
    xsniff_broadcast_type_t zone{xsniff_broadcast_type_t::invalid};
    xsniff_block_type_t type{xsniff_block_type_t::invalid};

    xtop_sniff_broadcast_config() = default;
    xtop_sniff_broadcast_config(xtop_sniff_broadcast_config const &) = default;
    xtop_sniff_broadcast_config & operator=(xtop_sniff_broadcast_config const &) = default;
    xtop_sniff_broadcast_config(xtop_sniff_broadcast_config &&) = default;
    xtop_sniff_broadcast_config & operator=(xtop_sniff_broadcast_config &&) = default;
    ~xtop_sniff_broadcast_config() = default;

    xtop_sniff_broadcast_config(xsniff_broadcast_type_t zone, xsniff_block_type_t type);
};
using xsniff_broadcast_config_t = xtop_sniff_broadcast_config;

class xtop_timer_config_data {
    uint32_t m_interval{0};
    std::string m_tcc_config_name{};

public:
    xtop_timer_config_data() = default;
    xtop_timer_config_data(xtop_timer_config_data const &) = default;
    xtop_timer_config_data & operator=(xtop_timer_config_data const &) = default;
    xtop_timer_config_data(xtop_timer_config_data &&) = default;
    xtop_timer_config_data & operator=(xtop_timer_config_data &&) = default;
    ~xtop_timer_config_data() = default;

    explicit xtop_timer_config_data(uint32_t const interval) noexcept;
    explicit xtop_timer_config_data(std::string tcc_config_name) noexcept;

    uint32_t get_timer_interval(std::error_code & ec) const;
    uint32_t get_timer_interval() const;
};
using xtimer_config_data_t = xtop_timer_config_data;

enum class xtop_timer_strategy_type: uint8_t {
    invalid,
    normal,
    table
};
using xtimer_strategy_type_t = xtop_timer_strategy_type;

struct xtop_sniff_timer_config {
    xtimer_config_data_t timer_config_data{};
    std::string action{};
    xtimer_strategy_type_t strategy{};

    xtop_sniff_timer_config() = default;
    xtop_sniff_timer_config(xtop_sniff_timer_config const &) = default;
    xtop_sniff_timer_config & operator=(xtop_sniff_timer_config const &) = default;
    xtop_sniff_timer_config(xtop_sniff_timer_config &&) = default;
    xtop_sniff_timer_config & operator=(xtop_sniff_timer_config &&) = default;
    ~xtop_sniff_timer_config() = default;

    explicit xtop_sniff_timer_config(uint32_t interval, std::string action, xtimer_strategy_type_t strategy);
    explicit xtop_sniff_timer_config(std::string tcc_config_name, std::string action, xtimer_strategy_type_t strategy);
};
using xsniff_timer_config_t = xtop_sniff_timer_config;

struct xtop_sniff_block_config {
    common::xaccount_address_t sniff_address;
    common::xaccount_address_t action_address;
    std::string action{};
    xsniff_block_type_t type{xsniff_block_type_t::invalid};

    xtop_sniff_block_config() = default;
    xtop_sniff_block_config(xtop_sniff_block_config const &) = default;
    xtop_sniff_block_config & operator=(xtop_sniff_block_config const &) = default;
    xtop_sniff_block_config(xtop_sniff_block_config &&) = default;
    xtop_sniff_block_config & operator=(xtop_sniff_block_config &&) = default;
    ~xtop_sniff_block_config() = default;

    xtop_sniff_block_config(common::xaccount_address_t const & sniff_address, common::xaccount_address_t const & action_address, std::string action, xsniff_block_type_t type);
};
using xsniff_block_config_t = xtop_sniff_block_config;

enum class enum_sniff_type: uint32_t {
    none = 0x00000000,
    broadcast = 0x00000001,
    timer = 0x00000002,
    block = 0x00000004,
};
using xsniff_type_t = enum_sniff_type;

constexpr xsniff_type_t operator|(xsniff_type_t const lhs, xsniff_type_t const rhs) noexcept {
    using type = std::underlying_type<xsniff_type_t>::type;
    return static_cast<xsniff_type_t>(static_cast<type>(lhs) | static_cast<type>(rhs));
}

constexpr xsniff_type_t operator&(xsniff_type_t const lhs, xsniff_type_t const rhs) noexcept {
    using type = std::underlying_type<xsniff_type_t>::type;
    return static_cast<xsniff_type_t>(static_cast<type>(lhs) & static_cast<type>(rhs));
}

xsniff_type_t & operator&=(xsniff_type_t & lhs, xsniff_type_t const rhs) noexcept;
xsniff_type_t & operator|=(xsniff_type_t & lhs, xsniff_type_t const rhs) noexcept;

template <xsniff_type_t SniffTypeV>
constexpr bool has(xsniff_type_t const input) noexcept {
    return SniffTypeV == (SniffTypeV & input);
}

NS_END2
