// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xvledger/xvblock.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbase/xobject_ptr.h"
#include "xcommon/xaddress.h"

#include <functional>
#include <initializer_list>
#include <unordered_map>

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
    xtop_sniff_broadcast_config(xsniff_broadcast_type_t type_, xsniff_broadcast_policy_t policy_) : type(type_), policy(policy_) {
    }
};
using xsniff_broadcast_config_t = xtop_sniff_broadcast_config;

struct xtop_sniff_timer_config {
    uint32_t interval{0};
    std::string action{};

    xtop_sniff_timer_config() = default;
    xtop_sniff_timer_config(uint32_t interval_, std::string action_) : interval(interval_), action(std::move(action_)) {
    }
};
using xsniff_timer_config_t = xtop_sniff_timer_config;

struct xtop_sniff_block_config {
    common::xaccount_address_t sniff_address;
    common::xaccount_address_t action_address;
    std::string action{};

    xtop_sniff_block_config() = default;
    xtop_sniff_block_config(common::xaccount_address_t const & sniff_address_, common::xaccount_address_t const & action_address_, std::string action_)
      : sniff_address(sniff_address_), action_address(action_address_), action(std::move(action_)) {
    }
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
