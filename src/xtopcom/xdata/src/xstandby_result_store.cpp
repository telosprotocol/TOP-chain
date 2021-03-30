// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xutility.h"
#include "xdata/xelection/xstandby_result_store.h"

NS_BEG3(top, data, election)

std::pair<xtop_standby_result_store::iterator, bool>
xtop_standby_result_store::insert(value_type const & value) {
    return m_results.insert(value);
}

std::pair<xtop_standby_result_store::iterator, bool>
xtop_standby_result_store::insert(value_type && value) {
    return m_results.insert(std::move(value));
}

bool
xtop_standby_result_store::empty() const noexcept {
    return m_results.empty();
}

bool
xtop_standby_result_store::empty(common::xnetwork_id_t const network_id) const noexcept {
    auto const it = m_results.find(network_id);
    if (it == std::end(m_results)) {
        return true;
    }

    auto const & result = top::get<xstandby_network_storage_result_t>(*it);
    return result.empty();
}

std::map<common::xnetwork_id_t, xstandby_network_storage_result_t> const &
xtop_standby_result_store::results() const noexcept {
    return m_results;
}

void
xtop_standby_result_store::results(std::map<common::xnetwork_id_t, xstandby_network_storage_result_t> && r) noexcept {
    m_results = std::move(r);
}

xstandby_network_storage_result_t const &
xtop_standby_result_store::result_of(common::xnetwork_id_t const network_id) const {
    return m_results.at(network_id);
}

xstandby_network_storage_result_t &
xtop_standby_result_store::result_of(common::xnetwork_id_t const network_id) {
    return m_results[network_id];
}

std::size_t
xtop_standby_result_store::size(common::xnetwork_id_t const network_id) const noexcept {
    if (empty(network_id)) {
        return 0;
    }

    return result_of(network_id).size();
}

std::size_t
xtop_standby_result_store::size() const noexcept {
    return m_results.size();
}

xtop_standby_result_store::iterator
xtop_standby_result_store::begin() noexcept {
    return m_results.begin();
}

xtop_standby_result_store::const_iterator
xtop_standby_result_store::begin() const noexcept {
    return m_results.begin();
}

xtop_standby_result_store::const_iterator
xtop_standby_result_store::cbegin() const noexcept {
    return m_results.cbegin();
}

xtop_standby_result_store::iterator
xtop_standby_result_store::end() noexcept {
    return m_results.end();
}

xtop_standby_result_store::const_iterator
xtop_standby_result_store::end() const noexcept {
    return m_results.end();
}

xtop_standby_result_store::const_iterator
xtop_standby_result_store::cend() const noexcept {
    return m_results.end();
}

xtop_standby_result_store::iterator
xtop_standby_result_store::erase(const_iterator pos) {
    return m_results.erase(pos);
}

xtop_standby_result_store::size_type
xtop_standby_result_store::erase(key_type const & key) {
    return m_results.erase(key);
}

NS_END3
