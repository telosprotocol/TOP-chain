// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xrunnable.h"
#include "xnetwork/xnode.h"
#include "xnetwork/xp2p/xnode_entry.hpp"

#include <future>
#include <memory>
#include <vector>

NS_BEG3(top, network, p2p)

// using xbootstrap_result_future_t = std::future<bool>;

class xtop_dht_host_face : public xrunnable_t<xtop_dht_host_face>
{
public:
    xtop_dht_host_face()                                       = default;
    xtop_dht_host_face(xtop_dht_host_face const &)             = delete;
    xtop_dht_host_face & operator=(xtop_dht_host_face const &) = delete;
    xtop_dht_host_face(xtop_dht_host_face &&)                  = default;
    xtop_dht_host_face & operator=(xtop_dht_host_face &&)      = default;
    virtual ~xtop_dht_host_face()                              = default;

    virtual
    void
    // std::vector<xbootstrap_result_future_t>
    bootstrap(std::vector<xdht_node_t> const & seeds) const = 0;

    virtual
    common::xnode_id_t const &
    host_node_id() const noexcept = 0;

    virtual
    xdht_node_t
    host_dht_node() const = 0;

    virtual
    xnode_endpoint_t
    endpoint(common::xnode_id_t const & node_id) const = 0;

    virtual
    std::vector<std::shared_ptr<xnode_entry_t>>
    nearest_node_entries(common::xnode_id_t const & target) const = 0;

    virtual
    std::vector<std::shared_ptr<xnode_entry_t>>
    all_node_entries() const = 0;

    virtual
    std::size_t
    neighbor_size_upper_limit() const noexcept = 0;

    virtual
    std::shared_ptr<xnode_entry_t>
    node_entry(common::xnode_id_t const & target) const = 0;

    virtual
    xnode_endpoint_t
    local_endpoint() const = 0;
};

using xdht_host_face_t = xtop_dht_host_face;

NS_END3
