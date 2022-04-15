// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>
#include "xdata/xdata_common.h"
#include "xbasic/xmemory.hpp"
#include "xbase/xutl.h"

#include "xmbus/xmessage_bus.h"
#include "xvledger/xvblock.h"
#include "xsync/xsync_store.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_sender.h"
#include "xsync/xgossip_message.h"
#include "xsync/xsync_peerset.h"
#include "xsync/xsync_time_rejecter.h"

NS_BEG2(top, sync)

class xsync_peer_keeper_t {
public:
    xsync_peer_keeper_t(std::string vnode_id, xsync_store_face_t *sync_store, xrole_chains_mgr_t *role_chains_mgr, xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender, xsync_peerset_t *peerset);
    void on_timer();
    std::vector<vnetwork::xvnode_address_t> get_random_neighbors(const vnetwork::xvnode_address_t& addr) const ;
    void add_role(const vnetwork::xvnode_address_t& addr);
    void remove_role(const vnetwork::xvnode_address_t& addr);
    void handle_message(const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &from_address, const std::vector<xchain_state_info_t> &info_list);
    uint32_t get_frozen_broadcast_factor();

private:
    void walk_role(const vnetwork::xvnode_address_t &self_addr, const std::set<vnetwork::xvnode_address_t> &target_list, const std::shared_ptr<xrole_chains_t> &role_chains);
    void process_timer();
    void send_chain_state(const vnetwork::xvnode_address_t &self_addr, const std::set<vnetwork::xvnode_address_t> &target_list, std::vector<xchain_state_info_t> &info_list);
    void send_frozen_chain_state(const vnetwork::xvnode_address_t &self_addr, std::vector<xchain_state_info_t> &info_list);
    void prune_table(const vnetwork::xvnode_address_t &self_addr, const map_chain_info_t &chains);
private:
    std::string m_vnode_id;
    xsync_store_face_t *m_sync_store;
    xrole_chains_mgr_t *m_role_chains_mgr;
    xrole_xips_manager_t *m_role_xips_mgr;
    xsync_sender_t *m_sync_sender;
    xsync_peerset_t *m_peerset;
    uint32_t m_count{0};

    std::mutex m_lock;
    std::map<vnetwork::xvnode_address_t, std::set<vnetwork::xvnode_address_t>> m_maps;
    xsync_time_rejecter_t m_time_rejecter{900};
};

NS_END2
