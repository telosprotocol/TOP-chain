// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xelection_result.h"

NS_BEG3(top, data, election)

std::pair<xtop_election_result::iterator, bool>
xtop_election_result::insert(value_type const & value) {
    return m_cluster_results.insert(value);
}

std::pair<xtop_election_result::iterator, bool>
xtop_election_result::insert(value_type && value) {
    return m_cluster_results.insert(std::move(value));
}

bool
xtop_election_result::empty() const noexcept {
    return m_cluster_results.empty();
}

std::map<common::xcluster_id_t, xelection_cluster_result_t> const &
xtop_election_result::results() const noexcept {
    return m_cluster_results;
}

void
xtop_election_result::results(std::map<common::xcluster_id_t, xelection_cluster_result_t> && r) noexcept {
    m_cluster_results = std::move(r);
}

xelection_cluster_result_t const &
xtop_election_result::result_of(common::xcluster_id_t const & cid) const {
    return m_cluster_results.at(cid);
}

xelection_cluster_result_t &
xtop_election_result::result_of(common::xcluster_id_t const & cid) {
    return m_cluster_results[cid];
}

std::size_t
xtop_election_result::size() const noexcept {
    return m_cluster_results.size();
}

xtop_election_result::iterator
xtop_election_result::begin() noexcept {
    return m_cluster_results.begin();
}

xtop_election_result::const_iterator
xtop_election_result::begin() const noexcept {
    return m_cluster_results.begin();
}

xtop_election_result::const_iterator
xtop_election_result::cbegin() const noexcept {
    return m_cluster_results.cbegin();
}

xtop_election_result::iterator
xtop_election_result::end() noexcept {
    return m_cluster_results.end();
}

xtop_election_result::const_iterator
xtop_election_result::end() const noexcept {
    return m_cluster_results.end();
}

xtop_election_result::const_iterator
xtop_election_result::cend() const noexcept {
    return m_cluster_results.cend();
}

xtop_election_result::iterator
xtop_election_result::erase(const_iterator pos) {
    return m_cluster_results.erase(pos);
}

xtop_election_result::size_type
xtop_election_result::erase(key_type const & key) {
    return m_cluster_results.erase(key);
}

NS_END3
