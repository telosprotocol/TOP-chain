// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcertauth/xcertauth_face.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xaddress.h"
#include "xcommon/xnode_type.h"
#include "xgrpc_mgr/xgrpc_mgr.h"
#include "xmbus/xmessage_bus.h"
#include "xrouter/xrouter_face.h"
#include "xstate_sync/xstate_downloader.h"
#include "xsync/xsync_object.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xunit_service/xcons_face.h"
#include "xvnode/xvnode_role_proxy_face.h"

NS_BEG2(top, vnode)

class xtop_vnode_role_proxy final : public xvnode_role_proxy_face_t {
private:
    std::set<common::xnode_address_t> m_node_address_set;
    observer_ptr<base::xvtxstore_t> m_txstore;
    observer_ptr<state_sync::xstate_downloader_t> m_downloader;
    xunit_service::xcons_service_mgr_ptr m_cons_mgr;

public:
    xtop_vnode_role_proxy(xtop_vnode_role_proxy const &) = delete;
    xtop_vnode_role_proxy & operator=(xtop_vnode_role_proxy const &) = delete;
    xtop_vnode_role_proxy(xtop_vnode_role_proxy &&) = default;
    xtop_vnode_role_proxy & operator=(xtop_vnode_role_proxy &&) = default;
    ~xtop_vnode_role_proxy() override = default;

    xtop_vnode_role_proxy(observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
                          observer_ptr<base::xvblockstore_t> const & block_store,
                          observer_ptr<base::xvtxstore_t> const & txstore,
                          observer_ptr<time::xchain_time_face_t> const & logic_timer,
                          observer_ptr<router::xrouter_face_t> const & router,
                          xobject_ptr_t<base::xvcertauth_t> const & certauth,
                          observer_ptr<xtxpool_v2::xtxpool_face_t> const & txpool,
                          observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
                          observer_ptr<state_sync::xstate_downloader_t> const & downloader);
    void create(vnetwork::xvnetwork_driver_face_ptr_t const & vnetwork) override;
    void change(common::xnode_address_t const & address, common::xlogic_time_t start_time) override;
    void fade(common::xnode_address_t const & address) override;
    void unreg(common::xnode_address_t const & address) override;
    void destroy(common::xnode_address_t const & address) override;

private:
    void update_modules_node_type() const;
    bool is_frozen(common::xnode_type_t const & node_type) const;
    bool is_edge_archive(common::xnode_type_t const & node_type) const;
};

using xvnode_role_proxy_t = xtop_vnode_role_proxy;

NS_END2
