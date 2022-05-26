// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xutility.h"

#include <system_error>

NS_BEG1(top)

template <class Iterator>
std::string to_hex(Iterator begin, Iterator end, std::string const & prefix) {
    typedef std::iterator_traits<Iterator> traits;
    static_assert(sizeof(typename traits::value_type) == 1, "to_hex needs byte-sized element type");

    static char const * hexdigits = "0123456789abcdef";
    std::size_t off = prefix.size();
    std::string hex(std::distance(begin, end) * 2 + off, '0');
    hex.replace(0, off, prefix);
    for (; begin != end; begin++) {
        hex[off++] = hexdigits[(*begin >> 4) & 0x0f];
        hex[off++] = hexdigits[*begin & 0x0f];
    }
    return hex;
}

/// Convert a series of bytes to the corresponding hex string.
/// @example to_hex("A\x69") == "4169"
template <class T>
std::string to_hex(T const & input) {
    return to_hex(input.begin(), input.end(), "");
}

/// Convert a series of bytes to the corresponding hex string with 0x prefix.
/// @example to_hex_prefixed("A\x69") == "0x4169"
template <class T>
std::string to_hex_prefixed(T const & input) {
    return to_hex(input.begin(), input.end(), "0x");
}

namespace {
constexpr std::uint8_t const_from_hex_char(char i) {
    return ((i >= 'a') && (i <= 'f')) ? (i - 87) : // NOLINT
           ((i >= 'A') && (i <= 'F')) ? (i - 55) : // NOLINT
           ((i >= '0') && (i <= '9')) ? (i - 48) : // NOLINT
           throw std::exception{};                 // NOLINT
}

constexpr std::uint8_t const_hex_char(char h, char l) {
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

}  // namespace

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
xbytes_t from_hex(std::string const & input, std::error_code & ec);

/// Converts a (printable) ASCII hex string into the corresponding byte stream.
/// @example fromHex("41626261") == asBytes("Abba")
/// Throw xtop_error_t exception when error occurs.
xbytes_t from_hex(std::string const & input);

/// @returns true if @a input is a hex string.
bool is_hex_string(std::string const & input) noexcept;

/// @returns true if @a _hash is a hash conforming to FixedHash type @a T.
// template <class T>
// static bool isHash(std::string const & _hash) {
//     return (_hash.size() == T::size * 2 || (_hash.size() == T::size * 2 + 2 && _hash.substr(0, 2) == "0x")) && is_hex_string(_hash);
// }

NS_END1