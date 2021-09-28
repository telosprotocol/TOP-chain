// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xparachain/xparachain_result.h"

#include "xbasic/xutility.h"

NS_BEG3(top, data, parachain)

std::pair<xtop_parachain_result::iterator, bool> xtop_parachain_result::insert(value_type const & value) {
    return m_parachain_result.insert(value);
}

std::pair<xtop_parachain_result::iterator, bool> xtop_parachain_result::insert(value_type && value) {
    return m_parachain_result.insert(std::move(value));
}

bool xtop_parachain_result::empty(uint32_t const & key) const noexcept {
    return m_parachain_result.find(key) == m_parachain_result.end();
}

bool xtop_parachain_result::empty() const noexcept {
    return m_parachain_result.empty();
}

std::map<uint32_t, xparachain_chain_info_t> const & xtop_parachain_result::results() const noexcept {
    return m_parachain_result;
}

void xtop_parachain_result::results(std::map<uint32_t, xparachain_chain_info_t> && r) noexcept {
    m_parachain_result = std::move(r);
}

std::size_t xtop_parachain_result::size() const noexcept {
    return m_parachain_result.size();
}

xtop_parachain_result::iterator xtop_parachain_result::begin() noexcept {
    return m_parachain_result.begin();
}

xtop_parachain_result::const_iterator xtop_parachain_result::begin() const noexcept {
    return m_parachain_result.begin();
}

xtop_parachain_result::const_iterator xtop_parachain_result::cbegin() const noexcept {
    return m_parachain_result.cbegin();
}

xtop_parachain_result::iterator xtop_parachain_result::end() noexcept {
    return m_parachain_result.end();
}

xtop_parachain_result::const_iterator xtop_parachain_result::end() const noexcept {
    return m_parachain_result.end();
}

xtop_parachain_result::const_iterator xtop_parachain_result::cend() const noexcept {
    return m_parachain_result.end();
}

xtop_parachain_result::iterator xtop_parachain_result::erase(const_iterator pos) {
    return m_parachain_result.erase(pos);
}

xtop_parachain_result::size_type xtop_parachain_result::erase(key_type const & key) {
    return m_parachain_result.erase(key);
}

NS_END3
