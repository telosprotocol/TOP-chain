// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xnode_id.h"

#include "xbase/xutl.h"
#include "xbasic/xbyte_buffer.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xeth_address.h"
#include "xutility/xhash.h"
#include "xvledger/xvaccount.h"

#include <cassert>
#include <vector>

NS_BEG2(top, common)

xtop_node_id::xtop_node_id(std::string value) : m_account_string{std::move(value)} {
    if (!empty()) {
        parse();
    }
}

xtop_node_id::xtop_node_id(xaccount_base_address_t base_address) : m_account_string {base_address.to_string()}, m_account_base_address{std::move(base_address)} {
    if (!empty()) {
        parse();
    }
}

xtop_node_id::xtop_node_id(xaccount_base_address_t base_address, uint16_t const table_id_value) : xtop_node_id{std::move(base_address), xtable_id_t{table_id_value}} {
}

xtop_node_id::xtop_node_id(xaccount_base_address_t base_address, xtable_id_t table_id)
  : m_account_string{base_address.to_string() + "@" + top::to_string(table_id)}
  , m_account_base_address{std::move(base_address)}
  , m_assigned_table_id{table_id} {
}

xtop_node_id xtop_node_id::build_from(std::string const & account_string, std::error_code & ec) {
    try {
        return build_from(account_string);
    } catch (top::error::xtop_error_t const & eh) {
        ec = eh.code();
    }
    return xtop_node_id{};
}

xtop_node_id xtop_node_id::build_from(std::string const & account_string) {
    return xtop_node_id{account_string};
}

xtop_node_id xtop_node_id::build_from(xeth_address_t const & eth_address, base::enum_vaccount_addr_type account_addr_type, std::error_code & ec) {
    assert(!ec);

    if (account_addr_type == base::enum_vaccount_addr_type_secp256k1_evm_user_account) {
        return xtop_node_id::build_from(base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN + eth_address.to_hex_string().substr(2), ec);
    }

    if (account_addr_type == base::enum_vaccount_addr_type_secp256k1_eth_user_account) {
        return xtop_node_id::build_from(base::ADDRESS_PREFIX_ETH_TYPE_IN_MAIN_CHAIN + eth_address.to_hex_string().substr(2), ec);
    }

    ec = common::error::xerrc_t::invalid_account_type;
    return xtop_node_id{};
}

xtop_node_id xtop_node_id::build_from(xeth_address_t const & eth_address, base::enum_vaccount_addr_type account_addr_type) {
    std::error_code ec;
    auto ret = xtop_node_id::build_from(eth_address, account_addr_type, ec);
    top::error::throw_error(ec);
    return ret;
}

bool xtop_node_id::empty() const noexcept {
    return m_account_string.empty();
}

bool xtop_node_id::has_value() const noexcept {
    return !empty();
}

std::string const & xtop_node_id::value() const noexcept {
    return m_account_string;
}

xaccount_base_address_t const & xtop_node_id::base_address() const noexcept {
    return m_account_base_address;
}

uint64_t xtop_node_id::hash() const {
    if (has_value()) {
        return utl::xxh64_t::digest(m_account_string.data(), m_account_string.size());
    }

    return 0;
}

std::string const & xtop_node_id::to_string() const noexcept {
    return m_account_string;
}

void xtop_node_id::clear() {
    m_assigned_table_id.clear();
    m_account_base_address.clear();
    m_account_string.clear();
}

void
xtop_node_id::swap(xtop_node_id & other) noexcept {
    std::swap(m_account_string, other.m_account_string);
    std::swap(m_account_base_address, other.m_account_base_address);
    std::swap(m_assigned_table_id, other.m_assigned_table_id);
}

bool
xtop_node_id::operator==(xtop_node_id const & other) const noexcept {
    return m_account_string == other.m_account_string;
}

bool xtop_node_id::operator<(xtop_node_id const & other) const noexcept {
    return m_account_string < other.m_account_string;
}

bool xtop_node_id::operator>(xtop_node_id const & other) const noexcept {
    return other < *this;
}

bool
xtop_node_id::operator!=(xtop_node_id const & other) const noexcept {
    return !(*this == other);
}

bool xtop_node_id::operator>=(xtop_node_id const & other) const noexcept {
    return !(*this < other);
}

bool xtop_node_id::operator<=(xtop_node_id const & other) const noexcept {
    return !(*this > other);
}

void xtop_node_id::random() {
    auto ranbytes = random_base58_bytes(40);
    m_account_string = "T80000" + std::string{std::begin(ranbytes), std::end(ranbytes)};
    parse();
}

