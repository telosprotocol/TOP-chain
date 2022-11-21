// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash.hpp"
#include "xmbus/xmessage_bus.h"
#include "xstate_sync/xstate_downloader_executer.h"
#include "xstate_sync/xstate_sync.h"
#include "xstatestore/xstatestore_face.h"
#include "xvledger/xvdbstore.h"
#include "xvnetwork/xvnetwork_driver_face.h"

namespace top {
namespace state_sync {

class xtop_state_downloader {
private:
    using sync_networks = std::vector<std::shared_ptr<vnetwork::xvnetwork_driver_face_t>>;

    base::xvdbstore_t * m_db{nullptr};
    statestore::xstatestore_face_t * m_store{nullptr};
    xobject_ptr_t<base::xiothread_t> m_executor_thread{nullptr};
    xobject_ptr_t<base::xiothread_t> m_syncer_thread{nullptr};
    observer_ptr<mbus::xmessage_bus_face_t> m_bus{nullptr};

    std::map<common::xaccount_address_t, std::shared_ptr<xdownload_executer_t>> m_running_tables;
    std::map<common::xaccount_address_t, std::shared_ptr<xdownload_executer_t>> m_running_units;
    sync_networks m_networks;

    mutable std::mutex m_table_dispatch;
    mutable std::mutex m_unit_dispatch;
    mutable std::mutex m_mutex;

public:
    xtop_state_downloader(base::xvdbstore_t * db,
                          statestore::xstatestore_face_t * store,
                          const observer_ptr<mbus::xmessage_bus_face_t> & msg_bus,
                          xobject_ptr_t<base::xiothread_t> executor_thread,
                          xobject_ptr_t<base::xiothread_t> syncer_thread);
    ~xtop_state_downloader() = default;

    // sync actions
    void sync_state(const common::xaccount_address_t & table,
                    const uint64_t height,
                    const evm_common::xh256_t & block_hash,
                    const evm_common::xh256_t & state_hash,
                    const evm_common::xh256_t & root_hash,
                    bool sync_unit,
                    std::error_code & ec);
    void sync_unit_state(const common::xaccount_address_t & account, const base::xaccount_index_t & index, std::error_code & ec);
    bool is_syncing(const common::xaccount_address_t & table) const;
    void sync_cancel(const common::xaccount_address_t & table) const;
    void handle_message(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message) const;

    // peer actions
    void add_network(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network);
    void del_network(const vnetwork::xvnode_address_t & vnode);
    sync_peers latest_peers(const common::xtable_id_t & id) const;

private:
    void process_trie_request(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message) const;
    void process_trie_response(const vnetwork::xmessage_t & message) const;
    void process_table_request(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message) const;
    void process_table_response(const vnetwork::xmessage_t & message) const;
    void process_unit_request(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message) const;
    void process_unit_response(const vnetwork::xmessage_t & message) const;
    void process_trie_finish(const sync_result & res);
    void process_unit_finish(const sync_result & res);
};
using xstate_downloader_t = xtop_state_downloader;

}  // namespace state_sync
}  // namespace top
