// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xregistration/xregistration_chain_result.h"

#include "xbasic/xutility.h"

NS_BEG3(top, data, registration)

std::pair<xtop_registration_chain_result::iterator, bool> xtop_registration_chain_result::insert(value_type const & value) {
    return m_result.insert(value);
}

std::pair<xtop_registration_chain_result::iterator, bool> xtop_registration_chain_result::insert(value_type && value) {
    return m_result.insert(std::move(value));
}

bool xtop_registration_chain_result::empty(common::xnode_id_t const & key) const noexcept {
    return m_result.find(key) == m_result.end();
}

bool xtop_registration_chain_result::empty() const noexcept {
    return m_result.empty();
}

std::map<common::xnode_id_t, xregistration_node_info> const & xtop_registration_chain_result::results() const noexcept {
    return m_result;
}

void xtop_registration_chain_result::results(std::map<common::xnode_id_t, xregistration_node_info> && r) noexcept {
    m_result = std::move(r);
}

std::size_t xtop_registration_chain_result::size() const noexcept {
    return m_result.size();
}

xtop_registration_chain_result::iterator xtop_registration_chain_result::begin() noexcept {
    return m_result.begin();
}

xtop_registration_chain_result::const_iterator xtop_registration_chain_result::begin() const noexcept {
    return m_result.begin();
}

xtop_registration_chain_result::const_iterator xtop_registration_chain_result::cbegin() const noexcept {
    return m_result.cbegin();
}

xtop_registration_chain_result::iterator xtop_registration_chain_result::end() noexcept {
    return m_result.end();
}

xtop_registration_chain_result::const_iterator xtop_registration_chain_result::end() const noexcept {
    return m_result.end();
}

xtop_registration_chain_result::const_iterator xtop_registration_chain_result::cend() const noexcept {
    return m_result.end();
}

xtop_registration_chain_result::iterator xtop_registration_chain_result::erase(const_iterator pos) {
    return m_result.erase(pos);
}

xtop_registration_chain_result::size_type xtop_registration_chain_result::erase(key_type const & key) {
    return m_result.erase(key);
}

NS_END3
