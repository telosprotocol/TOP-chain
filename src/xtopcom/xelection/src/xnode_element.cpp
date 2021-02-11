// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelection/xcache/xgroup_element.h"
#include "xelection/xcache/xnode_element.h"

NS_BEG3(top, election, cache)

xtop_node_element::xtop_node_element(common::xnode_id_t const & node_id,
                                     common::xslot_id_t const & slot_id,
                                     data::election::xelection_info_t const & election_info,
                                     std::shared_ptr<xgroup_element_t> const & group_element)
    : xbase_t{
        group_element->version(),
        group_element->address().network_id(),
        group_element->address().zone_id(),
        group_element->address().cluster_id(),
        group_element->address().group_id(),
        node_id,
        slot_id,
        group_element->sharding_size(),
        group_element->associated_blk_height()
    }
    , m_group_element{ group_element }
    , m_election_info{ election_info }
{
}

std::shared_ptr<xgroup_element_t>
xtop_node_element::group_element() const noexcept {
    return m_group_element.lock();
}

data::election::xelection_info_t const &
xtop_node_element::election_info() const noexcept {
    return m_election_info;
}

xtop_node_element::hash_result_type
xtop_node_element::hash() const {
    return node_id().hash();
}

common::xversion_t const &
xtop_node_element::joined_version() const noexcept {
    return m_election_info.joined_version;
}

std::uint64_t
xtop_node_element::staking() const noexcept {
    return m_election_info.stake;
}

bool
xtop_node_element::operator==(xtop_node_element const & other) const noexcept {
    if (!xbase_t::operator==(other)) {
        return false;
    }

    return m_election_info == other.m_election_info;
}

bool
xtop_node_element::operator!=(xtop_node_element const & other) const noexcept {
    return !(*this == other);
}

void
xtop_node_element::swap(xtop_node_element & other) noexcept {
    m_group_element.swap(other.m_group_element);
    m_election_info.swap(other.m_election_info);
    xbase_t::swap(other);
}

NS_END3
