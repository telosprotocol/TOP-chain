// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbinary.h"
#include "xbasic/xendian.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xhex.h"
#include "xbasic/xspan.h"
#include "xstring_view.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <iterator>
#include <string>
#include <system_error>

NS_BEG1(top)

enum class xtop_enum_string_format {
    binary,
    hex,
};

// bitset with fixed size in bits. The size must be a multiple of 8.
// The bitset is stored in little endian order.
// The first bit is the least significant bit. The last bit is the most significant bit.
// The bitset can be built from:
//  1) a string with specified format. The format can be binary or hex, thus the input string is the presentation of a binary or hex number. Hence the presentaion is in big endian order.
//      a. The string must contain only '0' and '1' with or without "0b" or "0B" prefix if the format is binary.
//      b. The string must contain only '0'-'9', 'a'-'f' and 'A'-'F' with or without "0x" or "0X" prefix if the format is hex.
//  2) a byte array with specified endian. The endian can be little or big. The size of the byte array can be in any size.
//  3) an unsigned integer.
template <size_t N>
class xtop_bitset {
    static_assert(N % 8 == 0, "N must be a multiple of 8");

    std::bitset<N> bitset_;

public:
    constexpr xtop_bitset() = default;
    xtop_bitset(xtop_bitset const &) = default;
    xtop_bitset(xtop_bitset&&) = default;
    xtop_bitset& operator=(xtop_bitset const &) = default;
    xtop_bitset& operator=(xtop_bitset&&) = default;
    ~xtop_bitset() = default;

    using reference = typename std::bitset<N>::reference;

private:
    xtop_bitset(std::bitset<N> value) : bitset_{std::move(value)} {
    }

public:
    using format = xtop_enum_string_format;

    /// @brief Build bitset from string with specified format.
    /// @param str_view The input string.
    /// @param fmt The format of the input string. If the format is binary, the string must contain only '0' and '1' with or without "0b" or "0B" prefix. If the format is hex, the string must contain only '0'-'9', 'a'-'f' and 'A'-'F' with or without "0x" or "0X" prefix.
    /// @param ec Holds the error code if the function fails.
    /// @return The bitset built from the input string.
    static xtop_bitset build_from(xstring_view_t str_view, format const fmt, std::error_code & ec) {
        assert(!ec);
        switch (fmt) {
        case format::hex: {
            if (!is_hex_string(str_view)) {
                ec = error::xbasic_errc_t::invalid_hex_string;
                return {};
            }

            if (has_hex_prefix(str_view)) {
                str_view.remove_prefix(2);
            }

            xbytes_t bytes{};
            if (str_view.length() & 1) {
                bytes.push_back(const_from_hex_char(str_view.front()));
                str_view.remove_prefix(1);
            }

            for (size_t i = 0; i < str_view.size(); i+=2) {
                bytes.push_back(static_cast<xbyte_t>(const_from_hex_char(str_view[i]) << 4 | const_from_hex_char(str_view[i + 1])));
            }

            return build_from(bytes, xendian_t::big);
        }

        case format::binary: {
            if (has_binary_prefix(str_view)) {
                str_view = str_view.substr(2);
            }

            if (!is_binary_string_without_prefix(str_view)) {
                ec = error::xbasic_errc_t::invalid_binary_string;
                return {};
            }
            str_view = str_view.substr(str_view.size() - std::min(N, str_view.size()));
            return {std::bitset<N>{str_view.data()}};
        }

        default:  // NOLINT(clang-diagnostic-covered-switch-default)
            assert(false);
            break;
        }

        ec = error::xbasic_errc_t::invalid_format;
        return {};
    }

