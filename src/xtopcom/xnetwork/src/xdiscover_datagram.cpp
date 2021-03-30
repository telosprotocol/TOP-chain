// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xp2p/xdiscover_datagram.h"

#include <cassert>

NS_BEG3(top, network, p2p)

xtop_ping_node::xtop_ping_node(xdht_node_t target) noexcept
    : m_target{ std::move(target) }
{
}

xdht_node_t const &
xtop_ping_node::target_node() const noexcept {
    return m_target;
}

bool
xtop_ping_node::empty() const noexcept {
    return m_target.empty();
}

xtop_pong::xtop_pong(xnode_endpoint_t ping_ep,
                     xhash256_t phash) noexcept
    : m_ping_endpoint{ std::move(ping_ep) }, m_ping_hash{ std::move(phash) }
{
}

xendpoint_t const &
xtop_pong::ping_endpoint() const noexcept {
    return m_ping_endpoint;
}

xhash256_t const &
xtop_pong::ping_hash() const noexcept {
    return m_ping_hash;
}

bool
xtop_pong::empty() const noexcept {
    return m_ping_endpoint.empty();
}

xtop_find_node::xtop_find_node(common::xnode_id_t target)
    : m_target_id{ std::move(target) } {
}

common::xnode_id_t const &
xtop_find_node::target_id() const noexcept {
    return m_target_id;
}

bool
xtop_find_node::empty() const noexcept {
    return m_target_id.empty();
}

xtop_neighbors::xtop_neighbors(std::unordered_set<xdht_node_t> nbs) noexcept
    : m_neighbors{ std::move(nbs) } {
}

std::unordered_set<xdht_node_t> const &
xtop_neighbors::neighbors() const noexcept {
    return m_neighbors;
}

bool
xtop_neighbors::empty() const noexcept {
    return m_neighbors.empty();
}

void
xtop_neighbors::add_neighbor(xdht_node_t neighbor_node) {
    auto const it = m_neighbors.find(neighbor_node);
    if (it == std::end(m_neighbors)) {
        m_neighbors.insert({ std::move(neighbor_node) });
    } else {
        assert(false);
        // todo log error
        // todo throw exception
        throw std::invalid_argument{ "neighbor exists" };
    }
}

NS_END3
