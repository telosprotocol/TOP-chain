// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xutility/xhash.h"

#include <tuple>

NS_BEG2(top, evm_common)

static constexpr std::size_t Bloom9ByteLength = 256;

// https://github.com/ethereum/go-ethereum/blob/20356e57b119b4e70ce47665a71964434e15200d/core/types/bloom9.go
class xtop_bloom9 {
private:
    xbytes_t m_data;

public:
    xtop_bloom9() {
        m_data.resize(Bloom9ByteLength, xbyte_t{0});
    }

    xtop_bloom9 & operator|=(xtop_bloom9 const & _c) {
        for (unsigned i = 0; i < Bloom9ByteLength; ++i)
            m_data[i] |= _c.m_data[i];
        return *this;
    }
    xtop_bloom9 operator|(xtop_bloom9 const & _c) const {
        return xtop_bloom9(*this) |= _c;
    }

public:
    void add(xbytes_t const & data) {
        auto res = bloomValues(data);
        m_data[std::get<0>(res)] |= std::get<1>(res);
        m_data[std::get<2>(res)] |= std::get<3>(res);
        m_data[std::get<4>(res)] |= std::get<5>(res);
    }

    bool contain(xbytes_t const & input) {
        auto res = bloomValues(input);
        return (std::get<1>(res) == (std::get<1>(res) & m_data[std::get<0>(res)])) &&  // NOLINT
               (std::get<3>(res) == (std::get<3>(res) & m_data[std::get<2>(res)])) &&  // NOLINT
               (std::get<5>(res) == (std::get<5>(res) & m_data[std::get<4>(res)]));    // NOLINT
    }

    xbytes_t get_data() {
        return m_data;
    }

    std::string get_hex_string_data() {
        std::string result;
        result.reserve(m_data.size() * 2);  // two digits per character

        static constexpr char hex[] = "0123456789abcdef";

        for (uint8_t c : m_data) {
            result.push_back(hex[c / 16]);
            result.push_back(hex[c % 16]);
        }

        return result;
    }

private:
    std::tuple<std::size_t, xbyte_t, std::size_t, xbyte_t, std::size_t, xbyte_t> bloomValues(xbytes_t data) {
        xbytes_t hashbuf;
        utl::xkeccak256_t hasher;
        hasher.update(data.data(), data.size());
        hasher.get_hash(hashbuf);

        xbyte_t v1 = (xbyte_t)(1 << (hashbuf[1] & 0x7));
        xbyte_t v2 = (xbyte_t)(1 << (hashbuf[3] & 0x7));
        xbyte_t v3 = (xbyte_t)(1 << (hashbuf[5] & 0x7));

        std::size_t i1 = Bloom9ByteLength - std::size_t(((((uint16_t)hashbuf[1] | ((uint16_t)hashbuf[0] << 8))) & 0x7ff) >> 3) - 1;
        std::size_t i2 = Bloom9ByteLength - std::size_t(((((uint16_t)hashbuf[3] | ((uint16_t)hashbuf[2] << 8))) & 0x7ff) >> 3) - 1;
        std::size_t i3 = Bloom9ByteLength - std::size_t(((((uint16_t)hashbuf[5] | ((uint16_t)hashbuf[4] << 8))) & 0x7ff) >> 3) - 1;

        return std::make_tuple(i1, v1, i2, v2, i3, v3);
    }
};
using xbloom9_t = xtop_bloom9;

NS_END2
