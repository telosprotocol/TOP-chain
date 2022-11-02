// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xelection/xmocked_vnode_service.h"

#include "tests/xmbus/xdummy_message_bus.h"
#include "tests/xvledger_test/xdummy_blockstore.h"

NS_BEG3(top, tests, election)

struct xtop_mocked_vnode_service_parameters {
    xobject_ptr_t<base::xiothread_t> dummy_thread = make_object_ptr<base::xiothread_t>();
    xobject_ptr_t<base::xvblockstore_t> dummy_block_store = make_object_ptr<tests::vledger::xdummy_block_store_t>(dummy_thread->get_thread_id());
    xobject_ptr_t<top::mbus::xmessage_bus_face_t> dummy_message_bus = make_object_ptr<tests::mbus::xdummy_message_bus_t>();
};
using xmocked_vnode_service_parameters_t = xtop_mocked_vnode_service_parameters;

static xmocked_vnode_service_parameters_t mocked_vnode_service_parameters;

xtop_mocked_vnode_group::xtop_mocked_vnode_group(common::xip2_t group_address_with_size_and_height) : top::base::xvnodegroup_t(group_address_with_size_and_height.value(), static_cast<uint64_t>(0), std::vector<top::base::xvnode_t *>{}) {
}

std::pair<xobject_ptr_t<base::xvnode_t>, common::xslot_id_t> xtop_mocked_vnode_group::add_node(common::xaccount_address_t account_address) {
    auto slot_id = m_nodes.size();

    common::xip2_t group_xip2{ get_xip2_addr() };
    common::xip2_t node_xip2{
        group_xip2.network_id(),
        group_xip2.zone_id(),
        group_xip2.cluster_id(),
        group_xip2.group_id(),
        // common::xdefault_network_version,
        group_xip2.size(),
        group_xip2.height()
    };

    base::xvnode_t * node = new base::xvnode_t(account_address.to_string(), node_xip2.value(), "");
    m_nodes.push_back(node);

    node->add_ref();
    xobject_ptr_t<base::xvnode_t> r;
    r.attach(node);

    return { r, common::xslot_id_t{slot_id} };
}

void xtop_mocked_vnode_group::reset_nodes() {
    m_nodes.resize(0);
}

xtop_mocked_vnode_service::xtop_mocked_vnode_service(common::xaccount_address_t const & account_address,
                                                     std::string const & sign_key,
                                                     xobject_ptr_t<base::xvblockstore_t> const & blockstore,
                                                     observer_ptr<top::mbus::xmessage_bus_face_t> const & bus)
    : xvnode_house_t(account_address, blockstore, bus) {
}


xtop_mocked_vnode_service::xtop_mocked_vnode_service(common::xaccount_address_t const & account_address, std::string const & sign_key)
    : xtop_mocked_vnode_service(account_address, sign_key, mocked_vnode_service_parameters.dummy_block_store, make_observer(mocked_vnode_service_parameters.dummy_message_bus.get())) {
}

xobject_ptr_t<xmocked_vnode_group_t> xtop_mocked_vnode_service::add_group(common::xnetwork_id_t const & nid,
                                                                          common::xzone_id_t const & zid,
                                                                          common::xcluster_id_t const & cid,
                                                                          common::xgroup_id_t const & gid,
                                                                          uint16_t const group_size,
                                                                          uint64_t const election_blk_height) {
    common::xip2_t group_xip2{nid, zid, cid, gid, group_size, election_blk_height};
    {
        auto group = get_group_internal(group_xip2);
        if (group != nullptr) {
            return dynamic_xobject_ptr_cast<xmocked_vnode_group_t>(xobject_ptr_t<base::xvnodegroup_t>(group));
        }
    }

    auto * group = new xmocked_vnode_group_t{ group_xip2 };
    auto group_key = get_group_key(group_xip2);

    m_vgroups.put(group_key, group);

    group->add_ref();
    xobject_ptr_t<xmocked_vnode_group_t> g;
    g.attach(group);
    return g;
}

NS_END3
