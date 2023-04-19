// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"

#include <tuple>
#include <algorithm>

NS_BEG2(top, evm_common)

static constexpr std::size_t Bloom9ByteLength = 256;

// https://github.com/ethereum/go-ethereum/blob/20356e57b119b4e70ce47665a71964434e15200d/core/types/bloom9.go
class xtop_bloom9 {
private:
    xbytes_t m_data;

public:
    static xtop_bloom9 build_from(xbytes_t const & _data, std::error_code & ec);

public:
    xtop_bloom9()
      : m_data(static_cast<size_t>(Bloom9ByteLength), static_cast<xbyte_t>(0)){
    }
    explicit xtop_bloom9(xbytes_t const& data) {
        m_data = data;
    }

    xtop_bloom9 & operator|=(xtop_bloom9 const & _c) {
        for (unsigned i = 0; i < Bloom9ByteLength; ++i)
            m_data[i] |= _c.m_data[i];
        return *this;
    }
    xtop_bloom9 operator|(xtop_bloom9 const & _c) const {
        return xtop_bloom9(*this) |= _c;
    }
    /// @returns true iff this is the empty hash.
    explicit operator bool() const {
        return std::any_of(m_data.begin(), m_data.end(), [](xbyte_t _b) { return _b != 0; });
    }
public:
    void add(xbytes_t const & data);

    bool contain(xbytes_t const & input);

    xbytes_t const& get_data() const {return m_data;}
    xbytes_t const& to_bytes() const {return m_data;}

    bool empty() const;

    std::string to_hex_string() const;

private:
    std::tuple<std::size_t, xbyte_t, std::size_t, xbyte_t, std::size_t, xbyte_t> bloomValues(xbytes_t data);
};
using xbloom9_t = xtop_bloom9;

NS_END2
