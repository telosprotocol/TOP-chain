// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xelection_cluster_result.h"

NS_BEG3(top, data, election)

xtop_election_cluster_result::iterator
xtop_election_cluster_result::begin() noexcept {
    return m_group_results.begin();
}

xtop_election_cluster_result::const_iterator
xtop_election_cluster_result::begin() const noexcept {
    return m_group_results.begin();
}

xtop_election_cluster_result::const_iterator
xtop_election_cluster_result::cbegin() const noexcept {
    return m_group_results.cbegin();
}

xtop_election_cluster_result::const_iterator
xtop_election_cluster_result::cend() const noexcept {
    return m_group_results.cend();
}

bool
xtop_election_cluster_result::empty() const noexcept {
    return m_group_results.empty();
}

xtop_election_cluster_result::iterator
xtop_election_cluster_result::end() noexcept {
    return m_group_results.end();
}

xtop_election_cluster_result::const_iterator
xtop_election_cluster_result::end() const noexcept {
    return m_group_results.end();
}

std::map<common::xgroup_id_t, xelection_group_result_t> const &
xtop_election_cluster_result::results() const noexcept {
    return m_group_results;
}

void
xtop_election_cluster_result::results(std::map<common::xgroup_id_t, xelection_group_result_t> && r) noexcept {
    m_group_results = std::move(r);
}

std::size_t
xtop_election_cluster_result::size() const noexcept {
    return m_group_results.size();
}

xelection_group_result_t &
xtop_election_cluster_result::result_of(common::xgroup_id_t const & gid) {
    return m_group_results[gid];
}

xelection_group_result_t const &
xtop_election_cluster_result::result_of(common::xgroup_id_t const & gid) const {
    return m_group_results.at(gid);
}

std::pair<xtop_election_cluster_result::iterator, bool>
xtop_election_cluster_result::insert(value_type && value) {
    return m_group_results.insert(std::move(value));
}

std::pair<xtop_election_cluster_result::iterator, bool>
xtop_election_cluster_result::insert(value_type const & value) {
    return m_group_results.insert(value);
}

xtop_election_cluster_result::size_type
xtop_election_cluster_result::erase(key_type const & key) {
    return m_group_results.erase(key);
}

xtop_election_cluster_result::iterator
xtop_election_cluster_result::erase(const_iterator pos) {
    return m_group_results.erase(pos);
}

NS_END3
