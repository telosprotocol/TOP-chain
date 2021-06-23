// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xelection_info_bundle.h"

NS_BEG3(top, data, election)

bool 
xtop_election_info_bundle::operator==(xtop_election_info_bundle const & other) const noexcept{
    return m_node_id == other.m_node_id && m_election_info == other.m_election_info;
}

bool 
xtop_election_info_bundle::operator!=(xtop_election_info_bundle const & other) const noexcept{
    return !(*this == other);
}

common::xnode_id_t const &
xtop_election_info_bundle::node_id() const noexcept {
    return m_node_id;
}

common::xnode_id_t &
xtop_election_info_bundle::node_id() noexcept {
    return m_node_id;
}

void
xtop_election_info_bundle::node_id(common::xnode_id_t && nid) noexcept {
    m_node_id = std::move(nid);
}

void
xtop_election_info_bundle::node_id(common::xnode_id_t const & nid) {
    m_node_id = nid;
}

common::xaccount_address_t const & xtop_election_info_bundle::account_address() const noexcept {
    return m_node_id;
}

common::xaccount_address_t & xtop_election_info_bundle::account_address() noexcept {
    return m_node_id;
}

void xtop_election_info_bundle::account_address(common::xaccount_address_t && addr) noexcept {
    m_node_id = std::move(addr);
}

void xtop_election_info_bundle::account_address(common::xaccount_address_t const & addr) {
    m_node_id = addr;
}

xelection_info_t const &
xtop_election_info_bundle::election_info() const noexcept {
    return m_election_info;
}

xelection_info_t &
xtop_election_info_bundle::election_info() noexcept {
    return m_election_info;
}

void
xtop_election_info_bundle::election_info(xelection_info_t && info) noexcept {
    m_election_info = std::move(info);
}

void
xtop_election_info_bundle::election_info(xelection_info_t const & info) {
    m_election_info = info;
}

bool
xtop_election_info_bundle::empty() const noexcept {
    return m_node_id.empty();
}

void
xtop_election_info_bundle::clear() noexcept {
    m_node_id.clear();
}

NS_END3
