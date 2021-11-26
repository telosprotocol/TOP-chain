// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xcommon/xnode_type.h"

#include <string>
#include <vector>
#include <type_traits>

NS_BEG2(top, common)

enum class xenum_lagacy_miner_type : uint32_t {
    invalid = static_cast<std::underlying_type<xlagacy_node_type_t>::type>(xlagacy_node_type_t::invalid),
    edge = static_cast<std::underlying_type<xlagacy_node_type_t>::type>(xlagacy_node_type_t::edge),
    advance = static_cast<std::underlying_type<xlagacy_node_type_t>::type>(common::xlagacy_node_type_t::consensus_auditor),
    consensus = static_cast<std::underlying_type<xlagacy_node_type_t>::type>(common::xlagacy_node_type_t::consensus_validator),
    archive = static_cast<std::underlying_type<xlagacy_node_type_t>::type>(xlagacy_node_type_t::archive)
};
using xlagacy_miner_type_t = xenum_lagacy_miner_type;

enum class xenum_miner_type : uint32_t {
    invalid      = 0x00000000,
    /// @brief edge miner which will be elected as 'edge'.
    edge         = 0x00000001,
    /// @brief advance miner which will be elected as 'auditor', 'validator' or 'archive'.
    advance      = 0x00000002,
    /// @brief validator miner which will be elected as 'validator' only.
    validator    = 0x00000004,
    /// @brief archive miner which will be elected as 'archive' only.
    archive      = 0x00000008,
    /// @brief special case for full-node. such account will be elected as a special'archive'
    ///        but its business behavour is untrustable (since no deposit is required)
    ///        which means sync module won't pull data from such 'archive'
    full_node = 0x00000010,
};
using xminer_type_t = xenum_miner_type;

XINLINE_CONSTEXPR char const * XMINER_TYPE_EDGE      = "edge";
XINLINE_CONSTEXPR char const * XMINER_TYPE_ADVANCE   = "advance";
XINLINE_CONSTEXPR char const * XMINER_TYPE_VALIDATOR = "validator";
XINLINE_CONSTEXPR char const * XMINER_TYPE_ARCHIVE   = "archive";
XINLINE_CONSTEXPR char const * XMINER_TYPE_FULL_NODE = "full_node";

std::int32_t
operator <<(top::base::xstream_t & stream, xminer_type_t const & role_type);

std::int32_t
operator >>(top::base::xstream_t & stream, xminer_type_t & role_type);

constexpr
xminer_type_t
operator &(xminer_type_t const lhs, xminer_type_t const rhs) noexcept {
#if defined XCXX14_OR_ABOVE
    auto const lhs_value = static_cast<std::underlying_type<xminer_type_t>::type>(lhs);
    auto const rhs_value = static_cast<std::underlying_type<xminer_type_t>::type>(rhs);
    return static_cast<xminer_type_t>(lhs_value & rhs_value);
#else
    return static_cast<xminer_type_t>(static_cast<std::underlying_type<xminer_type_t>::type>(lhs) & static_cast<std::underlying_type<xminer_type_t>::type>(rhs));
#endif
}

constexpr
xminer_type_t
operator |(xminer_type_t const lhs, xminer_type_t const rhs) noexcept {
#if defined XCXX14_OR_ABOVE
    auto const lhs_value = static_cast<std::underlying_type<xminer_type_t>::type>(lhs);
    auto const rhs_value = static_cast<std::underlying_type<xminer_type_t>::type>(rhs);
    return static_cast<xminer_type_t>(lhs_value | rhs_value);
#else
    return static_cast<xminer_type_t>(static_cast<std::underlying_type<xminer_type_t>::type>(lhs) | static_cast<std::underlying_type<xminer_type_t>::type>(rhs));
#endif
}

xminer_type_t &
operator &=(xminer_type_t & lhs, xminer_type_t const rhs) noexcept;

xminer_type_t &
operator |=(xminer_type_t & lhs, xminer_type_t const rhs) noexcept;

template <xminer_type_t VRoleType>
bool
has(xminer_type_t const type) noexcept {
    return VRoleType == (VRoleType & type);
}

bool
has(xminer_type_t const target, xminer_type_t const input) noexcept;

std::string
to_string(xminer_type_t const role);

common::xminer_type_t
to_miner_type(std::string const & node_types);

NS_END2
