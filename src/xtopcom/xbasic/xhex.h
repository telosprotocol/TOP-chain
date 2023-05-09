// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xstring_view.h"
#include "xbasic/xutility.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <system_error>

NS_BEG1(top)

inline char to_hex_char(uint8_t const value) noexcept {
    assert(value < 16);
    constexpr char const * hexdigits = "0123456789abcdef";
    return hexdigits[value];
}

template <class Iterator>
std::string to_hex(Iterator begin, Iterator end, std::string const & prefix) {
    typedef std::iterator_traits<Iterator> traits;
    static_assert(sizeof(typename traits::value_type) == 1, "to_hex needs byte-sized element type");

    static char const * hexdigits = "0123456789abcdef";
    std::size_t off = prefix.size();
    std::string hex(std::distance(begin, end) * 2 + off, '0');
    hex.replace(0, off, prefix);
    for (; begin != end; ++begin) {
        hex[off++] = hexdigits[(*begin >> 4) & 0x0f];
        hex[off++] = hexdigits[*begin & 0x0f];
    }
    return hex;
}

template <typename T, typename std::enable_if<std::is_fundamental<T>::value && std::is_integral<T>::value>::type * = nullptr>
std::string to_hex(T const value, std::string const & prefix) {
    std::stringstream stream;
    stream << prefix << std::hex << value;
    return stream.str();
}

/// Convert a series of bytes to the corresponding hex string.
/// @example to_hex("A\x69") == "4169"
template <class T, typename std::enable_if<!(std::is_fundamental<T>::value && std::is_integral<T>::value)>::type * = nullptr>
std::string to_hex(T const & input) {
    return to_hex(input.begin(), input.end(), "");
}

template <typename T, typename std::enable_if<std::is_fundamental<T>::value && std::is_integral<T>::value>::type * = nullptr>
std::string to_hex(T const value) {
    return to_hex(value, "");
}

/// Convert a series of bytes to the corresponding hex string with 0x prefix.
/// @example to_hex_prefixed("A\x69") == "0x4169"
template <class T>
std::string to_hex_prefixed(T const & input) {
    return to_hex(input.begin(), input.end(), "0x");
}

/// Convert a series of bytes to the corresponding hex string.
/// @example to_hex_shrink_0("A\x0000123") == "123"
template <class T>
std::string to_hex_shrink_0(T const & input) {
    std::string hex_str = to_hex(input.begin(), input.end(), "");
    hex_str.erase(0, hex_str.find_first_not_of('0'));
    if (hex_str.size() == 0) {
        hex_str = "0";
    }
    return hex_str;
}

/// Convert a series of bytes to the corresponding hex string with 0x prefix.
/// @example to_hex_prefixed("A\x0000123") == "0x123"
template <class T>
std::string to_hex_prefixed_shrink_0(T const & input) {
    std::string hex_str = to_hex_shrink_0(input);
    hex_str = "0x" + hex_str;
    return hex_str;
}

constexpr std::uint8_t const_from_hex_char(char const i) {
    return ((i >= 'a') && (i <= 'f')) ? (i - 87) : // NOLINT
           ((i >= 'A') && (i <= 'F')) ? (i - 55) : // NOLINT
           ((i >= '0') && (i <= '9')) ? (i - 48) : // NOLINT
           throw std::exception{};                 // NOLINT
}

constexpr std::uint8_t const_hex_char(char const h, char const l) {
    return (const_from_hex_char(h) << 4) | (const_from_hex_char(l));
}

/* Adapter that performs sets of 2 characters into a single byte and combine the results into a uniform initialization list used to initialize T */
template <typename T, std::size_t Length, std::size_t... Index>
constexpr T ConstHexBytes(const char (&Input)[Length], const index_sequence<Index...> &) {
    return T{const_hex_char(Input[(Index * 2)], Input[((Index * 2) + 1)])...};
}

template <typename T, std::size_t Length, std::size_t... Index>
constexpr T ConstBytes(const char (&Input)[Length], const index_sequence<Index...> &) {
    return T{static_cast<uint8_t>(Input[Index])...};
}

/* Entry function */
template <typename T, std::size_t Length>
constexpr T ConstHexBytes(const char (&input)[Length]) {
    return ConstHexBytes<T>(input, make_index_sequence<(Length / 2)>{});
}

/* Entry function */
template <std::size_t Length>
constexpr std::array<std::uint8_t, Length> ConstHexBytes(const char (&input)[2 * Length + 1]) {
    return ConstHexBytes<std::array<std::uint8_t, Length>>(input, make_index_sequence<(Length)>{});
}

/* Entry function */
template <typename T, std::size_t Length>
constexpr T ConstBytes(const char (&input)[Length + 1]) {
    return ConstBytes<T>(input, make_index_sequence<(Length)>{});
}

/* Entry function */
template <std::size_t Length>
constexpr std::array<std::uint8_t, Length> ConstBytes(const char (&input)[Length + 1]) {
    return ConstBytes<std::array<std::uint8_t, Length>>(input, make_index_sequence<(Length)>{});
}

/// Converts a (printable) ASCII hex string into the corresponding byte stream.
/// @example fromHex("41626261") == asBytes("Abba")
/// If _throw = ThrowType::DontThrow, it replaces bad hex characters with 0's, otherwise it will throw an exception.
xbytes_t from_hex(xstring_view_t input, std::error_code & ec);

/// Converts a (printable) ASCII hex string into the corresponding byte stream.
/// @example fromHex("41626261") == asBytes("Abba")
/// Throw xtop_error_t exception when error occurs.
xbytes_t from_hex(xstring_view_t input);

bool has_hex_prefix(xstring_view_t input) noexcept;

/// @returns true if @a input is a hex string.
bool is_hex_string(std::string const & input) noexcept;

bool is_hex_string_with_prefix(xstring_view_t input) noexcept;
bool is_hex_string_without_prefix(xstring_view_t input) noexcept;

template <typename T,
          typename std::enable_if<std::is_same<T, uint8_t>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value ||
                                  std::is_same<T, uint64_t>::value>::type * = nullptr>
auto hex_to(std::string const & input) -> T {
    xstring_view_t input_view{input.c_str(), input.size()};

    if (input.compare(0, 2, "0x") == 0 || input.compare(0, 2, "0X") == 0) {
        input_view.remove_prefix(2);
    }
    assert(std::all_of(std::begin(input_view), std::end(input_view), [](char const ch) { return std::isxdigit(ch); }));

    T ret = 0;
    for (auto const c : input_view) {
        ret <<= 4;
        if (c >= '0' && c <= '9') {
            ret |= (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            ret |= (c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            ret |= (c - 'A' + 10);
        } else {
            return 0;
        }
    }
    return ret;
}

NS_END1
