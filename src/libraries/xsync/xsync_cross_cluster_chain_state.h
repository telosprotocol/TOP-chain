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

#include "xvledger/xvblock.h"
#include "xsync/xsync_store.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_sender.h"
#include "xsync/xdownloader.h"
#include "xsync/xsync_time_rejecter.h"

NS_BEG2(top, sync)

class xsync_cross_cluster_chain_state_t {
public:
    xsync_cross_cluster_chain_state_t(std::string vnode_id, xsync_store_face_t* sync_store,
            xrole_chains_mgr_t *role_chains_mgr, xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender, xdownloader_face_t *downloader);
    void on_timer();
    void handle_message(const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &from_address,
        const std::vector<xchain_state_info_t> &info_list);

private:
    std::string m_vnode_id;
    xsync_store_face_t* m_sync_store;
    xrole_chains_mgr_t *m_role_chains_mgr;
    xrole_xips_manager_t *m_role_xips_mgr;
    xsync_sender_t *m_sync_sender;
    xdownloader_face_t *m_downloader;
    uint32_t m_counter{0};
    xsync_time_rejecter_t m_time_rejecter{900};
};

NS_END2
