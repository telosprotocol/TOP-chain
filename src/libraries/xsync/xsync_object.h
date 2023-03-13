// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include "xbasic/xmemory.hpp"
#include "xbasic/xrunnable.h"
#include "xmbus/xmessage_bus.h"
#include "xsync/xdeceit_node_manager.h"
//#include "xsync/xsession.h"
#include "xsync/xsync_session_manager.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_sender.h"
#include "xsync/xsync_netmsg_dispatcher.h"
#include "xsync/xsync_handler.h"
#include "xsyncbase/xsync_face.h"
#include "xsync/xsync_event_dispatcher.h"
#include "xvnetwork/xvhost_face.h"
#include "xvledger/xvcnode.h"
#include "xblockstore/xblockstore_face.h"
#include "xsync/xsync_store.h"
#include "xsync/xsync_ratelimit.h"
#include "xsync/xblock_fetcher.h"
#include "xsync/xdownloader.h"
#include "xsync/xsync_gossip.h"
#include "xsync/xsync_on_demand.h"
#include "xsync/xsync_peerset.h"
#include "xsync/xsync_peer_keeper.h"
#include "xsync/xsync_behind_checker.h"
#include "xsync/xsync_cross_cluster_chain_state.h"
#include "xsync/xsync_pusher.h"
#include "xvnetwork/xaddress.h"

NS_BEG2(top, sync)

class xsync_table_data {
public:
    xsync_table_data():display(false),total_cur_height(0),total_max_height(0) {}
    bool display;
    uint64_t total_cur_height;
    uint64_t total_max_height;
    //sync::enum_chain_sync_policy policy;
};
class xtop_sync_object final : public xbasic_runnable_t<xtop_sync_object>, public xsync_face_t {
private:
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
    std::string m_instance;

    std::unique_ptr<xsync_store_shadow_t> m_store_shadow{};
    std::unique_ptr<sync::xsync_store_face_t> m_sync_store{};
    std::unique_ptr<sync::xdeceit_node_manager_t> m_blacklist{};
    std::unique_ptr<sync::xsync_session_manager_t> m_session_mgr{};

    std::unique_ptr<sync::xrole_chains_mgr_t> m_role_chains_mgr{};
    std::unique_ptr<sync::xrole_xips_manager_t> m_role_xips_mgr{};
    std::unique_ptr<sync::xsync_sender_t> m_sync_sender{};
    std::unique_ptr<sync::xsync_ratelimit_face_t> m_sync_ratelimit{};
    std::unique_ptr<sync::xsync_peerset_t> m_peerset{};
    std::unique_ptr<sync::xsync_pusher_t> m_sync_pusher{};
    std::unique_ptr<sync::xdownloader_t> m_downloader{};

    std::unique_ptr<sync::xblock_fetcher_t> m_block_fetcher{};
    std::unique_ptr<sync::xsync_gossip_t> m_sync_gossip{};
    std::unique_ptr<sync::xsync_on_demand_t> m_sync_on_demand{};

    std::unique_ptr<sync::xsync_peer_keeper_t> m_peer_keeper{};
    std::unique_ptr<sync::xsync_behind_checker_t> m_behind_checker{};
    std::unique_ptr<sync::xsync_cross_cluster_chain_state_t> m_cross_cluster_chain_state{};

    std::unique_ptr<sync::xsync_handler_t> m_sync_handler{};

    xobject_ptr_t<sync::xsync_event_dispatcher_t> m_sync_event_dispatcher{};
    std::unique_ptr<sync::xsync_netmsg_dispatcher_t> m_sync_netmsg_dispatcher{};

public:
    xtop_sync_object(xtop_sync_object const &)             = delete;
    xtop_sync_object & operator=(xtop_sync_object const &) = delete;
    xtop_sync_object(xtop_sync_object &&)                  = default;
    xtop_sync_object & operator=(xtop_sync_object &&)      = default;
    ~xtop_sync_object()                                    = default;

    xtop_sync_object(observer_ptr<mbus::xmessage_bus_face_t> const & bus,
                     observer_ptr<vnetwork::xvhost_face_t> const & vhost,
                     xobject_ptr_t<base::xvblockstore_t> &blockstore,
                     xobject_ptr_t<base::xvcertauth_t> &cert_ptr,
                     observer_ptr<base::xiothread_t> const & sync_thread,
                     std::vector<observer_ptr<base::xiothread_t>> const & sync_account_thread_pool,
                     std::vector<observer_ptr<base::xiothread_t>> const & sync_handler_thread_pool);

    void
    start() override;

    void
    stop() override;

    //void
    //turn_on_synchronizing(observer_ptr<vnode::xvnode_t> const & node);

    std::string help() const override;
    std::string status() const override;
    std::map<std::string, std::vector<std::string>> get_neighbors() const override;
    std::string auto_prune_data(const std::string& prune) const override;

    void add_vnet(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver, const common::xminer_type_t miner_type, const bool genesis);
    void remove_vnet(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnetwork_driver, const common::xminer_type_t miner_type, const bool genesis);
private:
    void display_init(std::map<common::xenum_node_type, xsync_table_data>& table_display) const;
    common::xenum_node_type get_table_type(const common::xtable_address_t& account) const;
    std::string get_title(common::xenum_node_type type) const;
};
using xsync_object_t = xtop_sync_object;

class xtop_sync_out_object {
public:
    static xtop_sync_out_object & instance();
    void set_xsync_shadow(top::sync::xsync_store_shadow_t * shadow);
    void save_span();
private:
    static xtop_sync_out_object * __global_sync_out_instance;
    xsync_store_shadow_t* m_store_shadow{NULL};
};
NS_END2
