// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xregistration/xregistration_result_store.h"

#include "xbasic/xutility.h"

NS_BEG3(top, data, registration)

std::pair<xtop_registration_result_store::iterator, bool> xtop_registration_result_store::insert(value_type const & value) {
    return m_result.insert(value);
}

std::pair<xtop_registration_result_store::iterator, bool> xtop_registration_result_store::insert(value_type && value) {
    return m_result.insert(std::move(value));
}

bool xtop_registration_result_store::empty(common::xnetwork_id_t const & key) const noexcept {
    return m_result.find(key) == m_result.end();
}

bool xtop_registration_result_store::empty() const noexcept {
    return m_result.empty();
}

std::map<common::xnetwork_id_t, xregistration_chain_result_t> const & xtop_registration_result_store::results() const noexcept {
    return m_result;
}

void xtop_registration_result_store::results(std::map<common::xnetwork_id_t, xregistration_chain_result_t> && r) noexcept {
    m_result = std::move(r);
}

std::size_t xtop_registration_result_store::size() const noexcept {
    return m_result.size();
}

xtop_registration_result_store::iterator xtop_registration_result_store::begin() noexcept {
    return m_result.begin();
}

xtop_registration_result_store::const_iterator xtop_registration_result_store::begin() const noexcept {
    return m_result.begin();
}

xtop_registration_result_store::const_iterator xtop_registration_result_store::cbegin() const noexcept {
    return m_result.cbegin();
}

xtop_registration_result_store::iterator xtop_registration_result_store::end() noexcept {
    return m_result.end();
}

xtop_registration_result_store::const_iterator xtop_registration_result_store::end() const noexcept {
    return m_result.end();
}

xtop_registration_result_store::const_iterator xtop_registration_result_store::cend() const noexcept {
    return m_result.end();
}

xtop_registration_result_store::iterator xtop_registration_result_store::erase(const_iterator pos) {
    return m_result.erase(pos);
}

xtop_registration_result_store::size_type xtop_registration_result_store::erase(key_type const & key) {
    return m_result.erase(key);
}

NS_END3
