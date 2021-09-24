// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbase/xns_macro.h"
// #include "xcommon/xnode_type.h"

#include <string>
#include <type_traits>
#include <vector>

NS_BEG2(top, common)

enum class xenum_registration_type : uint32_t {
    invalid = 0x00000000,
    primary = 0x00000001,
    intermediate = 0x0000002,
    senior = 0x00000004,
    hardcode = senior | intermediate | primary,
};
using xregistration_type_t = xenum_registration_type;

XINLINE_CONSTEXPR char const * XREG_TYPE_PRIMARY = "primary";
XINLINE_CONSTEXPR char const * XREG_TYPE_INTERMEDIATE = "intermediate";
XINLINE_CONSTEXPR char const * XREG_TYPE_SENIOR = "senior";
XINLINE_CONSTEXPR char const * XREG_TYPE_HARDCODE = "hardcode";

common::xregistration_type_t to_registration_type(std::string const & reg_type);

std::string to_string(xregistration_type_t const & registration_type);

constexpr xregistration_type_t operator&(xregistration_type_t const lhs, xregistration_type_t const rhs) noexcept {
#if defined XCXX14_OR_ABOVE
    auto const lhs_value = static_cast<std::underlying_type<xregistration_type_t>::type>(lhs);
    auto const rhs_value = static_cast<std::underlying_type<xregistration_type_t>::type>(rhs);
    return static_cast<xregistration_type_t>(lhs_value & rhs_value);
#else
    return static_cast<xregistration_type_t>(static_cast<std::underlying_type<xregistration_type_t>::type>(lhs) &
                                             static_cast<std::underlying_type<xregistration_type_t>::type>(rhs));
#endif
}
constexpr xregistration_type_t operator|(xregistration_type_t const lhs, xregistration_type_t const rhs) noexcept {
#if defined XCXX14_OR_ABOVE
    auto const lhs_value = static_cast<std::underlying_type<xregistration_type_t>::type>(lhs);
    auto const rhs_value = static_cast<std::underlying_type<xregistration_type_t>::type>(rhs);
    return static_cast<xregistration_type_t>(lhs_value | rhs_value);
#else
    return static_cast<xregistration_type_t>(static_cast<std::underlying_type<xregistration_type_t>::type>(lhs) |
                                             static_cast<std::underlying_type<xregistration_type_t>::type>(rhs));
#endif
}
xregistration_type_t & operator&=(xregistration_type_t & lhs, xregistration_type_t const rhs) noexcept;
xregistration_type_t & operator|=(xregistration_type_t & lhs, xregistration_type_t const rhs) noexcept;

template <xregistration_type_t RegistrationType>
bool has(xregistration_type_t const & reg_type) noexcept {
    return RegistrationType == (RegistrationType & reg_type);
}

NS_END2
