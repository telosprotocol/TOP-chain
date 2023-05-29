// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xelect_net/include/elect_main.h"
#include "xgrpc_mgr/xgrpc_mgr.h"
#include "xmbus/xmessage_bus.h"
#include "xrouter/xrouter_face.h"
#include "xsync/xsync_object.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xunit_service/xcons_face.h"
#include "xvnetwork/xvhost_face.h"
#include "xvnode/xvnode_factory_face.h"
#include "xbasic/xtimer_driver_fwd.h"

#include <memory>

NS_BEG2(top, vnode)

class xtop_vnode_factory final : public xtop_vnode_factory_face {
private:
    observer_ptr<elect::ElectMain> m_elect_main;
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
    observer_ptr<base::xvblockstore_t> m_block_store;
    observer_ptr<base::xvtxstore_t> m_txstore;
    observer_ptr<time::xchain_time_face_t> m_logic_timer;
    observer_ptr<router::xrouter_face_t> m_router;
    observer_ptr<vnetwork::xvhost_face_t> m_vhost;
    observer_ptr<sync::xsync_object_t> m_sync_obj;
    observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> m_txpool_service_mgr;
    observer_ptr<election::cache::xdata_accessor_face_t> m_election_cache_data_accessor;
    observer_ptr<base::xvnodesrv_t> m_nodesvr;

public:
    xtop_vnode_factory(xtop_vnode_factory const &) = delete;
    xtop_vnode_factory & operator=(xtop_vnode_factory const &) = delete;
    xtop_vnode_factory(xtop_vnode_factory &&) = default;
    xtop_vnode_factory & operator=(xtop_vnode_factory &&) = default;
    ~xtop_vnode_factory() override = default;

    xtop_vnode_factory(observer_ptr<elect::ElectMain> elect_main,
                       observer_ptr<mbus::xmessage_bus_face_t> bus,
                       observer_ptr<base::xvblockstore_t> blockstore,
                       observer_ptr<base::xvtxstore_t> txstore,
                       observer_ptr<time::xchain_time_face_t> logic_timer,
                       observer_ptr<router::xrouter_face_t> router,
                       observer_ptr<vnetwork::xvhost_face_t> vhost,
                       observer_ptr<sync::xsync_object_t> sync,
                       observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> txpool_service_mgr,
                       observer_ptr<election::cache::xdata_accessor_face_t> cache_data_accessor,
                       observer_ptr<base::xvnodesrv_t> const & nodesvr);

    std::shared_ptr<xvnode_face_t> create_vnode_at(std::shared_ptr<election::cache::xgroup_element_t> const & group) const override;
};
using xvnode_factory_t = xtop_vnode_factory;

NS_END2
