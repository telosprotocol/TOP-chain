// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <unordered_map>
#include <unordered_set>

#include "xbasic/xhash.hpp"
#include "xnetwork/xnode.h"
#include "xbase/xns_macro.h"
#include "xcommon/xnode_id.h"
#include "xnetwork/xnode_endpoint.h"

NS_BEG3(top, network, p2p)

class xtop_ping_node final
{
private:
    xdht_node_t m_target{};
    //std::uint16_t m_app_port{};

public:
    xtop_ping_node()                                   = default;
    xtop_ping_node(xtop_ping_node const &)             = default;
    xtop_ping_node & operator=(xtop_ping_node const &) = default;
    xtop_ping_node(xtop_ping_node &&)                  = default;
    xtop_ping_node & operator=(xtop_ping_node &&)      = default;
    ~xtop_ping_node()                                  = default;

    explicit
    xtop_ping_node(xdht_node_t target) noexcept;

    xdht_node_t const &
    target_node() const noexcept;

    bool
    empty() const noexcept;
};
using xping_node_t = xtop_ping_node;

class xtop_pong final
{
    xnode_endpoint_t m_ping_endpoint{};
    xhash256_t m_ping_hash{};

public:
    xtop_pong()                              = default;
    xtop_pong(xtop_pong const &)             = default;
    xtop_pong & operator=(xtop_pong const &) = default;
    xtop_pong(xtop_pong &&)                  = default;
    xtop_pong & operator=(xtop_pong &&)      = default;
    ~xtop_pong()                             = default;

    xtop_pong(xnode_endpoint_t ping_ep, xhash256_t phash) noexcept;

    xnode_endpoint_t const &
    ping_endpoint() const noexcept;

    xhash256_t const &
    ping_hash() const noexcept;

    bool
    empty() const noexcept;
};
using xpong_t = xtop_pong;

class xtop_find_node final
{
private:
    common::xnode_id_t m_target_id{};

public:
    xtop_find_node()                                   = default;
    xtop_find_node(xtop_find_node const &)             = default;
    xtop_find_node & operator=(xtop_find_node const &) = default;
    xtop_find_node(xtop_find_node &&)                  = default;
    xtop_find_node & operator=(xtop_find_node &&)      = default;
    ~xtop_find_node()                                  = default;

    explicit
    xtop_find_node(common::xnode_id_t target);

    common::xnode_id_t const &
    target_id() const noexcept;

    bool
    empty() const noexcept;
};
using xfind_node_t = xtop_find_node;

class xtop_neighbors final
{
    std::unordered_set<xdht_node_t> m_neighbors{};

public:
    xtop_neighbors()                                   = default;
    xtop_neighbors(xtop_neighbors const &)             = default;
    xtop_neighbors & operator=(xtop_neighbors const &) = default;
    xtop_neighbors(xtop_neighbors &&)                  = default;
    xtop_neighbors & operator=(xtop_neighbors &&)      = default;
    ~xtop_neighbors()                                  = default;

    explicit
    xtop_neighbors(std::unordered_set<xdht_node_t> nbs) noexcept;

    std::unordered_set<xdht_node_t> const &
    neighbors() const noexcept;

    void
    add_neighbor(xdht_node_t neighbor_node);

    bool
    empty() const noexcept;
};
using xneighbors_t = xtop_neighbors;

NS_END3
