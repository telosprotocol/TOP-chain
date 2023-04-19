// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xnode_id.h"

#include "xbase/xutl.h"
#include "xbasic/xbyte_buffer.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xeth_address.h"
#include "xcommon/xtable_base_address.h"
#include "xmetrics/xmetrics.h"
#include "xutility/xhash.h"
#include "xvledger/xvaccount.h"

#include <cassert>
#include <vector>

NS_BEG2(top, common)

metrics_xtop_node_id::metrics_xtop_node_id() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_account_address, 1);
}
metrics_xtop_node_id::metrics_xtop_node_id(metrics_xtop_node_id const &) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_account_address, 1);
}
metrics_xtop_node_id::metrics_xtop_node_id(metrics_xtop_node_id &&) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_account_address, 1);
}
metrics_xtop_node_id::~metrics_xtop_node_id() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_account_address, -1);
}

xtop_node_id::xtop_node_id(std::string const & value) {
    parse(value);
}

xtop_node_id::xtop_node_id(xaccount_base_address_t base_address) : m_account_base_address{std::move(base_address)} {
    if (!empty()) {
        parse(m_account_base_address.to_string());
    }
}

xtop_node_id::xtop_node_id(xaccount_base_address_t base_address, uint16_t const table_id_value) : xtop_node_id{std::move(base_address), xtable_id_t{table_id_value}} {
}

