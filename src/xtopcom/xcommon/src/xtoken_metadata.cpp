// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xtoken_metadata.h"

#include "xbasic/xstring.h"
#include "xcommon/xerror/xerror.h"

#include <type_traits>

NS_BEG2(top, common)

observer_ptr<xtoken_metadata_t const> predefined_token_metadata(xtoken_id_t const token_id, std::error_code & ec) noexcept {
    static xtoken_metadata_t const top_token_metadata{xsymbol_t{"TOP"}, std::string{"TOP"}, xtoken_id_t::top};
    static xtoken_metadata_t const eth_token_metadata{xsymbol_t{"ETH"}, std::string{"ETH"}, xtoken_id_t::eth};
    static xtoken_metadata_t const usdt_token_metadata{xsymbol_t{"USDT"}, std::string{"USDT"}, xtoken_id_t::usdt};
    static xtoken_metadata_t const usdc_token_metadata{xsymbol_t{"USDC"}, std::string{"USDC"}, xtoken_id_t::usdc};

    assert(!ec);

    xtoken_id_t const id = static_cast<xtoken_id_t>(static_cast<std::underlying_type<xtoken_id_t>::type>(token_id) & 0x00FF);
    switch (id) {
    case xtoken_id_t::top:
        return top::make_observer(std::addressof(top_token_metadata));

    case xtoken_id_t::eth:
        return top::make_observer(std::addressof(eth_token_metadata));

    case xtoken_id_t::usdt:
        return top::make_observer(std::addressof(usdt_token_metadata));

    case xtoken_id_t::usdc:
        return top::make_observer(std::addressof(usdc_token_metadata));

    default:
        assert(false);
        ec = error::xerrc_t::token_not_predefined;
        return nullptr;
    }
}

bool operator==(xtoken_metadata_t const & lhs, xtoken_metadata_t const & rhs) noexcept {
    return lhs.token_symbol == rhs.token_symbol && lhs.token_id == rhs.token_id;
}

bool operator!=(xtoken_metadata_t const & lhs, xtoken_metadata_t const & rhs) noexcept {
    return !(lhs == rhs);
}

NS_END2

NS_BEG1(top)

template <>
std::string to_string<common::xtoken_id_t>(common::xtoken_id_t const & token_id) {
    return std::to_string(static_cast<std::underlying_type<common::xtoken_id_t>::type>(token_id));
}

template <>
common::xtoken_id_t from_string<common::xtoken_id_t>(std::string const & input, std::error_code &) {
    return static_cast<common::xtoken_id_t>(static_cast<std::underlying_type<common::xtoken_id_t>::type>(std::stoul(input)));
}

template <>
xbytes_t to_bytes<common::xtoken_id_t>(common::xtoken_id_t const & token_id) {
    return to_bytes(std::to_string(static_cast<std::underlying_type<common::xtoken_id_t>::type>(token_id)));
}

template <>
common::xtoken_id_t from_bytes<common::xtoken_id_t>(xbytes_t const & input, std::error_code &) {
    return static_cast<common::xtoken_id_t>(static_cast<std::underlying_type<common::xtoken_id_t>::type>(std::stoul(top::to_string(input))));
}

template <>
xbyte_t to_byte<common::xtoken_id_t>(common::xtoken_id_t const & token_id) {
    return static_cast<xbyte_t>(static_cast<std::underlying_type<common::xtoken_id_t>::type>(token_id));
}

template <>
common::xtoken_id_t from_byte<common::xtoken_id_t>(xbyte_t input) {
    return static_cast<common::xtoken_id_t>(static_cast<std::underlying_type<common::xtoken_id_t>::type>(input));
}

NS_END1

#if !defined(XCXX14_OR_ABOVE)

NS_BEG1(std)

std::size_t hash<top::common::xtoken_id_t>::operator()(top::common::xtoken_id_t const token_id) const {
    return static_cast<std::underlying_type<top::common::xtoken_id_t>::type>(token_id);
}

NS_END1

#endif