    /// @brief Build bitset object from bytes with specified endian.
    /// @param bytes The input bytes.
    /// @param bytes_endian The endian of the input bytes.
    /// @return The bitset built from the input bytes.
    static xtop_bitset build_from(xspan_t<xbyte_t const> const bytes, xendian_t const bytes_endian) {
        if (bytes.empty()) {
            return {};
        }

        using xword_t = uint64_t;
        constexpr size_t bytes_per_word{sizeof(xword_t)};
        constexpr size_t bits_per_byte{8};
        constexpr size_t bits_per_word{bytes_per_word * bits_per_byte};

        size_t const num_words = (bytes.size() + bytes_per_word - 1) / bytes_per_word;

        if (bytes_endian == xendian_t::little) {
            std::bitset<N> bits;

            for (size_t i = 0; i < num_words; i++) {
                xword_t word = 0;
                size_t bytes_remaining = bytes.size() - i * bytes_per_word;
                size_t const num_bytes = std::min<size_t>(bytes_per_word, bytes_remaining);
                for (size_t j = 0; j < num_bytes; j++) {
                    word |= static_cast<xword_t>(bytes[i * bytes_per_word + j]) << (bits_per_byte * j);
                }
                for (size_t j = 0; j < bits_per_word; j++) {
                    size_t bit_index = bits_per_word * i + j;
                    if (bit_index < N) {
                        bits[bit_index] = (word >> j) & 1;
                    } else {
                        break;
                    }
                }
            }

            return {std::move(bits)};
        }

        if (bytes_endian == xendian_t::big) {
            std::bitset<N> bits;

            size_t const num_bits = std::min<size_t>(N, bits_per_word * num_words);
            for (size_t i = 0; i < num_words; i++) {
                uint64_t word = 0;
                size_t bytes_remaining = bytes.size() - i * bytes_per_word;
                size_t const num_bytes = std::min<size_t>(bytes_per_word, bytes_remaining);
                for (size_t j = 0; j < num_bytes; j++) {
                    word |= static_cast<uint64_t>(bytes[bytes.size() - 1 - (i * bytes_per_word + j)]) << (bits_per_byte * j);
                }
                for (size_t j = 0; j < bits_per_word; j++) {
                    size_t const bit_index = bits_per_word * i + j;
                    if (bit_index < num_bits) {
                        bits[bit_index] = (word >> j) & 1;
                    } else {
                        break;
                    }
                }
            }

            return {std::move(bits)};
        }

        unreachable();
    }

    /// @brief Build bitset object from an unsigned integer.
    /// @tparam T Template type of the input integer.
    /// @param integer The input integer.
    /// @return The bitset built from the input integer.
    template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type * = nullptr>
    static xtop_bitset build_from(T const integer) {
        size_t const bits_count = std::min(N, sizeof(T) * 8);
        std::bitset<N> bits;
        for (size_t i = 0; i < bits_count; ++i) {
            bits[i] = (integer >> i) & 1;
        }

        return {std::move(bits)};
    }

    reference operator[](size_t const pos) {
        return bitset_[pos];
    }

    bool operator[](size_t const pos) const {
        return bitset_[pos];
    }

    bool test(size_t const pos) const {
        return bitset_.test(pos);
    }

    bool all() const {
        return bitset_.all();
    }

    bool any() const {
        return bitset_.any();
    }

    bool none() const {
        return bitset_.none();
    }

    size_t count() const {
        return bitset_.count();
    }

    size_t size() const {
        return bitset_.size();
    }

    xtop_bitset & operator&=(xtop_bitset const & rhs) {
        bitset_ &= rhs.bitset_;
        return *this;
    }

    xtop_bitset & operator|=(xtop_bitset const & rhs) {
        bitset_ |= rhs.bitset_;
        return *this;
    }

    xtop_bitset & operator^=(xtop_bitset const & rhs) {
        bitset_ ^= rhs.bitset_;
        return *this;
    }

    xtop_bitset operator~() const {
        return {~bitset_};
    }

    xtop_bitset operator<<(size_t const pos) const {
        return {bitset_ << pos};
    }

    xtop_bitset & operator<<=(size_t const pos) {
        bitset_ <<= pos;
        return *this;
    }

    xtop_bitset operator>>(size_t const pos) const {
        return {bitset_ >> pos};
    }

    xtop_bitset & operator>>=(size_t const pos) {
        bitset_ >>= pos;
        return *this;
    }

    xtop_bitset & set() {
        bitset_.set();
        return *this;
    }

    xtop_bitset & set(size_t const pos, bool const value = true) {
        bitset_.set(pos, value);
        return *this;
    }

    xtop_bitset & reset() {
        bitset_.reset();
        return *this;
    }

    xtop_bitset & reset(size_t const pos) {
        bitset_.reset(pos);
        return *this;
    }

