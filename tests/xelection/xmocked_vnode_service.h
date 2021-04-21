// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define private protected

#include "xelection/xvnode_house.h"
#include "xbase/xobject_ptr.h"
#include "xcommon/xaddress.h"

#include <unordered_map>

NS_BEG3(top, tests, election)

class xtop_mocked_vnode_group : public top::base::xvnodegroup_t {
public:
    xtop_mocked_vnode_group(xtop_mocked_vnode_group const &) = delete;
    xtop_mocked_vnode_group & operator=(xtop_mocked_vnode_group const &) = delete;

    explicit xtop_mocked_vnode_group(common::xip2_t group_address_with_size_and_height);

    std::pair<xobject_ptr_t<base::xvnode_t>, common::xslot_id_t> add_node(common::xaccount_address_t account_address);

protected:
    ~xtop_mocked_vnode_group() override = default;
};
using xmocked_vnode_group_t = xtop_mocked_vnode_group;

class xtop_mocked_node_service : public top::election::xvnode_house_t {
protected:
    std::unordered_map<common::xgroup_address_t, std::vector<xobject_ptr_t<base::xvnode_t>>> m_data;
public:
    xtop_mocked_node_service() = default;
    xtop_mocked_node_service(xtop_mocked_node_service const &) = delete;
    xtop_mocked_node_service & operator=(xtop_mocked_node_service const &) = delete;
    xtop_mocked_node_service(xtop_mocked_node_service &&) = default;
    xtop_mocked_node_service & operator=(xtop_mocked_node_service &&) = default;
    ~xtop_mocked_node_service() override = default;

    bool add_group(const base::xvnodegroup_t * group_ptr) override;
    bool remove_group(const xvip2_t & target_group) override;

    xobject_ptr_t<xmocked_vnode_group_t> add_group(common::xnetwork_id_t const & nid,
                                                   common::xzone_id_t const & zid,
                                                   common::xcluster_id_t const & cid,
                                                   common::xgroup_id_t const & gid,
                                                   uint16_t const group_size,
                                                   uint64_t const election_blk_height);
};
using xmocked_nodesvr_t = xtop_mocked_node_service;

NS_END3
