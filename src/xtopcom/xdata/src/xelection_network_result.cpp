// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xutility.h"
#include "xdata/xelection/xelection_network_result.h"

#include <exception>
#include <utility>

NS_BEG3(top, data, election)

std::map<common::xnode_type_t, xelection_result_t> const &
xtop_election_network_result::results() const noexcept {
    return m_results;
}

void
xtop_election_network_result::results(std::map<common::xnode_type_t, xtop_election_result> && r) noexcept {
    m_results = std::move(r);
}

xelection_result_t const &
xtop_election_network_result::result_of(common::xnode_type_t const type) const {
    return m_results.at(type);
}

xelection_result_t &
xtop_election_network_result::result_of(common::xnode_type_t const type) {
    return m_results[type];
}

std::size_t
xtop_election_network_result::size(common::xnode_type_t const type) const noexcept {
    try {
        return result_of(type).size();
    } catch (std::exception const &) {
        return 0;
    }
}

xtop_election_network_result::iterator
xtop_election_network_result::begin() noexcept {
    return m_results.begin();
}

xtop_election_network_result::const_iterator
xtop_election_network_result::begin() const noexcept {
    return m_results.begin();
}

xtop_election_network_result::const_iterator
xtop_election_network_result::cbegin() const noexcept {
    return m_results.cbegin();
}

xtop_election_network_result::iterator
xtop_election_network_result::end() noexcept {
    return m_results.end();
}

xtop_election_network_result::const_iterator
xtop_election_network_result::end() const noexcept {
    return m_results.end();
}

xtop_election_network_result::const_iterator
xtop_election_network_result::cend() const noexcept {
    return m_results.cend();
}

xtop_election_network_result::iterator
xtop_election_network_result::erase(const_iterator pos) {
    return m_results.erase(pos);
}

xtop_election_network_result::size_type
xtop_election_network_result::erase(key_type const & key) {
    return m_results.erase(key);
}

std::pair<xtop_election_network_result::iterator, bool>
xtop_election_network_result::insert(value_type const & value) {
    return m_results.insert(value);
}

std::pair<xtop_election_network_result::iterator, bool>
xtop_election_network_result::insert(value_type && value) {
    return m_results.insert(std::move(value));
}

std::size_t
xtop_election_network_result::size() const noexcept {
    return m_results.size();
}

NS_END3
