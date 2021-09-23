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

enum class xenum_old_role_type : uint32_t {
    invalid = static_cast<std::underlying_type<xold_node_type_t>::type>(xold_node_type_t::invalid),
    edge = static_cast<std::underlying_type<xold_node_type_t>::type>(xold_node_type_t::edge),
    advance = static_cast<std::underlying_type<xold_node_type_t>::type>(common::xold_node_type_t::consensus_auditor),
    consensus = static_cast<std::underlying_type<xold_node_type_t>::type>(common::xold_node_type_t::consensus_validator),
    archive = static_cast<std::underlying_type<xold_node_type_t>::type>(xold_node_type_t::archive)
};
using xold_role_type_t = xenum_old_role_type;

enum class xenum_role_type : uint32_t {
    invalid      = 0x00000000,
    /// @brief edge role which will be elected as 'edge'.
    edge         = 0x00000001,
    /// @brief advance role which will be elected as 'auditor', 'validator' or 'archive'.
    advance      = 0x00000002,
    /// @brief validator role which will be elected as 'validator' only.
    validator    = 0x00000004,
    /// @brief archive role which will be elected as 'archive' only.
    archive      = 0x00000008,
    /// @brief special case for full-edge. such account will be elected as a special'archive'
    ///        but its business behavour is untrustable (since no deposit is required)
    ///        which means sync module won't pull data from such 'archive'
    full_node = 0x00000010,
};
using xrole_type_t = xenum_role_type;

XINLINE_CONSTEXPR char const * XMINER_TYPE_EDGE      = "edge";
XINLINE_CONSTEXPR char const * XMINER_TYPE_ADVANCE   = "advance";
XINLINE_CONSTEXPR char const * XMINER_TYPE_VALIDATOR = "validator";
XINLINE_CONSTEXPR char const * XMINER_TYPE_ARCHIVE   = "archive";
XINLINE_CONSTEXPR char const * XMINER_TYPE_FULL_NODE = "full_node";

std::int32_t
operator <<(top::base::xstream_t & stream, xrole_type_t const & role_type);

std::int32_t
operator >>(top::base::xstream_t & stream, xrole_type_t & role_type);

constexpr
xrole_type_t
operator &(xrole_type_t const lhs, xrole_type_t const rhs) noexcept {
#if defined XCXX14_OR_ABOVE
    auto const lhs_value = static_cast<std::underlying_type<xrole_type_t>::type>(lhs);
    auto const rhs_value = static_cast<std::underlying_type<xrole_type_t>::type>(rhs);
    return static_cast<xrole_type_t>(lhs_value & rhs_value);
#else
    return static_cast<xrole_type_t>(static_cast<std::underlying_type<xrole_type_t>::type>(lhs) & static_cast<std::underlying_type<xrole_type_t>::type>(rhs));
#endif
}

constexpr
xrole_type_t
operator |(xrole_type_t const lhs, xrole_type_t const rhs) noexcept {
#if defined XCXX14_OR_ABOVE
    auto const lhs_value = static_cast<std::underlying_type<xrole_type_t>::type>(lhs);
    auto const rhs_value = static_cast<std::underlying_type<xrole_type_t>::type>(rhs);
    return static_cast<xrole_type_t>(lhs_value | rhs_value);
#else
    return static_cast<xrole_type_t>(static_cast<std::underlying_type<xrole_type_t>::type>(lhs) | static_cast<std::underlying_type<xrole_type_t>::type>(rhs));
#endif
}

xrole_type_t &
operator &=(xrole_type_t & lhs, xrole_type_t const rhs) noexcept;

xrole_type_t &
operator |=(xrole_type_t & lhs, xrole_type_t const rhs) noexcept;

template <xrole_type_t VRoleType>
bool
has(xrole_type_t const type) noexcept {
    return VRoleType == (VRoleType & type);
}

bool
has(xrole_type_t const target, xrole_type_t const input) noexcept;

std::string
to_string(xrole_type_t const role);

common::xrole_type_t
to_role_type(std::string const & node_types);

NS_END2
