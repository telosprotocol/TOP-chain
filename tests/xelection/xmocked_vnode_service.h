// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xcommon/xaddress.h"
#include "xelection/xvnode_house.h"
#include "xmbus/xmessage_bus.h"

#include <unordered_map>

NS_BEG3(top, tests, election)

class xtop_mocked_vnode_group : public top::base::xvnodegroup_t {
public:
    xtop_mocked_vnode_group(xtop_mocked_vnode_group const &) = delete;
    xtop_mocked_vnode_group & operator=(xtop_mocked_vnode_group const &) = delete;

    explicit xtop_mocked_vnode_group(common::xip2_t group_address_with_size_and_height);

    std::pair<xobject_ptr_t<base::xvnode_t>, common::xslot_id_t> add_node(common::xaccount_address_t account_address);
    void reset_nodes();

protected:
    ~xtop_mocked_vnode_group() override = default;
};
using xmocked_vnode_group_t = xtop_mocked_vnode_group;

class xtop_mocked_vnode_service : public top::election::xvnode_house_t {
protected:
    std::unordered_map<common::xgroup_address_t, std::vector<xobject_ptr_t<base::xvnode_t>>> m_data;
public:
    xtop_mocked_vnode_service() = default;
    xtop_mocked_vnode_service(xtop_mocked_vnode_service const &) = delete;
    xtop_mocked_vnode_service & operator=(xtop_mocked_vnode_service const &) = delete;
    xtop_mocked_vnode_service(xtop_mocked_vnode_service &&) = default;
    xtop_mocked_vnode_service & operator=(xtop_mocked_vnode_service &&) = default;
    ~xtop_mocked_vnode_service() override = default;

    xtop_mocked_vnode_service(common::xaccount_address_t const & account_address,
                              std::string const & sign_key,
                              xobject_ptr_t<base::xvblockstore_t> const & blockstore,
                              observer_ptr<top::mbus::xmessage_bus_face_t> const & bus);

    xtop_mocked_vnode_service(common::xaccount_address_t const & account_address,
                              std::string const & sign_key);

    xobject_ptr_t<xmocked_vnode_group_t> add_group(common::xnetwork_id_t const & nid,
                                                   common::xzone_id_t const & zid,
                                                   common::xcluster_id_t const & cid,
                                                   common::xgroup_id_t const & gid,
                                                   uint16_t const group_size,
                                                   uint64_t const election_blk_height);
};
using xmocked_vnodesvr_t = xtop_mocked_vnode_service;

NS_END3
