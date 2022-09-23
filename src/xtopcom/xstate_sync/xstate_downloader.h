// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhash.hpp"
#include "xmbus/xmessage_bus.h"
#include "xstate_sync/xstate_sync.h"
#include "xvledger/xvdbstore.h"
#include "xvnetwork/xvnetwork_driver_face.h"

namespace top {
namespace state_sync {

struct sync_result {
    common::xaccount_address_t table;
    xhash256_t root;
    std::error_code ec;

    sync_result(common::xaccount_address_t _table, xhash256_t _root, std::error_code _ec) : table(_table), root(_root), ec(_ec) {
    }
};

class xtop_download_executer {
public:
    xtop_download_executer() = default;
    ~xtop_download_executer() = default;

    void run_state_sync(std::shared_ptr<xtop_state_sync> syncer, std::function<void(sync_result)> callback, bool sync_unit);
    void cancel();

    void push_track_req(const state_req & req);
    void push_state_pack(const state_res & res);

private:
    void pop_track_req();
    void pop_state_pack();

    bool m_cancel{false};
    std::list<state_req> m_track_req;
    std::list<state_res> m_state_packs;
    std::mutex m_track_mutex;
    std::mutex m_state_pack_mutex;
};
using xdownload_executer_t = xtop_download_executer;

class xtop_state_downloader {
public:
    explicit xtop_state_downloader(base::xvdbstore_t * db, const observer_ptr<mbus::xmessage_bus_face_t> & msg_bus);
    ~xtop_state_downloader() = default;

    // sync actions
    void sync_state(const common::xaccount_address_t & table, const xhash256_t & root, bool sync_unit, std::error_code & ec);
    void sync_cancel(const common::xaccount_address_t & table);
    void handle_message(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message);
    
    // peer actions
    state_sync_peers_t get_peers();
    void add_peer(const vnetwork::xvnode_address_t & peer, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network);
    void del_peer(const vnetwork::xvnode_address_t & peer);

private:
    void process_request(const vnetwork::xvnode_address_t & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const vnetwork::xmessage_t & message);
    void process_response(const vnetwork::xmessage_t & message);
    void process_finish(const sync_result & res);

    base::xvdbstore_t * m_db{nullptr};
    observer_ptr<mbus::xmessage_bus_face_t> m_bus{nullptr};

    std::map<common::xaccount_address_t, std::shared_ptr<xdownload_executer_t>> m_running;
    state_sync_peers_t m_peers;

    std::mutex m_dispatch_mutex;
    std::mutex m_peers_mutex;
};
using xstate_downloader_t = xtop_state_downloader;

}  // namespace state_sync
}  // namespace top