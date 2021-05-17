// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xnode.h"

#include "xnetwork/xendpoint.h"

NS_BEG2(top, common)

// template
// class xtop_node<network::xnode_endpoint_t, common::xnode_id_t>;

NS_END2

NS_BEG2(top, network)

bool
xtop_dht_node::operator==(xtop_dht_node const & other) const noexcept {
    return base_t::operator==(other);
}

bool
xtop_dht_node::operator!=(xtop_dht_node const & other) const noexcept {
    return !(*this == other);
}

void
xtop_dht_node::swap(xtop_dht_node & other) noexcept {
    base_t::swap(other);
}

bool
xtop_node::operator==(xtop_node const & other) const noexcept {
    return base_t::operator==(other);
}

bool
xtop_node::operator!=(xtop_node const & other) const noexcept {
    return !(*this == other);
}

void
xtop_node::swap(xtop_node & other) noexcept {
    base_t::swap(other);
}

NS_END2

NS_BEG1(std)

std::size_t
hash<top::network::xtop_dht_node>::operator()(top::network::xdht_node_t const & dht_node) const noexcept {
    return std::hash<top::common::xnode_id_t>{}(dht_node.id()) ^ std::hash<top::network::xnode_endpoint_t>{}(dht_node.endpoint());
}

NS_END1
