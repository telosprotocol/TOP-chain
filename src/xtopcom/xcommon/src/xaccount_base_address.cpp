// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xaccount_base_address.h"

#include "xbase/xlog.h"
#include "xbasic/xerror/xerror.h"
#include "xcommon/xerror/xerror.h"

#include <cassert>
#include <vector>

NS_BEG2(top, common)

xtop_account_base_address::xtop_account_base_address(std::string const & base_address) {
    if (base_address.find('@') != std::string::npos) {
        assert(false);
        top::error::throw_error(error::xerrc_t::invalid_account_base_address);
    }

    if (base_address.length() < base::xvaccount_t::enum_vaccount_address_prefix_size) {
        assert(false);
        top::error::throw_error(error::xerrc_t::invalid_account_base_address);
    }

    if (base_address.length() >= base::xvaccount_t::enum_vaccount_address_max_size) {
        assert(false);
        top::error::throw_error(error::xerrc_t::invalid_account_base_address);
    }

    if (base_address.at(0) != 'T') {
        assert(false);
        top::error::throw_error(error::xerrc_t::invalid_account_base_address);
    }

    auto const & prefix = base_address.substr(1, 4);    // "Tx000yblabla" => "x000"
    auto const t = static_cast<base::enum_vaccount_addr_type>(prefix.at(0));
    switch (t) {
    case base::enum_vaccount_addr_type::enum_vaccount_addr_type_black_hole:
        XATTRIBUTE_FALLTHROUGH;
    case base::enum_vaccount_addr_type::enum_vaccount_addr_type_clock:
        XATTRIBUTE_FALLTHROUGH;
    case base::enum_vaccount_addr_type::enum_vaccount_addr_type_drand:
        XATTRIBUTE_FALLTHROUGH;
    case base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_user_account:
        XATTRIBUTE_FALLTHROUGH;
    case base::enum_vaccount_addr_type::enum_vaccount_addr_type_native_contract:
        XATTRIBUTE_FALLTHROUGH;
    case base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_eth_user_account:
        XATTRIBUTE_FALLTHROUGH;
    case base::enum_vaccount_addr_type::enum_vaccount_addr_type_block_contract:
        m_account_type = t;
        break;

    default:
        assert(false);
        top::error::throw_error(error::xerrc_t::invalid_account_base_address);
        break;
    }

    if (prefix.at(1) != '0' || prefix.at(2) != '0' || prefix.at(3) != '0') {
        top::error::throw_error(error::xerrc_t::invalid_account_base_address);
    }

    m_base_address_str = base_address;

    m_account_index = base::xvaccount_t::get_index_from_account(m_base_address_str);
    m_ledger_id = static_cast<uint16_t>(base::xstring_utl::hex2uint64(m_base_address_str.substr(2, 4)));
    m_default_table_id = static_cast<uint16_t>(m_account_index % static_cast<uint32_t>(enum_vbucket_has_tables_count));
}

xtop_account_base_address xtop_account_base_address::build_from(std::string const & input, std::error_code & ec) {
    try {
        return build_from(input);
    } catch (top::error::xtop_error_t const & eh) {
        ec = eh.code();
        xwarn("invalid account address: %s", input.c_str());
    }

    return xtop_account_base_address{};
}

xtop_account_base_address xtop_account_base_address::build_from(std::string const & input) {
    return xaccount_base_address_t{input};
}

void xtop_account_base_address::swap(xtop_account_base_address & other) noexcept {
    std::swap(m_base_address_str, other.m_base_address_str);
    std::swap(m_account_type, other.m_account_type);
    std::swap(m_account_index, other.m_account_index);
    std::swap(m_ledger_id, other.m_ledger_id);
    std::swap(m_default_table_id, other.m_default_table_id);
}

std::string const & xtop_account_base_address::to_string() const noexcept {
    return m_base_address_str;
}

char const * xtop_account_base_address::c_str() const noexcept {
    return m_base_address_str.c_str();
}

