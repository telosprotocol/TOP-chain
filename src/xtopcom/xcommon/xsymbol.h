// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbase/xns_macro.h"

#include <string>

NS_BEG2(top, common)

class xtop_symbol;

std::int32_t operator<<(top::base::xstream_t & stream, xtop_symbol const & symbol);
std::int32_t operator>>(top::base::xstream_t & stream, xtop_symbol & symbol);
std::int32_t operator<<(top::base::xbuffer_t & buffer, xtop_symbol const & symbol);
std::int32_t operator>>(top::base::xbuffer_t & buffer, xtop_symbol & symbol);

class xtop_symbol {
    std::string symbol_;

public:
    xtop_symbol() = default;
    xtop_symbol(xtop_symbol const &) = default;
    xtop_symbol & operator=(xtop_symbol const &) = default;
    xtop_symbol(xtop_symbol &&) = default;
    xtop_symbol & operator=(xtop_symbol &&) = default;
    ~xtop_symbol() = default;

    xtop_symbol(std::string symbol) noexcept;

    void swap(xtop_symbol & other) noexcept;
    void clear() noexcept;
    bool empty() const noexcept;
    std::string const & to_string() const noexcept;
    char const * c_str() const noexcept;
    size_t length() const noexcept;

    bool operator==(xtop_symbol const & other) const noexcept;
    bool operator<(xtop_symbol const & other) const noexcept;

    bool operator!=(xtop_symbol const & other) const noexcept;
    bool operator<=(xtop_symbol const & other) const noexcept;
    bool operator>(xtop_symbol const & other) const noexcept;
    bool operator>=(xtop_symbol const & other) const noexcept;

    std::int32_t serialize_to(base::xstream_t & stream) const;
    std::int32_t serialize_from(base::xstream_t & stream);
    std::int32_t serialize_to(base::xbuffer_t & buffer) const;
    std::int32_t serialize_from(base::xbuffer_t & buffer);

private:
    std::int32_t do_read(base::xstream_t & stream);

    std::int32_t do_write(base::xstream_t & stream) const;
};
using xsymbol_t = xtop_symbol;

extern xsymbol_t const SYMBOL_TOP_TOKEN;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::common::xsymbol_t> final {
    std::size_t operator()(top::common::xsymbol_t const & symbol) const noexcept;
};

NS_END1
