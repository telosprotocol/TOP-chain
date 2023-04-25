// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver_fwd.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xchain_param.h"
#include "xelect_net/include/elect_main.h"
#include "xelection/xcache/xgroup_element.h"
#include "xgrpc_mgr/xgrpc_mgr.h"
#include "xmbus/xmessage_bus.h"
#include "xrouter/xrouter.h"
#include "xrpc/xrpc_init.h"
#include "xsync/xsync_object.h"
#include "xtxpool_service_v2/xtxpool_service_face.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xunit_service/xcons_face.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xvnode/xbasic_vnode.h"
#include "xvnode/xcomponents/xblock_sniffing/xsniffer.h"
#include "xvnode/xcomponents/xprune_data/xprune_data.h"
#include "xvnode/xvnode_face.h"

#include <memory>

NS_BEG2(top, vnode)

class xtop_vnode final : public xbasic_vnode_t
                       , public std::enable_shared_from_this<xtop_vnode> {
private:
    observer_ptr<elect::ElectMain> m_elect_main;
    observer_ptr<router::xrouter_face_t> m_router;
    observer_ptr<base::xvblockstore_t> m_block_store;
    observer_ptr<base::xvtxstore_t> m_txstore;
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
    observer_ptr<time::xchain_time_face_t> m_logic_timer;
    observer_ptr<sync::xsync_object_t> m_sync_obj;
    observer_ptr<data::xuser_params> m_user_params;

    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_the_binding_driver;
    std::shared_ptr<xrpc::xrpc_init> m_rpc_services;

    // observer_ptr<xunit_service::xcons_service_mgr_face> m_cons_mgr;
    xtxpool_service_v2::xtxpool_proxy_face_ptr m_txpool_face;
    std::unique_ptr<components::prune_data::xprune_data> m_prune_data;

    std::unique_ptr<components::sniffing::xsniffer_t> m_sniff;

public:
    xtop_vnode(xtop_vnode const &) = delete;
    xtop_vnode & operator=(xtop_vnode const &) = delete;
    xtop_vnode(xtop_vnode &&) = default;
    xtop_vnode & operator=(xtop_vnode &&) = default;
    ~xtop_vnode() override = default;

    xtop_vnode(observer_ptr<elect::ElectMain> const & elect_main,
               observer_ptr<vnetwork::xvhost_face_t> const & vhost,
               std::shared_ptr<election::cache::xgroup_element_t> group_info,
               observer_ptr<router::xrouter_face_t> const & router,
               observer_ptr<base::xvblockstore_t> const & block_store,
               observer_ptr<base::xvtxstore_t> const & txstore,
               observer_ptr<mbus::xmessage_bus_face_t> const & bus,
               observer_ptr<time::xchain_time_face_t> const & logic_timer,
               observer_ptr<sync::xsync_object_t> const & sync_obj,
               observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> const & txpool_service_mgr,
               observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
               observer_ptr<base::xvnodesrv_t> const & nodesvr);

    xtop_vnode(observer_ptr<elect::ElectMain> const & elect_main,
               common::xsharding_address_t const & sharding_address,
               common::xslot_id_t const & slot_id,
               common::xelection_round_t joined_election_round,
               common::xminer_type_t miner_type,
               bool genesis,
               uint64_t raw_credit_score,
               common::xelection_round_t election_round,
               std::uint16_t const group_size,
               std::uint64_t const associated_blk_height,
               observer_ptr<vnetwork::xvhost_face_t> const & vhost,
               observer_ptr<router::xrouter_face_t> const & router,
               observer_ptr<base::xvblockstore_t> const & block_store,
               observer_ptr<base::xvtxstore_t> const & txstore,
               observer_ptr<mbus::xmessage_bus_face_t> const & bus,
               observer_ptr<time::xchain_time_face_t> const & logic_timer,
               observer_ptr<sync::xsync_object_t> const & sync_obj,
               observer_ptr<xtxpool_service_v2::xtxpool_service_mgr_face> const & txpool_service_mgr,
               observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor,
               observer_ptr<base::xvnodesrv_t> const & nodesvr);

    void synchronize() override;

    void start() override;
    void fade() override;
    void stop() override;
    components::sniffing::xsniffer_config_t sniff_config() const override;

    // xtxpool_service_v2::xtxpool_proxy_face_ptr const & txpool_proxy() const override;
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> const & vnetwork_driver() const override;

private:
    void new_driver_added();
    void driver_removed();
    void update_rpc_service();
    void update_contract_manager(bool destory);
    void sync_add_vnet();
    void sync_remove_vnet();
    bool update_auto_prune_control(top::common::xnode_type_t node_type, base::xvdbstore_t* xvdb_ptr);
};

using xvnode_t = xtop_vnode;

std::vector<common::xip2_t> get_group_nodes_xip2_from(std::shared_ptr<xvnode_face_t> const & vnode, common::xip_t const & group_xip, std::error_code & ec);

NS_END2
