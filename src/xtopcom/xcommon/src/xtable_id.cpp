// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xtable_id.h"

#include "xcommon/xaccount_address.h"
#include "xcommon/xaccount_base_address.h"
#include "xcommon/xaccount_id.h"
#include "xcommon/xerror/xerror.h"

#include <cassert>

NS_BEG2(top, common)

xtop_table_id::xtop_table_id(uint16_t const value) : m_value{ value } {
    if (m_value >= static_cast<uint16_t>(enum_vbucket_has_tables_count)) {
        top::error::throw_error(std::make_error_code(std::errc::invalid_argument));
    }
}

xtop_table_id::xtop_table_id(xaccount_id_t const & account_id) : m_value{static_cast<uint16_t>(account_id.value() % static_cast<xvid_t>(enum_vbucket_has_tables_count))} {
}

void xtop_table_id::swap(xtop_table_id & other) noexcept {
    std::swap(m_value, other.m_value);
}

void xtop_table_id::clear() noexcept {
    m_value = static_cast<uint16_t>(enum_vbucket_has_tables_count);
}

bool xtop_table_id::empty() const noexcept {
    return m_value >= static_cast<uint16_t>(enum_vbucket_has_tables_count);
}

uint16_t xtop_table_id::value(std::error_code & ec) const noexcept {
    assert(!ec);
    if (empty()) {
        ec = error::xerrc_t::invalid_table_id;
    }

    return m_value;
}

uint16_t xtop_table_id::value() const {
    std::error_code ec;
    auto r = value(ec);
#if !defined(XENABLE_TESTS)
    assert(!ec);
#endif
    top::error::throw_error(ec);
    return r;
}

bool xtop_table_id::operator==(xtop_table_id const & other) const noexcept {
    return m_value == other.m_value;
}

bool xtop_table_id::operator!=(xtop_table_id const & other) const noexcept {
    return m_value != other.m_value;
}

bool xtop_table_id::operator<(xtop_table_id const & other) const noexcept {
    if (other.empty()) {
        return false;
    }

    if (empty()) {
        return true;
    }

    return m_value < other.m_value;
}

bool xtop_table_id::operator<=(xtop_table_id const & other) const noexcept {
    if (empty()) {
        return true;
    }

    if (other.empty()) {
        return false;
    }

    return m_value <= other.m_value;
}

bool xtop_table_id::operator>(xtop_table_id const & other) const noexcept {
    if (empty()) {
        return false;
    }

    if (other.empty()) {
        return true;
    }

    return m_value > other.m_value;
}

bool xtop_table_id::operator>=(xtop_table_id const & other) const noexcept {
    if (other.empty()) {
        return true;
    }

    if (empty()) {
        return false;
    }

    return m_value >= other.m_value;
}

NS_END2

NS_BEG1(top)

template <>
std::string to_string<common::xtable_id_t>(common::xtable_id_t const & table_id) {
    if (table_id.empty()) {
        return {};
    }
    return std::to_string(table_id.value());
}

NS_END1
