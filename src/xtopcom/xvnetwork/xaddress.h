// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcommon/xnode_type.h"

#include <cassert>
#include <ostream>

NS_BEG2(top, common)

// extern
// template
// class xtop_simple_address<vnetwork::xaccount_address_tag, common::xnode_id_t>;

NS_END2

NS_BEG2(top, vnetwork)

/**
 * @brief Account address
 */
using xaccount_address_t = common::xaccount_address_t;

using xcluster_address_t = common::xcluster_address_t;
using xgroup_address_t = common::xgroup_address_t;

using xvnode_address_t = common::xnode_address_t;
using xnode_address_t = common::xnode_address_t;

template <
    common::xnode_type_t VNodeTypeV,
    typename std::enable_if<VNodeTypeV == common::xnode_type_t::zone || VNodeTypeV == common::xnode_type_t::cluster || VNodeTypeV == common::xnode_type_t::group>::type * = nullptr>
common::xnode_address_t address_cast(common::xnode_address_t const & address) {
    assert(!address.empty());

    switch (VNodeTypeV) {
    case common::xnode_type_t::zone: {
        return common::xnode_address_t{common::xsharding_address_t{address.cluster_address().network_id(), address.cluster_address().zone_id()}};
    }

    case common::xnode_type_t::cluster: {
        return common::xnode_address_t{
            common::xsharding_address_t{address.cluster_address().network_id(), address.cluster_address().zone_id(), address.cluster_address().cluster_id()}};
    }

    case common::xnode_type_t::group: {
        return common::xnode_address_t{
            common::xsharding_address_t{
                address.cluster_address().network_id(), address.cluster_address().zone_id(), address.cluster_address().cluster_id(), address.cluster_address().group_id()},
            address.election_round(),
            address.group_size(),
            address.associated_blk_height()};
    }

    default:
        assert(false);
        return common::xnode_address_t{};
    }
}

NS_END2

std::ostream & operator<<(std::ostream & o, top::vnetwork::xaccount_address_t const & account_address);

std::ostream & operator<<(std::ostream & o, top::vnetwork::xcluster_address_t const & cluster_address);

std::ostream & operator<<(std::ostream & o, top::vnetwork::xvnode_address_t const & vnode_address);
