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
                                       observer_ptr<base::xvblockstore_t> blockstore,
                                       observer_ptr<base::xvtxstore_t> txstore,
                                       observer_ptr<time::xchain_time_face_t> logic_timer,
                                       observer_ptr<router::xrouter_face_t> router,
                                       observer_ptr<vnetwork::xvhost_face_t> vhost,
                                       observer_ptr<sync::xsync_object_t> sync,
                                       observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> txpool_service_mgr,
                                       observer_ptr<election::cache::xdata_accessor_face_t> cache_data_accessor,
                                       observer_ptr<base::xvnodesrv_t> const & nodesvr)
  : m_elect_main{elect_main}
  , m_bus{bus}
  , m_block_store{blockstore}
  , m_txstore{txstore}
  , m_logic_timer{logic_timer}
  , m_router{router}
  , m_vhost{vhost}
  , m_sync_obj{sync}
  , m_txpool_service_mgr{txpool_service_mgr}
  , m_election_cache_data_accessor{cache_data_accessor}
  , m_nodesvr{nodesvr} {
}

std::shared_ptr<xvnode_face_t> xtop_vnode_factory::create_vnode_at(std::shared_ptr<election::cache::xgroup_element_t> const & group) const {
    return std::make_shared<xvnode_t>(m_elect_main,
                                      m_vhost,
                                      group,
                                      m_router,
                                      m_block_store,
                                      m_txstore,
                                      m_bus,
                                      m_logic_timer,
                                      m_sync_obj,
                                      m_txpool_service_mgr,
                                      m_election_cache_data_accessor,
                                      m_nodesvr);
}

NS_END2
