// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhex.h"
#include "xutility/xhash.h"
#include "xcommon/xbloom9.h"
#include "xcommon/xerror/xerror.h"
#include <tuple>

NS_BEG2(top, evm_common)

void xtop_bloom9::add(xbytes_t const & data) {
    auto res = bloomValues(data);
    m_data[std::get<0>(res)] |= std::get<1>(res);
    m_data[std::get<2>(res)] |= std::get<3>(res);
    m_data[std::get<4>(res)] |= std::get<5>(res);
}

bool xtop_bloom9::contain(xbytes_t const & input) {
    auto res = bloomValues(input);
    return (std::get<1>(res) == (std::get<1>(res) & m_data[std::get<0>(res)])) &&  // NOLINT
            (std::get<3>(res) == (std::get<3>(res) & m_data[std::get<2>(res)])) &&  // NOLINT
            (std::get<5>(res) == (std::get<5>(res) & m_data[std::get<4>(res)]));    // NOLINT
}

std::string xtop_bloom9::to_hex_string() const {
    // std::string result;
    // result.reserve(m_data.size() * 2);  // two digits per character

    // static constexpr char hex[] = "0123456789abcdef";

    // for (uint8_t c : m_data) {
    //     result.push_back(hex[c / 16]);
    //     result.push_back(hex[c % 16]);
    // }

    // return result;
    return top::to_hex_prefixed(m_data);
}

std::tuple<std::size_t, xbyte_t, std::size_t, xbyte_t, std::size_t, xbyte_t> xtop_bloom9::bloomValues(xbytes_t data) {
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

xtop_bloom9 xtop_bloom9::build_from(xbytes_t const & _data, std::error_code & ec) {
    if (_data.size() != Bloom9ByteLength) {
        ec = common::error::xerrc_t::invalid_bloom;
        return {};
    }
    return xtop_bloom9(_data);
}


NS_END2
