// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xnode.hpp"
#include "xcommon/xnode_id.h"
#include "xnetwork/xendpoint.h"

#include <functional>

NS_BEG2(top, network)

class xtop_dht_node : common::xnode_t<xendpoint_t, common::xnode_id_t>
{
private:
    using base_t = common::xnode_t<xendpoint_t, common::xnode_id_t>;

public:
    using base_t::base_t;

    using endpoint_type = base_t::endpoint_type;
    using id_type = base_t::id_type;

    using base_t::empty;
    using base_t::endpoint;
    using base_t::id;

    bool
    operator==(xtop_dht_node const & other) const noexcept;

    bool
    operator!=(xtop_dht_node const & other) const noexcept;

    void
    swap(xtop_dht_node & other) noexcept;
};
using xdht_node_t = xtop_dht_node;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::network::xdht_node_t> final
{
    std::size_t
    operator()(top::network::xdht_node_t const & dht_node) const noexcept;
};

NS_END1
