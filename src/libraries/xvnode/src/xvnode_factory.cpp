// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xvnode_factory.h"

#include "xvnode/xvnode.h"
#include "xvnode/xvnode_manager.h"

#include <cassert>

NS_BEG2(top, vnode)

xtop_vnode_factory::xtop_vnode_factory(observer_ptr<elect::ElectMain> elect_main,
                                       observer_ptr<mbus::xmessage_bus_face_t> bus,
                                       observer_ptr<store::xstore_face_t> store,
                                       observer_ptr<base::xvblockstore_t> blockstore,
                                       observer_ptr<time::xchain_time_face_t> logic_timer,
                                       observer_ptr<router::xrouter_face_t> router,
                                       observer_ptr<vnetwork::xvhost_face_t> vhost,
                                       observer_ptr<sync::xsync_object_t> sync,
                                       observer_ptr<grpcmgr::xgrpc_mgr_t> grpc,
                                       observer_ptr<xunit_service::xcons_service_mgr_face> cons_service_mgr,
                                       observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> txpool_service_mgr,
                                       observer_ptr<xtxpool_v2::xtxpool_face_t> txpool,
                                       observer_ptr<election::cache::xdata_accessor_face_t> cache_data_accessor,
                                       observer_ptr<xbase_timer_driver_t> timer_driver)
  : m_elect_main{std::move(elect_main)}
  , m_bus{std::move(bus)}
  , m_store{std::move(store)}
  , m_block_store{std::move(blockstore)}
  , m_logic_timer{std::move(logic_timer)}
  , m_router{std::move(router)}
  , m_vhost{std::move(vhost)}
  , m_sync_obj{std::move(sync)}
  , m_grpc_mgr{std::move(grpc)}
  , m_cons_mgr{std::move(cons_service_mgr)}
  , m_txpool_service_mgr{std::move(txpool_service_mgr)}
  , m_txpool{std::move(txpool)}
  , m_election_cache_data_accessor{std::move(cache_data_accessor)}
  , m_timer_driver{std::move(timer_driver)} {}

std::shared_ptr<xvnode_face_t> xtop_vnode_factory::create_vnode_at(std::shared_ptr<election::cache::xgroup_element_t> const & group) const {
    return std::make_shared<xvnode_t>(m_elect_main,
                                      m_vhost,
                                      group,
                                      m_router,
                                      m_store,
                                      m_block_store,
                                      m_bus,
                                      m_logic_timer,
                                      m_sync_obj,
                                      m_grpc_mgr,
                                      m_cons_mgr,
                                      m_txpool_service_mgr,
                                      m_txpool,
                                      m_election_cache_data_accessor,
                                      m_timer_driver);
}

NS_END2
