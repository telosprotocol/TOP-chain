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

enum class xenum_role_type : std::underlying_type<xnode_type_t>::type {
    invalid   = static_cast<std::underlying_type<xnode_type_t>::type>(xnode_type_t::invalid),
    edge      = static_cast<std::underlying_type<xnode_type_t>::type>(xnode_type_t::edge),
    advance   = static_cast<std::underlying_type<xnode_type_t>::type>(common::xnode_type_t::consensus_auditor),
    consensus = static_cast<std::underlying_type<xnode_type_t>::type>(common::xnode_type_t::consensus_validator),
    archive   = static_cast<std::underlying_type<xnode_type_t>::type>(xnode_type_t::archive)
};
using xrole_type_t = xenum_role_type;

XINLINE_CONSTEXPR char const * XNODE_TYPE_EDGE      = "edge";
XINLINE_CONSTEXPR char const * XNODE_TYPE_ADVANCE   = "advance";
XINLINE_CONSTEXPR char const * XNODE_TYPE_VALIDATOR = "validator";
XINLINE_CONSTEXPR char const * XNODE_TYPE_ARCHIVE   = "archive";

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
