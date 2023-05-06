// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbinary.h"
#include "xbasic/xendian.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xhex.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <iterator>
#include <string>
#include <system_error>

NS_BEG1(top)

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
    enum class format { binary, hex };

    static xtop_bitset build_from(std::string const & str, format const fmt, std::error_code & ec) {
        assert(!ec);
        switch (fmt) {
        case format::hex: {
            if (!is_hex_string(str)) {
                ec = error::xbasic_errc_t::invalid_hex_string;
                return {};
            }

            auto it = std::begin(str);
            if (has_hex_prefix(str)) {
                it += 2;
            }

            std::string binary_str;
            std::transform(it, std::end(str), std::back_inserter(binary_str), [](char const ch) { return nibble_binary_cstr[const_from_hex_char(ch)]; });

            return {std::bitset<N>{binary_str}};
        }

        case format::binary: {
            xstring_view_t str_view = str;
            if (has_binary_prefix(str_view)) {
                str_view = str_view.substr(2);
            }

            if (!is_binary_string_without_prefix(str_view)) {
                ec = error::xbasic_errc_t::invalid_binary_string;
                return {};
            }

            return {std::bitset<N>{str_view.data()}};
        }

        default:
            assert(false);
            break;
        }

        return {};
    }

    static xtop_bitset build_from(xbytes_t const & bytes, xendian_t endian, std::error_code & ec) {
        assert(!ec);
        std::array<uint8_t, N / 8> arr;
        std::copy(std::begin(bytes), std::end(bytes), std::begin(arr));
        if (endian == xendian_t::little) {
            std::reverse(std::begin(arr), std::end(arr));
        }
        return {std::bitset<N>{arr}};
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

    xbytes_t to_bytes(xendian_t const endian) const {
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