bool xtop_account_base_address::empty() const noexcept {
    return m_base_address_str.empty();
}

void xtop_account_base_address::clear() {
    m_default_table_id = std::numeric_limits<uint16_t>::max();
    m_ledger_id = std::numeric_limits<uint32_t>::max();
    m_account_index = std::numeric_limits<uint32_t>::max();
    m_account_type = base::enum_vaccount_addr_type::enum_vaccount_addr_type_invalid;
    m_base_address_str.clear();
}

base::enum_vaccount_addr_type xtop_account_base_address::type(std::error_code & ec) const {
    if (m_account_type == base::enum_vaccount_addr_type_invalid) {
        ec = error::xerrc_t::invalid_account_type;
    }

    return m_account_type;
}

uint32_t xtop_account_base_address::ledger_id(std::error_code & ec) const {
    if (m_ledger_id == std::numeric_limits<uint32_t>::max()) {
        ec = error::xerrc_t::invalid_ledger_id;
    }
    return m_ledger_id;
}

uint16_t xtop_account_base_address::default_table_id(std::error_code & ec) const {
    if (m_default_table_id == std::numeric_limits<uint32_t>::max()) {
        ec = error::xerrc_t::invalid_table_id;
    }
    return m_default_table_id;
}

base::enum_vaccount_addr_type xtop_account_base_address::type() const {
    std::error_code ec;
    auto const r = type(ec);
    top::error::throw_error(ec);
    return r;
}

uint32_t xtop_account_base_address::ledger_id() const {
    std::error_code ec;
    auto const r = ledger_id(ec);
    top::error::throw_error(ec);
    return r;
}

uint16_t xtop_account_base_address::default_table_id() const {
    std::error_code ec;
    auto const r = default_table_id(ec);
    top::error::throw_error(ec);
    return r;
}

bool xtop_account_base_address::operator==(xtop_account_base_address const & other) const noexcept {
    return m_base_address_str == other.m_base_address_str;
}

bool xtop_account_base_address::operator<(xtop_account_base_address const & other) const noexcept {
    return m_base_address_str < other.m_base_address_str;
}

bool xtop_account_base_address::operator>(xtop_account_base_address const & other) const noexcept {
    return other < *this;
}

bool xtop_account_base_address::operator<=(xtop_account_base_address const & other) const noexcept {
    return !(other < *this);
}

bool xtop_account_base_address::operator>=(xtop_account_base_address const & other) const noexcept {
    return !(*this < other);
}

bool xtop_account_base_address::operator!=(xtop_account_base_address const & other) const noexcept {
    return !(*this == other);
}

int32_t xtop_account_base_address::serialize_to(base::xstream_t & stream) const {
    return do_write(stream);
}

int32_t xtop_account_base_address::serialize_from(base::xstream_t & stream) {
    return do_read(stream);
}

int32_t xtop_account_base_address::do_write(base::xstream_t & stream) const {
    return stream << m_base_address_str;
}
int32_t xtop_account_base_address::do_read(base::xstream_t & stream){
    return stream >> m_base_address_str;
}

int32_t operator>>(base::xstream_t & stream, xaccount_base_address_t & account_base_address) {
    return account_base_address.serialize_to(stream);
}

int32_t operator>>(base::xbuffer_t & buffer, xaccount_base_address_t & account_base_address) {
    std::string account_string;
    auto const r = buffer >> account_string;
    account_base_address = xaccount_base_address_t{account_string};
    return r;
}

int32_t operator<<(base::xstream_t & stream, xaccount_base_address_t const & account_base_address) {
    return account_base_address.serialize_to(stream);
}
int32_t operator<<(base::xbuffer_t & buffer, xaccount_base_address_t const & account_base_address) {
    return buffer << account_base_address.to_string();
}

NS_END2

NS_BEG1(std)

size_t hash<top::common::xaccount_base_address_t>::operator()(top::common::xaccount_base_address_t const & input) const {
    return std::hash<std::string>{}(input.to_string());
}

NS_END1
