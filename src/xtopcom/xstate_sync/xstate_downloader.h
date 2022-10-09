// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash.hpp"
#include "xmbus/xmessage_bus.h"
#include "xstate_sync/xstate_sync.h"
#include "xstatestore/xstatestore_face.h"
#include "xvledger/xvdbstore.h"
#include "xvnetwork/xvnetwork_driver_face.h"

namespace top {
namespace state_sync {


class xtop_download_executer {
public:
    xtop_download_executer() = default;
    ~xtop_download_executer() = default;

    void run_state_sync(std::shared_ptr<xstate_sync_face_t> syncer, std::function<void(sync_result)> callback);
    void loop(std::shared_ptr<xstate_sync_face_t> syncer, std::error_code & ec);
    void cancel();

    void push_track_req(const state_req & req);
    void push_state_pack(const state_res & res);
    void push_single_state(const single_state_detail & detail);

private:
    void pop_track_req();
    void pop_state_pack();
    void pop_single_state();

    bool m_cancel{false};
    bool m_notify{false};
    std::list<state_req> m_track_req;
    std::list<state_res> m_state_packs;
    std::list<single_state_detail> m_single_states;
    std::mutex m_track_mutex;
    std::mutex m_state_pack_mutex;
    std::mutex m_single_state_mutex;
};
using xdownload_executer_t = xtop_download_executer;

class xtop_state_downloader {
public:
    xtop_state_downloader(base::xvdbstore_t * db, statestore::xstatestore_face_t * store, const observer_ptr<mbus::xmessage_bus_face_t> & msg_bus);
    ~xtop_state_downloader() = default;

    bool is_syncing(const common::xaccount_address_t & table);
    // sync actions
    void sync_state(const common::xaccount_address_t & table,
                    const uint64_t height,
                    const xhash256_t & block_hash,
                    const xhash256_t & state_hash,
                    const xhash256_t & root_hash,
                    bool sync_unit,
                    std::error_code & ec);
    void sync_unit_state(const common::xaccount_address_t & account, const base::xaccount_index_t & index, std::error_code & ec);
    void sync_cancel(const common::xaccount_address_t & table);
    void handle_message(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message);

    // peer actions
    state_sync_peers_t get_peers();
    void add_peer(const vnetwork::xvnode_address_t & peer, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network);
    void del_peer(const vnetwork::xvnode_address_t & peer);

private:
    void process_request(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message);
    void process_response(const vnetwork::xmessage_t & message);
    void process_table_request(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message);
    void process_table_response(const vnetwork::xmessage_t & message);
    void process_unit_request(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message);
    void process_unit_response(const vnetwork::xmessage_t & message);
    void process_finish(const sync_result & res);
    void process_unit_finish(const sync_result & res);

    base::xvdbstore_t * m_db{nullptr};
    statestore::xstatestore_face_t * m_store{nullptr};
    observer_ptr<mbus::xmessage_bus_face_t> m_bus{nullptr};

    std::map<common::xaccount_address_t, std::shared_ptr<xdownload_executer_t>> m_running_tables;
    std::map<common::xaccount_address_t, std::shared_ptr<xdownload_executer_t>> m_running_units;
    state_sync_peers_t m_peers;

    std::mutex m_table_dispatch;
    std::mutex m_unit_dispatch;
    std::mutex m_peers_mutex;
};
using xstate_downloader_t = xtop_state_downloader;

}  // namespace state_sync
}  // namespace top