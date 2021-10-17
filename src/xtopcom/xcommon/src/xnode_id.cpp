// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xnode_id.h"

#include "xbase/xutl.h"
#include "xbasic/xbyte_buffer.h"
#include "xcommon/xerror/xerror.h"
#include "xutility/xhash.h"
#include "xvledger/xvaccount.h"

#include <cassert>
#include <vector>

NS_BEG2(top, common)

xtop_node_id::xtop_node_id(std::string const & v) : m_account_string{v} {
    parse();
}

xtop_node_id::xtop_node_id(xaccount_base_address_t base_address) : m_account_base_address{std::move(base_address)} {
    m_account_string = m_account_base_address.to_string();
    parse();
}

bool xtop_node_id::empty() const noexcept {
    return m_account_string.empty();
}

bool xtop_node_id::has_value() const noexcept {
    return !m_account_string.empty();
}

std::string const & xtop_node_id::value() const noexcept {
    return m_account_string;
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
    m_assigned_table_id = std::numeric_limits<uint16_t>::max();
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
    auto ranbytes = random_base58_bytes(33);
    m_account_string = "T-" + std::string{std::begin(ranbytes), std::end(ranbytes)};
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

uint16_t xtop_node_id::table_id(std::error_code & ec) const {
    assert(!ec);
    if (m_assigned_table_id != std::numeric_limits<uint16_t>::max()) {
        return m_assigned_table_id;
    }

    return m_account_base_address.default_table_id(ec);
}

uint16_t xtop_node_id::table_id() const {
    std::error_code ec;
    auto const r = table_id(ec);
    top::error::throw_error(ec);
    return r;
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
    if (m_account_string.length() <= static_cast<size_t>(base::xvaccount_t::enum_vaccount_address_prefix_size) ||
        m_account_string.length() >= static_cast<size_t>(static_cast<int>(base::xvaccount_t::enum_vaccount_address_max_size))) {
        assert(false);
        top::error::throw_error(error::xerrc_t::invalid_account_address);
    }

    std::vector<std::string> parts;
    if (base::xstring_utl::split_string(m_account_string, '@', parts) > 2) {
        assert(false);
        top::error::throw_error(error::xerrc_t::invalid_account_address);
    }

    m_account_base_address = xaccount_base_address_t::build_from(parts[0]);
    if (parts.size() > 1) {
        if (!std::all_of(std::begin(parts[1]), std::end(parts[1]), [](char const ch) { return '0' <= ch && ch <= '9'; })) {
            top::error::throw_error(error::xerrc_t::invalid_table_id);
        }

        auto assigned_table_id = static_cast<uint16_t>(std::stoi(parts[1]));
        if (m_assigned_table_id != std::numeric_limits<uint16_t>::max()) {
            if (m_assigned_table_id != assigned_table_id) {
                top::error::throw_error(error::xerrc_t::invalid_table_id);
            }
        } else {
            m_assigned_table_id = assigned_table_id;
        }

        assert(m_assigned_table_id == assigned_table_id);
    }
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