std::size_t xtop_node_id::length() const noexcept {
    return m_account_string.length();
}

std::size_t
xtop_node_id::size() const noexcept {
    return m_account_string.size();
}

char const *
xtop_node_id::c_str() const noexcept {
    return m_account_string.c_str();
}

base::enum_vaccount_addr_type xtop_node_id::type() const {
    return m_account_base_address.type();
}

base::enum_vaccount_addr_type xtop_node_id::type(std::error_code & ec) const {
    return m_account_base_address.type(ec);
}

xaccount_id_t const & xtop_node_id::account_id() const noexcept {
    return m_account_id;
}

xledger_id_t const & xtop_node_id::ledger_id() const noexcept {
    return m_account_base_address.ledger_id();
}

xtable_id_t const & xtop_node_id::table_id() const noexcept {
    if (!m_assigned_table_id.empty()) {
        return m_assigned_table_id;
    }

    return m_account_base_address.default_table_id();
}

bool xtop_node_id::has_assigned_table_id() const noexcept {
    return !m_assigned_table_id.empty();
}

base::xvaccount_t xtop_node_id::vaccount() const {
    return {to_string()};
}

int32_t xtop_node_id::serialize_to(base::xstream_t & stream) const {
    return do_write(stream);
}

int32_t xtop_node_id::serialize_from(base::xstream_t & stream) {
    return do_read(stream);
}

int32_t xtop_node_id::serialize_to(base::xbuffer_t & buffer) const {
    return buffer << m_account_string;
}

int32_t xtop_node_id::serialize_from(base::xbuffer_t & buffer) {
    auto const r = buffer >> m_account_string;
    parse();
    return r;
}

void xtop_node_id::parse() {
    if (m_account_string.length() < static_cast<size_t>(base::xvaccount_t::enum_vaccount_address_prefix_size) ||
        m_account_string.length() > static_cast<size_t>(static_cast<int>(base::xvaccount_t::enum_vaccount_address_max_size))) {
        top::error::throw_error(error::xerrc_t::invalid_account_address);
    }

    std::vector<std::string> parts;
    if (base::xstring_utl::split_string(m_account_string, '@', parts) > 2) {
        top::error::throw_error(error::xerrc_t::invalid_account_address);
    }

    if (m_account_base_address.empty()) {
        m_account_base_address = xaccount_base_address_t::build_from(parts[0]);
    }
    assert(m_account_base_address == xaccount_base_address_t::build_from(parts[0]));

    assert(parts.size() == 1 || parts.size() == 2);
    if (parts.size() > 1) {
        if (parts[1].length() > 2) {
            top::error::throw_error(error::xerrc_t::invalid_table_id);
        }

        if (!std::all_of(std::begin(parts[1]), std::end(parts[1]), [](char const ch) { return '0' <= ch && ch <= '9'; })) {
            top::error::throw_error(error::xerrc_t::invalid_table_id);
        }

        auto assigned_table_id = static_cast<uint16_t>(std::stoi(parts[1]));
        if (m_assigned_table_id.empty()) {
            m_assigned_table_id = xtable_id_t{assigned_table_id};
        }
        assert(m_assigned_table_id == xtable_id_t{assigned_table_id});
    } else {
        if (m_account_string.find('@') != std::string::npos) {
            top::error::throw_error(error::xerrc_t::invalid_account_address);
        }
    }

    m_account_id = xaccount_id_t{m_account_string};
}

std::int32_t
xtop_node_id::do_read(base::xstream_t & stream) {
    auto const begin_size = stream.size();
    stream >> m_account_string;
    parse();
    return begin_size - stream.size();
}

std::int32_t
xtop_node_id::do_write(base::xstream_t & stream) const {
    auto const begin_size = stream.size();
    stream << m_account_string;
    return stream.size() - begin_size;
}

std::int32_t
operator <<(top::base::xstream_t & stream, top::common::xnode_id_t const & node_id) {
    return node_id.serialize_to(stream);
}

std::int32_t
operator >>(top::base::xstream_t & stream, top::common::xnode_id_t & node_id) {
    return node_id.serialize_from(stream);
}

std::int32_t operator<<(top::base::xbuffer_t & buffer, top::common::xnode_id_t const & node_id) {
    return node_id.serialize_to(buffer);
}

std::int32_t operator>>(top::base::xbuffer_t & buffer, top::common::xnode_id_t & node_id) {
    return node_id.serialize_from(buffer);
}

NS_END2

NS_BEG1(std)

std::size_t
hash<top::common::xnode_id_t>::operator()(top::common::xnode_id_t const & id) const noexcept {
    return std::hash<std::string>{}(id.value());
}

NS_END1
