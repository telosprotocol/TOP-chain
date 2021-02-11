// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xrunnable.h"
#include "xnetwork/xnode.h"
#include "xnetwork/xp2p/xnode_entry.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>

NS_BEG3(top, network, p2p)

class xtop_routing_table_face : public xbasic_runnable_t<xtop_routing_table_face>
{
public:
    virtual
    void
    ping(xdht_node_t const & remote_dht_node) = 0;

    virtual
    xnode_endpoint_t
    endpoint(common::xnode_id_t const & node_id) const = 0;

    virtual
    std::vector<std::shared_ptr<xnode_entry_t>>
    nearest_node_entries(common::xnode_id_t const & target) const = 0;

    virtual
    std::shared_ptr<xnode_entry_t>
    node_entry(common::xnode_id_t const & node_id) const = 0;

    virtual
    common::xnode_id_t const &
    host_node_id() const noexcept = 0;

    virtual
    xdht_node_t
    host_dht_node() const = 0;

    virtual
    std::unordered_map<common::xnode_id_t, std::shared_ptr<xnode_entry_t>>
    all_known_nodes() const = 0;

    virtual
    std::size_t
    size_upper_limit() const noexcept = 0;
};

using xrouting_table_face_t = xtop_routing_table_face;

NS_END3
