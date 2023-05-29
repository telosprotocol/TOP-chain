// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xstring_view.h"

#include <array>

NS_BEG1(top)

XINLINE_CONSTEXPR std::array<char const *, 16> nibble_binary_cstr = {
    "0000", "0001", "0010", "0011",
    "0100", "0101", "0110", "0111",
    "1000", "1001", "1010", "1011",
    "1100", "1101", "1110", "1111",
};

XINLINE_CONSTEXPR char const * binary_prefix_cstr = "0b";
XINLINE_CONSTEXPR char const * binary_prefix_uppercase_cstr = "0B";

bool has_binary_prefix(xstring_view_t input) noexcept;

bool is_binary_string(xstring_view_t str) noexcept;
bool is_binary_string_with_prefix(xstring_view_t str) noexcept;
bool is_binary_string_without_prefix(xstring_view_t str) noexcept;

NS_END1

NS_BEG2(top, details)

enum class xtop_enum_significant_bit : uint8_t {
    lsb0 = 0x00,
    msb0 = 0x01,
};

NS_END2

NS_BEG1(top)

using xsignificant_bit_t = details::xtop_enum_significant_bit;

XINLINE_CONSTEXPR xsignificant_bit_t LSB0 = xsignificant_bit_t::lsb0;
XINLINE_CONSTEXPR xsignificant_bit_t MSB0 = xsignificant_bit_t::msb0;

NS_END1
