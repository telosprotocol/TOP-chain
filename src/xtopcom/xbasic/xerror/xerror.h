// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xbase/xns_macro.h"

#include <limits>
#include <system_error>
#include <type_traits>

using xbase_errc_t = enum_xerror_code;

std::error_category const & base_category();
std::error_code make_error_code(enum_xerror_code ec) noexcept;
std::error_condition make_error_condition(enum_xerror_code ec) noexcept;


NS_BEG2(top, error)

class xtop_top_error : public std::runtime_error {
    using base_t = std::runtime_error;

    std::error_code m_ec{};

public:

    xtop_top_error(xtop_top_error const &)             = default;
    xtop_top_error & operator=(xtop_top_error const &) = default;
    xtop_top_error(xtop_top_error &&)                  = default;
    xtop_top_error & operator=(xtop_top_error &&)      = default;
    ~xtop_top_error() override                         = default;

    explicit xtop_top_error(std::error_code ec);
    xtop_top_error(std::error_code ec, std::string const & extra_what);
    xtop_top_error(std::error_code ec, char const * extra_what);
    xtop_top_error(int const ev, std::error_category const & ecat);
    xtop_top_error(int const ev, std::error_category const & ecat, std::string const & extra_what);
    xtop_top_error(int const ev, std::error_category const & ecat, char const * extra_what);

    std::error_code const &
    code() const noexcept;

    std::error_category const & category() const noexcept;
};
using xtop_error_t = xtop_top_error;

void throw_error(std::error_code const & ec);
void throw_error(std::error_code const ec, char const * extra_what);
void throw_error(std::error_code const ec, std::string const & extra_what);

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
