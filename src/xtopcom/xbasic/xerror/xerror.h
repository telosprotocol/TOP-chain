// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xbasic/xerror/xchain_error.h"

#include <limits>
#include <system_error>
#include <type_traits>

std::error_category const & base_category();
std::error_code make_error_code(enum_xerror_code ec) noexcept;
std::error_condition make_error_condition(enum_xerror_code ec) noexcept;


NS_BEG2(top, error)

enum class xenum_basic_errc {
    successful,
    serialization_error,
    deserialization_error,

    unknown_error = std::numeric_limits<std::underlying_type<xenum_basic_errc>::type>::max()
};
using xbasic_errc_t = xenum_basic_errc;

std::error_code make_error_code(xbasic_errc_t errc) noexcept;
std::error_condition make_error_condition(xbasic_errc_t errc) noexcept;

std::error_category const & basic_category();

NS_END2

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

template <>
struct hash<top::error::xbasic_errc_t> final {
    size_t operator()(top::error::xbasic_errc_t errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<enum_xerror_code> : std::true_type {
};

template <>
struct is_error_condition_enum<enum_xerror_code> : std::true_type {
};

template <>
struct is_error_code_enum<top::error::xbasic_errc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::error::xbasic_errc_t> : std::true_type {};


NS_END1
