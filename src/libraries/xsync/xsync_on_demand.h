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
#include "xsync/xsync_download_tracer_mgr.h"

NS_BEG2(top, sync)

const uint32_t max_request_block_count = 100;

class xsync_on_demand_t {
public:
    xsync_on_demand_t(std::string vnode_id, const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_store_face_t *sync_store,
        xrole_chains_mgr_t *role_chains_mgr,
        xrole_xips_manager_t *role_xips_mgr,
        xsync_sender_t *sync_sender);
    void on_behind_event(const mbus::xevent_ptr_t &e);
    void on_behind_by_hash_event(const mbus::xevent_ptr_t &e);
    void handle_blocks_by_hash_response(const std::vector<data::xblock_ptr_t> &blocks, 
        const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self);    
    xsync_download_tracer_mgr* download_tracer_mgr();
    void handle_blocks_response_with_params(const std::vector<data::xblock_ptr_t>& blocks, 
            const std::string& unit_proof_str, const vnetwork::xvnode_address_t& to_address, 
            const vnetwork::xvnode_address_t& network_self);
    void handle_blocks_response_with_hash(const xsync_msg_block_request_ptr_t& request_ptr, 
            const std::vector<data::xblock_ptr_t> &blocks,  const vnetwork::xvnode_address_t &to_address, 
            const vnetwork::xvnode_address_t &network_self);

private:
    int32_t check(const std::string &account_address);
    int32_t check(const std::string &account_address, 
        const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self);
    bool basic_check(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &to_address, const vnetwork::xvnode_address_t &network_self);
    bool check_unit_blocks(const std::vector<data::xblock_ptr_t> &blocks, std::vector<data::xblock_ptr_t> &validated_blocks);
    bool check_unit_blocks(const std::vector<data::xblock_ptr_t> &blocks, const std::string & unit_proof_str);
    bool check_unit_blocks(const std::vector<data::xblock_ptr_t> & blocks);
    bool store_blocks(const std::vector<data::xblock_ptr_t> &blocks);
    void store_on_demand_sync_blocks(const std::vector<data::xblock_ptr_t> &blocks, const std::string &unit_proof_str);

private:
    std::string m_vnode_id;
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
    observer_ptr<base::xvcertauth_t> m_certauth;
    xsync_store_face_t *m_sync_store;
    xrole_chains_mgr_t *m_role_chains_mgr;
    xrole_xips_manager_t *m_role_xips_mgr;
    xsync_sender_t *m_sync_sender;
    xsync_download_tracer_mgr m_download_tracer{};
};

NS_END2
