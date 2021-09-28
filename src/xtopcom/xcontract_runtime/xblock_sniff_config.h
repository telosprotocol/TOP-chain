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

enum class enum_sniff_broadcast_policy : uint8_t { invalid, normal, fullunit };
using xsniff_broadcast_policy_t = enum_sniff_broadcast_policy;

struct xtop_sniff_broadcast_config {
    xsniff_broadcast_type_t m_type{xsniff_broadcast_type_t::invalid};
    xsniff_broadcast_policy_t m_policy{xsniff_broadcast_policy_t::invalid};

    xtop_sniff_broadcast_config() = default;
    xtop_sniff_broadcast_config(xsniff_broadcast_type_t type, xsniff_broadcast_policy_t policy) : m_type(type), m_policy(policy) {
    }
};
using xsniff_broadcast_config_t = xtop_sniff_broadcast_config;

struct xtop_sniff_timer_config {
    uint32_t m_interval{0};
    std::string m_action{""};

    xtop_sniff_timer_config() = default;
    xtop_sniff_timer_config(uint32_t interval, std::string action) : m_interval(interval), m_action(action) {
    }
};
using xsniff_timer_config_t = xtop_sniff_timer_config;

struct xtop_sniff_block_config {
    common::xaccount_address_t m_sniff_address;
    common::xaccount_address_t m_action_address;
    std::string m_action{""};

    xtop_sniff_block_config() = default;
    xtop_sniff_block_config(common::xaccount_address_t const & sniff_address, common::xaccount_address_t const & action_address, std::string action)
      : m_sniff_address(sniff_address), m_action_address(action_address), m_action(action) {
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
