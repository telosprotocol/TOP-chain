// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xchain_uuid.h"

#include <cassert>

NS_BEG1(top)

template <>
std::string to_string<common::xchain_uuid_t>(common::xchain_uuid_t const & chain_uuid) {
    return top::to_string(top::to_byte(chain_uuid));
}

template <>
common::xchain_uuid_t from_string<common::xchain_uuid_t>(std::string const & input, std::error_code & ec) {
    assert(!ec);
    auto const byte = top::from_string<xbyte_t>(input, ec);
    if (ec) {
        return common::xchain_uuid_t::invalid;
    }

    return top::from_byte<common::xchain_uuid_t>(byte);
}

template <>
xbytes_t to_bytes<common::xchain_uuid_t>(common::xchain_uuid_t const & chain_uuid) {
    return to_bytes(top::to_byte(chain_uuid));
}

template <>
common::xchain_uuid_t from_bytes<common::xchain_uuid_t>(xbytes_t const & input, std::error_code & ec) {
    assert(!ec);
    auto const byte = top::from_bytes<xbyte_t>(input, ec);
    if (ec) {
        return common::xchain_uuid_t::invalid;
    }

    return top::from_byte<common::xchain_uuid_t>(byte);
}

template <>
xbyte_t to_byte<common::xchain_uuid_t>(common::xchain_uuid_t const & chain_uuid) {
    return static_cast<xbyte_t>(static_cast<std::underlying_type<common::xchain_uuid_t>::type>(chain_uuid));
}

template <>
common::xchain_uuid_t from_byte<common::xchain_uuid_t>(xbyte_t input) {
    return static_cast<common::xchain_uuid_t>(static_cast<std::underlying_type<common::xchain_uuid_t>::type>(input));
}

NS_END1
