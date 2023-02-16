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

enum class xenum_legacy_miner_type : uint32_t {
    invalid = static_cast<std::underlying_type<xlegacy_node_type_t>::type>(xlegacy_node_type_t::invalid),
    edge = static_cast<std::underlying_type<xlegacy_node_type_t>::type>(xlegacy_node_type_t::edge),
    advance = static_cast<std::underlying_type<xlegacy_node_type_t>::type>(common::xlegacy_node_type_t::consensus_auditor),
    consensus = static_cast<std::underlying_type<xlegacy_node_type_t>::type>(common::xlegacy_node_type_t::consensus_validator),
    archive = static_cast<std::underlying_type<xlegacy_node_type_t>::type>(xlegacy_node_type_t::archive)
};
using xlegacy_miner_type_t = xenum_legacy_miner_type;

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
    /// @brief special case for exchange. such account will be elected as a special'archive'
    ///        but its business behavour is untrustable (since no deposit is required)
    ///        which means sync module won't pull data from such 'archive'
    exchange = 0x00000010,
};
using xminer_type_t = xenum_miner_type;

XINLINE_CONSTEXPR char const * XMINER_TYPE_INVALID   = "invalid";
XINLINE_CONSTEXPR char const * XMINER_TYPE_EDGE      = "edge";
XINLINE_CONSTEXPR char const * XMINER_TYPE_ADVANCE   = "advance";
XINLINE_CONSTEXPR char const * XMINER_TYPE_VALIDATOR = "validator";
// #if defined(XENABLE_MOCK_ZEC_STAKE)
XINLINE_CONSTEXPR char const * XMINER_TYPE_ARCHIVE   = "archive";
// #endif
XINLINE_CONSTEXPR char const * XMINER_TYPE_EXCHANGE  = "exchange";

std::int32_t
operator <<(top::base::xstream_t & stream, xminer_type_t const & role_type);

std::int32_t
operator >>(top::base::xstream_t & stream, xminer_type_t & role_type);

constexpr
xminer_type_t
operator &(xminer_type_t const lhs, xminer_type_t const rhs) noexcept {
#if defined(XCXX14)
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
#if defined(XCXX14)
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
