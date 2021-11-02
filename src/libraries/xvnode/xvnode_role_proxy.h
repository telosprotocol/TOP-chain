// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcertauth/xcertauth_face.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xelect_net/include/elect_main.h"
#include "xgrpc_mgr/xgrpc_mgr.h"
#include "xmbus/xmessage_bus.h"
#include "xrouter/xrouter_face.h"
#include "xstore/xstore_face.h"
#include "xsync/xsync_object.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xunit_service/xcons_face.h"
#include "xvnode/xvnode_role_proxy_face.h"
#include "xcommon/xnode_type.h"
#include "xcommon/xaddress.h"

NS_BEG2(top, vnode)

class xtop_vnode_role_proxy final : public xtop_vnode_role_proxy_face {
private:
    std::set<common::xnode_address_t> m_node_address_set;
private:
    // observer_ptr<mbus::xmessage_bus_face_t> const & mbus;
    // observer_ptr<store::xstore_face_t> const & store;
    // observer_ptr<base::xvblockstore_t> const & block_store;
    // observer_ptr<time::xchain_time_face_t> const & logic_timer;
    // observer_ptr<router::xrouter_face_t> const & router;
    // xobject_ptr_t<base::xvcertauth_t> const & certauth;
    // observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool;
    // std::vector<xobject_ptr_t<base::xiothread_t>> const & iothreads;
    // observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor;

    xunit_service::xcons_service_mgr_ptr m_cons_mgr;
    observer_ptr<base::xvtxstore_t> m_txstore;
    // std::shared_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> m_txpool_service_mgr;

public:
    xtop_vnode_role_proxy(observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
                          observer_ptr<store::xstore_face_t> const & store,
                          observer_ptr<base::xvblockstore_t> const & block_store,
                          observer_ptr<base::xvtxstore_t> const & txstore,
                          observer_ptr<time::xchain_time_face_t> const & logic_timer,
                          observer_ptr<router::xrouter_face_t> const & router,
                          xobject_ptr_t<base::xvcertauth_t> const & certauth,
                          observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                          //  std::vector<xobject_ptr_t<base::xiothread_t>> const & iothreads,
                          observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor);

    // void start() override;
    
    // void stop() override;

    void create(vnetwork::xvnetwork_driver_face_ptr_t const & vnetwork) override;
    void change(common::xnode_address_t const & address, common::xlogic_time_t start_time) override;
    void fade(common::xnode_address_t const & address) override;
    void unreg(common::xnode_address_t const & address) override;
    void destroy(common::xnode_address_t const & address) override;

private:
    void update_modules_node_type() const;

private:
    bool is_frozen(common::xnode_type_t const & node_type) const;
    bool is_edge_archive(common::xnode_type_t const & node_type) const;
};

using xvnode_role_proxy_t = xtop_vnode_role_proxy;

NS_END2