// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xeth_address.h"

#include "xstatistic/xbasic_size.hpp"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xhex.h"
#include "xbasic/xstring.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xerror/xerror.h"

#include <algorithm>
#include <cassert>
#include <random>

NS_BEG2(top, common)

xtop_eth_address xtop_eth_address::build_from(xaccount_address_t const & account_address, std::error_code & ec) {
    if (account_address.type() != base::enum_vaccount_addr_type_secp256k1_eth_user_account && account_address.type() != base::enum_vaccount_addr_type_secp256k1_evm_user_account) {
        ec = common::error::xerrc_t::invalid_account_type;
        return {};
    }

    auto const account_string = account_address.to_string().substr(6);
    assert(account_string.size() == 40);
    return xeth_address_t{account_string, ec};
}

xtop_eth_address xtop_eth_address::build_from(xaccount_address_t const & account_address) {
    std::error_code ec;
    auto ret = xtop_eth_address::build_from(account_address, ec);
    top::error::throw_error(ec);

    return ret;
}

xtop_eth_address xtop_eth_address::build_from(std::array<uint8_t, 20> const & address_data) {
    return xeth_address_t{address_data};
}

xtop_eth_address xtop_eth_address::build_from(xspan_t<xbyte_t const> address_data, std::error_code & ec) {
    assert(!ec);

    if (address_data.size() != xtop_eth_address::size()) {
        ec = common::error::xerrc_t::invalid_account_address;
        return {};
    }

    std::array<uint8_t, xtop_eth_address::size()> addr_data{};
    std::copy_n(std::begin(address_data), xtop_eth_address::size(), std::begin(addr_data));
    return xtop_eth_address{addr_data};
}

xtop_eth_address xtop_eth_address::build_from(xspan_t<xbyte_t const> const address_data) {
    std::error_code ec;
    auto ret = xtop_eth_address::build_from(address_data, ec);
    top::error::throw_error(ec);

    return ret;
}

void xtop_eth_address::build_from(xspan_t<xbyte_t const> address_data, xtop_eth_address & address, std::error_code & ec) {
    assert(!ec);
    assert(address.is_zero());

    if (address_data.size() != xtop_eth_address::size()) {
        ec = common::error::xerrc_t::invalid_account_address;
        return;
    }

    std::copy_n(std::begin(address_data), xtop_eth_address::size(), std::begin(address));
}

void xtop_eth_address::build_from(xspan_t<xbyte_t const> const address_data, xtop_eth_address & address) {
    assert(address_data.size() == xtop_eth_address::size());
    std::error_code ec;
    build_from(address_data, address, ec);
    top::error::throw_error(ec);
}

xtop_eth_address xtop_eth_address::build_from(xstring_view_t const hex_string, std::error_code & ec) {
    assert(!ec);

    if (hex_string.empty()) {
        return xtop_eth_address{};
    }

    auto bytes = from_hex(hex_string, ec);
    if (ec) {
        return xtop_eth_address{};
    }

    return xtop_eth_address::build_from(bytes, ec);
}

xtop_eth_address xtop_eth_address::build_from(xstring_view_t const hex_string) {
    std::error_code ec;
    auto ret = xtop_eth_address::build_from(hex_string, ec);
    top::error::throw_error(ec);
    return ret;
}

xtop_eth_address::xtop_eth_address() {
    std::fill(std::begin(raw_address_), std::end(raw_address_), 0);
}

xtop_eth_address::xtop_eth_address(std::array<uint8_t, 20> const & raw_account_address) : raw_address_(raw_account_address) {
}

xtop_eth_address::xtop_eth_address(std::string const & account_string) {
    std::error_code ec;
    auto const & bytes = top::from_hex({account_string.data(), account_string.size()}, ec);
    top::error::throw_error(ec);

    assert(bytes.size() == raw_address_.size());

    std::copy(std::begin(bytes), std::end(bytes), std::begin(raw_address_));
}

xtop_eth_address::xtop_eth_address(std::string const & account_string, std::error_code & ec) {
    auto const & bytes = top::from_hex({account_string.data(), account_string.size()}, ec);
    assert(bytes.size() == raw_address_.size());

    std::copy(std::begin(bytes), std::end(bytes), std::begin(raw_address_));
}

std::string const & xtop_eth_address::to_hex_string() const {
    if (hex_string_.empty()) {
        hex_string_ = top::to_hex_prefixed(raw_address_);
    }

    return hex_string_;
}

std::string xtop_eth_address::to_string() const {
    return top::to_string(to_bytes());
}

xbytes_t xtop_eth_address::to_bytes() const {
    return {std::begin(raw_address_), std::end(raw_address_)};
}

xbytes_t xtop_eth_address::to_h160() const {
    return to_bytes();
}

void xtop_eth_address::to_h160(xh160_t & h160) const {
    std::copy_n(std::begin(raw_address_), raw_address_.size(), std::begin(h160.asArray()));
}

xbytes_t xtop_eth_address::to_h256() const {
    xbytes_t h256(32, 0);
    xbytes_t h160 = to_h160();
    std::copy_n(std::begin(h160), h160.size(), std::next(std::begin(h256), 12));
    return h256;
}

void xtop_eth_address::to_h256(xh256_t & h256) const {
    std::copy_n(std::begin(raw_address_), raw_address_.size(), std::next(std::begin(h256.asArray()), 12));
}

char const * xtop_eth_address::c_str() const {
    return to_hex_string().c_str();
}

xbyte_t const * xtop_eth_address::data() const noexcept {
    return raw_address_.data();
}

xtop_eth_address const & xtop_eth_address::zero() {
    static xtop_eth_address const z;
    return z;
}

bool xtop_eth_address::is_zero() const {
    return raw_address_ == zero().raw_address_;
}

bool xtop_eth_address::operator==(xtop_eth_address const & rhs) const noexcept {
    return raw_address_ == rhs.raw_address_;
}

bool xtop_eth_address::operator!=(xtop_eth_address const & rhs) const noexcept {
    return !(*this == rhs);
}

void xtop_eth_address::clear() noexcept {
    xwarn("xeth_address_t %s cleared.");
    std::fill(std::begin(raw_address_), std::end(raw_address_), 0);
}

size_t xtop_eth_address::get_ex_alloc_size() const {
    return get_size(hex_string_);
}

xtop_eth_address::iterator xtop_eth_address::begin() noexcept {
    return raw_address_.begin();
}

xtop_eth_address::const_iterator xtop_eth_address::begin() const noexcept {
    return raw_address_.begin();
}

xtop_eth_address::const_iterator xtop_eth_address::cbegin() const noexcept {
       return raw_address_.end();
}

xtop_eth_address::iterator xtop_eth_address::end() noexcept {
    return raw_address_.end();
}

xtop_eth_address::const_iterator xtop_eth_address::end() const noexcept {
       return raw_address_.end();
}

xtop_eth_address::const_iterator xtop_eth_address::cend() const noexcept {
    return raw_address_.end();
}

xeth_address_t xtop_eth_address::random() {
    static std::random_device rd;
    std::uniform_int_distribution<> dist(0, 255);
    xtop_eth_address ret;
    for (auto & byte : ret.raw_address_) {
        byte = static_cast<xbyte_t>(dist(rd));
    }

    return ret;
}

void xtop_eth_address::random(xtop_eth_address & address) {
    static std::random_device rd;
    std::uniform_int_distribution<> dist(0, 255);
    for (auto & byte : address.raw_address_) {
        byte = static_cast<xbyte_t>(dist(rd));
    }
}

NS_END2
