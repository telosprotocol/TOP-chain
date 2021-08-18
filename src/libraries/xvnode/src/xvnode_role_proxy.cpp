// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xvnode_role_proxy.h"

#include "xunit_service/xcons_service_mgr.h"

NS_BEG2(top, vnode)

xtop_vnode_role_proxy::xtop_vnode_role_proxy(observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
                                   observer_ptr<store::xstore_face_t> const & store,
                                   observer_ptr<base::xvblockstore_t> const & block_store,
                                   observer_ptr<time::xchain_time_face_t> const & logic_timer,
                                   observer_ptr<router::xrouter_face_t> const & router,
                                   xobject_ptr_t<base::xvcertauth_t> const & certauth,
                                   observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                                   //    std::vector<xobject_ptr_t<base::xiothread_t>> const & iothreads,
                                   observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor) {
    m_cons_mgr = xunit_service::xcons_mgr_build(
        data::xuser_params::get_instance().account.value(), store, block_store, txpool, logic_timer, certauth, election_cache_data_accessor, mbus, router);
    // m_txpool_service_mgr = xtxpool_service_v2::xtxpool_service_mgr_instance::create_xtxpool_service_mgr_inst(store, block_store, txpool, iothreads, mbus, logic_timer);
}

bool xtop_vnode_role_proxy::is_edge_archive(common::xnode_type_t const & node_type) {
    return common::has<common::xnode_type_t::storage>(node_type) || common::has<common::xnode_type_t::edge>(node_type);
}

bool xtop_vnode_role_proxy::is_frozen(common::xnode_type_t const & node_type) {
    return common::has<common::xnode_type_t::frozen>(node_type);
}

void xtop_vnode_role_proxy::create(vnetwork::xvnetwork_driver_face_ptr_t const & vnetwork) {
    if (!is_edge_archive(vnetwork->type()) && !is_frozen(vnetwork->type())) {
        m_cons_mgr->create(vnetwork);
    }
}
void xtop_vnode_role_proxy::change(common::xnode_address_t address, common::xlogic_time_t start_time) {
    if (!is_edge_archive(address.type()) && !is_frozen(address.type())) {
        m_cons_mgr->start(address.xip2(), start_time);
    }
}

void xtop_vnode_role_proxy::unreg(common::xnode_address_t address) {
    if (!is_edge_archive(address.type()) && !is_frozen(address.type())) {
        m_cons_mgr->unreg(address.xip2());
    }
}

void xtop_vnode_role_proxy::destroy(xvip2_t xip2) {
    m_cons_mgr->destroy(xip2);
}

NS_END2