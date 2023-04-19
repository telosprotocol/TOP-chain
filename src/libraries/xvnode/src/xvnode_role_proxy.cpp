// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xvnode_role_proxy.h"

#include "xunit_service/xcons_service_mgr.h"
#include "xstatestore/xstatestore_face.h"
NS_BEG2(top, vnode)

xtop_vnode_role_proxy::xtop_vnode_role_proxy(observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
                                             observer_ptr<base::xvblockstore_t> const & block_store,
                                             observer_ptr<base::xvtxstore_t> const & txstore,
                                             observer_ptr<time::xchain_time_face_t> const & logic_timer,
                                             observer_ptr<router::xrouter_face_t> const & router,
                                             xobject_ptr_t<base::xvcertauth_t> const & certauth,
                                             observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                                             //    std::vector<xobject_ptr_t<base::xiothread_t>> const & iothreads,
                                             observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
                                             observer_ptr<state_sync::xstate_downloader_t> const & downloader)
  : m_txstore{txstore} {
    m_downloader = downloader;
    m_cons_mgr = xunit_service::xcons_mgr_build(
        data::xuser_params::get_instance().account.to_string(),
                                                block_store,
                                                txpool,
                                                logic_timer,
                                                certauth,
                                                election_cache_data_accessor,
                                                mbus,
                                                router,
                                                downloader);
}

bool xtop_vnode_role_proxy::is_edge_archive(common::xnode_type_t const & node_type) const {
    return common::has<common::xnode_type_t::storage>(node_type) || common::has<common::xnode_type_t::edge>(node_type);
}

bool xtop_vnode_role_proxy::is_frozen(common::xnode_type_t const & node_type) const {
    return common::has<common::xnode_type_t::frozen>(node_type);
}

void xtop_vnode_role_proxy::create(vnetwork::xvnetwork_driver_face_ptr_t const & vnetwork) {
    xdbg("xtop_vnode_role_proxy::create node_type: %s %x", to_string(vnetwork->type()).c_str(), static_cast<std::underlying_type<common::xnode_type_t>::type>(vnetwork->type()));
    m_node_address_set.insert(vnetwork->address());
    update_modules_node_type();

    if (!is_edge_archive(vnetwork->type()) && !is_frozen(vnetwork->type()) && !common::has<common::xnode_type_t::fullnode>(vnetwork->type())) {
        m_cons_mgr->create(vnetwork);
    }
    if (!is_frozen(vnetwork->type())) {
        m_downloader->add_network(vnetwork);
    }
}
void xtop_vnode_role_proxy::change(common::xnode_address_t const & address, common::xlogic_time_t start_time) {
    m_node_address_set.insert(address);
    update_modules_node_type();

    if (!is_edge_archive(address.type()) && !is_frozen(address.type()) && !common::has<common::xnode_type_t::fullnode>(address.type())) {
        m_cons_mgr->start(address.xip2(), start_time);
    }
}

void xtop_vnode_role_proxy::fade(common::xnode_address_t const & address) {
    if (!is_edge_archive(address.type()) && !is_frozen(address.type()) && !common::has<common::xnode_type_t::fullnode>(address.type())) {
        m_cons_mgr->fade(address.xip2());
    }
}

void xtop_vnode_role_proxy::unreg(common::xnode_address_t const & address) {
    m_node_address_set.erase(address);
    update_modules_node_type();

    if (!is_edge_archive(address.type()) && !is_frozen(address.type()) && !common::has<common::xnode_type_t::fullnode>(address.type())) {
        m_cons_mgr->unreg(address.xip2());
    }
    if (!is_frozen(address.type())) {
        m_downloader->del_network(address);
    }
}

void xtop_vnode_role_proxy::destroy(common::xnode_address_t const & address) {
    common::xip2_t const xip2 = address.xip2().group_xip2();
    m_cons_mgr->destroy(xip2);
}

void xtop_vnode_role_proxy::update_modules_node_type() const {
    //  add update component
    common::xnode_type_t node_type{common::xnode_type_t::invalid};
    for (auto const & address : m_node_address_set) {
        node_type |= address.type();
    }

    // TODO(jimmy) mbus notify future
    m_txstore->update_node_type(static_cast<std::underlying_type<common::xnode_type_t>::type>(node_type));
}

NS_END2