    xtop_bitset & flip() {
        bitset_.flip();
        return *this;
    }

    xtop_bitset & flip(size_t const pos) {
        bitset_.flip(pos);
        return *this;
    }

    std::string to_string(format const fmt = format::binary) const {
        switch (fmt) {
        case format::binary:
            return bitset_.to_string();
        case format::hex:
            return to_hex_string();
        default:
            assert(false);
            return {};
        }
    }

    template <typename T, typename std::enable_if<std::is_same<T, xbytes_t>::value>::type * = nullptr>
    xbytes_t to(xendian_t const endian) const {
        switch (endian) {
            case xendian_t::big:
                return to_bytes_big_endian();

            case xendian_t::little:
                return to_bytes_little_endian();

            default:  // NOLINT(clang-diagnostic-covered-switch-default)
                assert(false);
                break;
        }

        return {};
    }

    template <typename T, typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value>::type * = nullptr>
    T to() const {
        size_t const bits_count = std::min(N, sizeof(T) * 8);
        T result = 0;
        for (size_t i = 0; i < bits_count; ++i) {
            result |= static_cast<T>(bitset_[i]) << i;
        }
        return result;
    }

    struct hash {
        size_t operator()(xtop_bitset const & bitset) const {
            return std::hash<std::bitset<N>>{}(bitset.bitset_);
        }
    };

private:
    std::string to_hex_string() const {
        std::string result;
        result.resize(N / 4);
        for (size_t i = 0; i < N; i += 4) {
            assert((N - i) / 4 > 0);
            result[(N - i) / 4 - 1] = to_hex_char(bitset_[i], bitset_[i + 1], bitset_[i + 2], bitset_[i + 3]);
        }

        return result;
    }

    static char to_hex_char(bool const b0, bool const b1, bool const b2, bool const b3) {
        return top::to_hex_char(
            static_cast<uint8_t>(static_cast<uint8_t>(b0) | (static_cast<uint8_t>(b1) << 1) | (static_cast<uint8_t>(b2) << 2) | (static_cast<uint8_t>(b3) << 3)));
    }

    xbytes_t to_bytes_big_endian() const {
        xbytes_t result;
        result.resize(N / 8);
        for (size_t i = 0; i < N; i += 8) {
            assert((N - i) / 8 > 0);
            result[(N - i) / 8 - 1] = static_cast<uint8_t>(bitset_[i] | (bitset_[i + 1] << 1) | (bitset_[i + 2] << 2) | (bitset_[i + 3] << 3) | (bitset_[i + 4] << 4) |
                                                           (bitset_[i + 5] << 5) | (bitset_[i + 6] << 6) | (bitset_[i + 7] << 7));
        }
        return result;
    }

    xbytes_t to_bytes_little_endian() const {
        xbytes_t result;
        result.resize(N / 8);
        for (size_t i = 0; i < N; i += 8) {
            assert(i / 8 < result.size());
            result[i / 8] = static_cast<uint8_t>(bitset_[i] | (bitset_[i + 1] << 1) | (bitset_[i + 2] << 2) | (bitset_[i + 3] << 3) | (bitset_[i + 4] << 4) |
                                                 (bitset_[i + 5] << 5) | (bitset_[i + 6] << 6) | (bitset_[i + 7] << 7));
        }
        return result;
    }
};

using xstring_format_t = xtop_enum_string_format;

template <size_t N>
using xbitset_t = xtop_bitset<N>;

template <size_t N>
xbitset_t<N> operator&(xbitset_t<N> const & lhs, xbitset_t<N> const & rhs) {
    return xbitset_t<N>{lhs} &= rhs;
}

template <size_t N>
xbitset_t<N> operator|(xbitset_t<N> const & lhs, xbitset_t<N> const & rhs) {
       return xbitset_t<N>{lhs} |= rhs;
}

template <size_t N>
xbitset_t<N> operator^(xbitset_t<N> const & lhs, xbitset_t<N> const & rhs) {
       return xbitset_t<N>{lhs} ^= rhs;
}

NS_END1

NS_BEG1(std)

template <size_t N>
struct hash<top::xbitset_t<N>> : top::xbitset_t<N>::hash {};

NS_END1
