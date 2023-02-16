// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xstring.h"
#include "xcommon/xsymbol.h"

#include <cstdint>
#include <string>

NS_BEG2(top, common)

enum class xtop_token_id : uint8_t {
    invalid = 0,
    top = 1,
    eth = 2,
    usdt = 3,
    usdc = 4,
};
using xtoken_id_t = xtop_token_id;

struct xtop_token_metadata {
    xsymbol_t token_symbol;
    std::string token_name;
    xtoken_id_t token_id;
};
using xtoken_metadata_t = xtop_token_metadata;

observer_ptr<xtoken_metadata_t const> predefined_token_metadata(xtoken_id_t const token_id, std::error_code & ec) noexcept;
bool operator==(xtoken_metadata_t const & lhs, xtoken_metadata_t const & rhs) noexcept;
bool operator!=(xtoken_metadata_t const & lhs, xtoken_metadata_t const & rhs) noexcept;

xsymbol_t symbol(xtoken_id_t const token_id, std::error_code & ec);
xsymbol_t symbol(xtoken_id_t const token_id);
xtoken_id_t token_id(xsymbol_t const & symbol, std::error_code & ec);
xtoken_id_t token_id(xsymbol_t const & symbol);

NS_END2

NS_BEG1(top)

template <>
std::string to_string<common::xtoken_id_t>(common::xtoken_id_t const & token_id);

template <>
common::xtoken_id_t from_string<common::xtoken_id_t>(std::string const & input, std::error_code & ec);

template <>
xbytes_t to_bytes<common::xtoken_id_t>(common::xtoken_id_t const & token_id);

template <>
common::xtoken_id_t from_bytes<common::xtoken_id_t>(xbytes_t const & input, std::error_code & ec);

template <>
xbyte_t to_byte<common::xtoken_id_t>(common::xtoken_id_t const & token_id);

template <>
common::xtoken_id_t from_byte<common::xtoken_id_t>(xbyte_t input);

NS_END1

#if !defined(XCXX14)

#include <functional>

NS_BEG1(std)

template <>
struct hash<top::common::xtoken_id_t> {
    std::size_t operator()(top::common::xtoken_id_t const token_id) const;
};

NS_END1

#endif
