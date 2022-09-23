// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_kv_db.h"
#include "xevm_common/trie/xtrie_sync.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvnetwork_driver_face.h"

namespace top {
namespace state_sync {

XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_request, 0x01);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_response, 0x02);

struct trie_task {
    std::vector<xbytes_t> path;
    std::set<std::string> attemps;

    trie_task() = default;
    trie_task(std::vector<xbytes_t> & path_) : path(path_) {
    }
};

struct unit_task {
    std::set<std::string> attemps;
};

struct state_req {
    uint32_t id;
    uint32_t n_items{0};
    std::map<xhash256_t, trie_task> trie_tasks;
    std::map<xhash256_t, unit_task> unit_tasks;
    uint64_t start{0};
    uint64_t delivered{0};
    std::vector<xbytes_t> nodes_response;
    std::vector<xbytes_t> units_response;
};

struct state_res {
    uint32_t id;
    common::xaccount_address_t table;
    std::vector<xbytes_t> nodes;
    std::vector<xbytes_t> units;
};

using state_sync_peers_t = std::vector<std::shared_ptr<vnetwork::xvnetwork_driver_face_t>>;

class xtop_state_sync {
public:
    xtop_state_sync() = default;
    ~xtop_state_sync() = default;

    static std::shared_ptr<xtop_state_sync> new_state_sync(const common::xaccount_address_t & table,
                                                           const xhash256_t & root,
                                                           std::function<state_sync_peers_t()> peers,
                                                           std::function<void(const state_req &)> track_req,
                                                           base::xvdbstore_t * db,
                                                           bool sync_unit);

    void run();
    void cancel();
    bool is_done() const;
    std::error_code error() const;
    common::xaccount_address_t table() const;
    xhash256_t root() const;
    void push_deliver_req(const state_req & req);

private:
    void wait() const;
    void loop(std::error_code & ec);
    void assign_tasks(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network);
    void fill_tasks(uint32_t n, state_req & req, std::vector<xhash256_t> & nodes, std::vector<xhash256_t> & codes);
    void commit(bool force, std::error_code & ec);
    void process(state_req & req, std::error_code & ec);
    xhash256_t process_node_data(xbytes_t & blob, std::error_code & ec);
    xhash256_t process_unit_data(xbytes_t & blob, std::error_code & ec);
    void pop_deliver_req();
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> available_network() const;

    common::xaccount_address_t m_table;
    xhash256_t m_root;
    std::shared_ptr<evm_common::trie::xkv_db_face_t> m_db;
    std::shared_ptr<evm_common::trie::Sync> m_sched;
    std::function<state_sync_peers_t()> m_peers_func{nullptr};
    std::function<void(const state_req &)> m_track_func{nullptr};

    std::map<xhash256_t, trie_task> m_trie_tasks;
    std::map<xhash256_t, unit_task> m_unit_tasks;

    bool m_started{false};
    bool m_done{false};
    bool m_cancel{false};
    std::error_code m_ec;

    std::list<state_req> m_deliver_list;
    std::mutex m_deliver_mutex;
    uint32_t m_req_sequence_id{0};
    uint32_t m_num_uncommitted{0};
    uint32_t m_bytes_uncommitted{0};
};
using xstate_sync_t = xtop_state_sync;

}  // namespace state_sync
}  // namespace top