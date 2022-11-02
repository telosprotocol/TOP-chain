// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xlegacy_account_address.h"

#include "xbasic/xbyte_buffer.h"
#include "xutility/xhash.h"

#include <cassert>

NS_BEG2(top, common)

xtop_legacy_account_address::xtop_legacy_account_address(char const * v) : id_base_t{v} {
}

xtop_legacy_account_address::xtop_legacy_account_address(std::string v) : id_base_t{std::move(v)} {
}

xtop_legacy_account_address::xtop_legacy_account_address(xaccount_address_t const & address) : id_base_t{address.to_string()} {
}

void xtop_legacy_account_address::swap(xtop_legacy_account_address & other) noexcept {
    id_base_t::swap(other);
}

bool xtop_legacy_account_address::operator<(xtop_legacy_account_address const & other) const noexcept {
    return id_base_t::operator<(other);
}

bool xtop_legacy_account_address::operator==(xtop_legacy_account_address const & other) const noexcept {
    return id_base_t::operator==(other);
}

bool xtop_legacy_account_address::operator!=(xtop_legacy_account_address const & other) const noexcept {
    return id_base_t::operator!=(other);
}

std::size_t xtop_legacy_account_address::length() const noexcept {
    return this->m_id.length();
}

std::size_t xtop_legacy_account_address::size() const noexcept {
    return this->m_id.size();
}

char const * xtop_legacy_account_address::c_str() const noexcept {
    return this->m_id.c_str();
}

base::enum_vaccount_addr_type xtop_legacy_account_address::type() const noexcept {
    if (m_type == base::enum_vaccount_addr_type::enum_vaccount_addr_type_invalid) {
        m_type = base::xvaccount_t::get_addrtype_from_account(m_id);
    }
    assert(m_type != base::enum_vaccount_addr_type::enum_vaccount_addr_type_invalid);
    return m_type;
}

NS_END2

NS_BEG1(std)

std::size_t hash<top::common::xlegacy_account_address_t>::operator()(top::common::xlegacy_account_address_t const & id) const noexcept {
    return std::hash<top::common::xstring_id_t<top::common::xlegacy_account_address_t>>{}(id);
}

NS_END1