xtop_node_id::xtop_node_id(xaccount_base_address_t base_address, xtable_id_t const table_id)
  : m_account_base_address{std::move(base_address)}, m_assigned_table_id{table_id} {
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

xtop_node_id xtop_node_id::build_from(xaccount_base_address_t const & account_base_address) {
    return build_from(account_base_address.to_string());
}

xtop_node_id xtop_node_id::build_from(xaccount_base_address_t const & account_base_address, xtable_id_t table_id) {
    return xtop_node_id{account_base_address, table_id};
}

bool xtop_node_id::empty() const noexcept {
    return m_account_base_address.empty();
}

bool xtop_node_id::has_value() const noexcept {
    return !empty();
}

xaccount_base_address_t const & xtop_node_id::base_address() const noexcept {
    return m_account_base_address;
}

uint64_t xtop_node_id::hash() const {
    if (has_value()) {
        auto const & account_string = to_string();
        return utl::xxh64_t::digest(account_string.data(), account_string.size());
    }

    return 0;
}

std::string xtop_node_id::to_string() const {
    if (m_assigned_table_id.empty()) {
        return m_account_base_address.to_string();
    }

    return m_account_base_address.to_string() + '@' + top::to_string(m_assigned_table_id);
}

void xtop_node_id::clear() {
    m_assigned_table_id.clear();
    m_account_base_address.clear();
}

void
xtop_node_id::swap(xtop_node_id & other) noexcept {
    // std::swap(m_account_string, other.m_account_string);
    std::swap(m_account_base_address, other.m_account_base_address);
    std::swap(m_assigned_table_id, other.m_assigned_table_id);
}

bool
xtop_node_id::operator==(xtop_node_id const & other) const noexcept {
    return m_account_base_address == other.m_account_base_address && m_assigned_table_id == other.m_assigned_table_id;
}

bool xtop_node_id::operator<(xtop_node_id const & other) const noexcept {
    if (m_account_base_address != other.m_account_base_address) {
        return m_account_base_address < other.m_account_base_address;
    }

    return m_assigned_table_id < other.m_assigned_table_id;
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

//void xtop_node_id::random() {
//    auto ranbytes = random_base58_bytes(40);
//    auto account_string = "T80000" + std::string{std::begin(ranbytes), std::end(ranbytes)};
//    parse(account_string);
//}

std::size_t xtop_node_id::length() const noexcept {
    if (m_assigned_table_id.empty()) {
        return m_account_base_address.size();
    }

    return m_account_base_address.size() + 1 + top::to_string(m_assigned_table_id).size();
}

std::size_t
xtop_node_id::size() const noexcept {
    return length();
}

//char const *
//xtop_node_id::c_str() const noexcept {
//    return m_account_string.c_str();
//}

base::enum_vaccount_addr_type xtop_node_id::type() const {
    return m_account_base_address.type();
}

base::enum_vaccount_addr_type xtop_node_id::type(std::error_code & ec) const {
    return m_account_base_address.type(ec);
}

xaccount_id_t const & xtop_node_id::account_id() const noexcept {
    return m_account_id;
}

xledger_id_t xtop_node_id::ledger_id() const noexcept {
    return m_account_base_address.ledger_id();
}

xzone_id_t xtop_node_id::zone_id() const noexcept {
    return ledger_id().zone_id();
}

xtable_id_t xtop_node_id::table_id() const noexcept {
    if (!m_assigned_table_id.empty()) {
        return m_assigned_table_id;
    }

    return m_account_base_address.default_table_id();
}

bool xtop_node_id::has_assigned_table_id() const noexcept {
    return !m_assigned_table_id.empty();
}

base::xvaccount_t xtop_node_id::vaccount() const {
    return base::xvaccount_t{to_string()};
}

xtable_address_t xtop_node_id::table_address() const {
    std::error_code ec;
    auto const r = table_address(ec);
    top::error::throw_error(ec);
    return r;
}

xtable_address_t xtop_node_id::table_address(std::error_code & ec) const {
    switch (type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case base::enum_vaccount_addr_type_secp256k1_user_account:
    case base::enum_vaccount_addr_type_secp256k1_eth_user_account:
    case base::enum_vaccount_addr_type_secp256k1_evm_user_account:
    case base::enum_vaccount_addr_type_native_contract: {
        auto const table_base_address = xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, ledger_id().zone_id(), ec);
        return !ec ? xtable_address_t::build_from(table_base_address, table_id(), ec) : xtable_address_t{};
    }

    case base::enum_vaccount_addr_type_block_contract:
    case base::enum_vaccount_addr_type_relay_block: {
        auto const table_base_address = xtable_base_address_t::build_from(type(), ledger_id().zone_id(), ec);
        return !ec ? xtable_address_t::build_from(table_base_address, table_id(), ec) : xtable_address_t{};
    }

    default: {
        ec = error::xerrc_t::invalid_account_type;
        return xtable_address_t{};
    }
    }
}


int32_t xtop_node_id::serialize_to(base::xstream_t & stream) const {
    return do_write(stream);
}

int32_t xtop_node_id::serialize_from(base::xstream_t & stream) {
    return do_read(stream);
}

int32_t xtop_node_id::serialize_to(base::xbuffer_t & buffer) const {
    return buffer << to_string();
}

int32_t xtop_node_id::serialize_from(base::xbuffer_t & buffer) {
    std::string account_string;
    auto const r = buffer >> account_string;
    parse(account_string);
    return r;
}

void xtop_node_id::parse(std::string const & account_string) {
    if (account_string.empty()) {
        xwarn("xaccount_address_t::parse empty string");
        return;
    }

    if (account_string.length() < static_cast<size_t>(base::xvaccount_t::enum_vaccount_address_prefix_size) ||
        account_string.length() > static_cast<size_t>(static_cast<int>(base::xvaccount_t::enum_vaccount_address_max_size))) {
        top::error::throw_error(error::xerrc_t::invalid_account_address);
    }

    std::vector<std::string> parts;
    if (base::xstring_utl::split_string(account_string, '@', parts) > 2) {
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

        auto const assigned_table_id = static_cast<uint16_t>(std::stoi(parts[1]));
        if (m_assigned_table_id.empty()) {
            m_assigned_table_id = xtable_id_t{assigned_table_id};
        }
        assert(m_assigned_table_id == xtable_id_t{assigned_table_id});
    } else {
        if (account_string.find('@') != std::string::npos) {
            top::error::throw_error(error::xerrc_t::invalid_account_address);
        }
    }

    m_account_id = xaccount_id_t{account_string};
}

std::int32_t
xtop_node_id::do_read(base::xstream_t & stream) {
    auto const begin_size = stream.size();
    std::string account_string;
    stream >> account_string;
    parse(account_string);
    return begin_size - stream.size();
}

std::int32_t
xtop_node_id::do_write(base::xstream_t & stream) const {
    auto const begin_size = stream.size();
    stream << to_string();
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
    return std::hash<std::string>{}(id.to_string());
}

NS_END1

NS_BEG1(top)

template <>
xbytes_t to_bytes<common::xnode_id_t>(common::xnode_id_t const & input) {
    auto const & string = input.to_string();
    return {string.begin(), string.end()};
}

template <>
std::string to_string<common::xnode_id_t>(common::xnode_id_t const & input) {
    return input.to_string();
}

NS_END1
