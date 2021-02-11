// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"
#include "xsync/xsync_store.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xsync_sender.h"

NS_BEG2(top, sync)

class xsync_latest_t {
public:
    xsync_latest_t(const std::string &vnode_id, const observer_ptr<base::xvcertauth_t> &certauth, xsync_store_face_t *sync_store, xrole_chains_mgr_t *role_chains_mgr, xsync_sender_t *sync_sender);
    void add_role(const vnetwork::xvnode_address_t& addr);
    void on_timer();
    void handle_latest_block_info(const std::vector<xlatest_block_info_t> &info_list, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);
    void handle_latest_blocks(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &from_address);

private:
    std::string m_vnode_id;
    observer_ptr<base::xvcertauth_t> m_certauth;
    xsync_store_face_t *m_sync_store;
    xrole_chains_mgr_t *m_role_chains_mgr;
    xsync_sender_t *m_sync_sender;

protected:
    int64_t m_last_send_time{0};
};


NS_END2