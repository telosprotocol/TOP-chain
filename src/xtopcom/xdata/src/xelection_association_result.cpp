// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xelection_association_result.h"

NS_BEG3(top, data, election)

xtop_election_association_result::iterator
xtop_election_association_result::begin() noexcept {
    return m_assocation_result.begin();
}

xtop_election_association_result::const_iterator
xtop_election_association_result::begin() const noexcept {
    return m_assocation_result.begin();
}

xtop_election_association_result::const_iterator
xtop_election_association_result::cbegin() const noexcept {
    return m_assocation_result.cbegin();
}

xtop_election_association_result::const_iterator
xtop_election_association_result::cend() const noexcept {
    return m_assocation_result.cend();
}

xtop_election_association_result::iterator
xtop_election_association_result::end() noexcept {
    return m_assocation_result.end();
}

xtop_election_association_result::const_iterator
xtop_election_association_result::end() const noexcept {
    return m_assocation_result.end();
}

common::xelection_round_t &
xtop_election_association_result::cluster_version() noexcept {
    return m_cluster_version;
}

common::xelection_round_t const &
xtop_election_association_result::cluster_version() const noexcept {
    return m_cluster_version;
}

void
xtop_election_association_result::cluster_version(common::xelection_round_t && cluster_ver) noexcept {
    m_cluster_version = std::move(cluster_ver);
}

std::pair<xtop_election_association_result::iterator, bool>
xtop_election_association_result::insert(value_type const & value) {
    return m_assocation_result.insert(value);
}

std::pair<xtop_election_association_result::iterator, bool>
xtop_election_association_result::insert(value_type && value) {
    return m_assocation_result.insert(std::move(value));
}

xtop_election_association_result::iterator
xtop_election_association_result::erase(const_iterator pos) {
    return m_assocation_result.erase(pos);
}

std::size_t
xtop_election_association_result::erase(key_type const & key) {
    return m_assocation_result.erase(key);
}

common::xgroup_id_t &
xtop_election_association_result::result_of(common::xgroup_id_t const & gid) noexcept {
    return m_assocation_result[gid];
}

common::xgroup_id_t const &
xtop_election_association_result::result_of(common::xgroup_id_t const & gid) const noexcept {
    return m_assocation_result.at(gid);
}

std::map<common::xgroup_id_t, common::xgroup_id_t> const &
xtop_election_association_result::results() const noexcept {
    return m_assocation_result;
}

void
xtop_election_association_result::results(std::map<common::xgroup_id_t, common::xgroup_id_t> && r) noexcept {
    m_assocation_result = std::move(r);
}

void
xtop_election_association_result::increase_cluster_version() {
    if (m_cluster_version.empty()) {
        m_cluster_version = common::xelection_round_t{ 0 };
    } else {
        m_cluster_version.increase();
    }
}

std::size_t
xtop_election_association_result::size() const noexcept {
    return m_assocation_result.size();
}

NS_END3
