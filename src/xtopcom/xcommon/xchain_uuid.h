// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xstring.h"

#include <cstdint>

NS_BEG2(top, common)

enum class xtop_enum_chain_uuid : uint8_t {
    invalid = 0,
    top = 1,
    eth = 2,
    bsc = 3,
    heco = 4,
};
using xchain_uuid_t = xtop_enum_chain_uuid;

NS_END2

NS_BEG1(top)

template <>
std::string to_string<common::xchain_uuid_t>(common::xchain_uuid_t const & chain_uuid);

template <>
common::xchain_uuid_t from_string<common::xchain_uuid_t>(std::string const & input, std::error_code & ec);

template <>
xbytes_t to_bytes<common::xchain_uuid_t>(common::xchain_uuid_t const & chain_uuid);

template <>
common::xchain_uuid_t from_bytes<common::xchain_uuid_t>(xbytes_t const & input, std::error_code & ec);

template <>
xbyte_t to_byte<common::xchain_uuid_t>(common::xchain_uuid_t const & chain_uuid);

template <>
common::xchain_uuid_t from_byte<common::xchain_uuid_t>(xbyte_t input);

NS_END1
