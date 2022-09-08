// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_sync.h"
#include "xstate_mpt/src/xstate_mpt_db.cpp"

namespace top {
namespace state_sync {

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
    uint32_t n_items{0};
    std::map<xhash256_t, trie_task> trie_tasks;
    std::map<xhash256_t, unit_task> unit_tasks;
    uint64_t timeout{0};
    uint64_t delivered{0};
    std::vector<xbytes_t> response;
};

struct state_res {
    uint32_t id;
    std::vector<xbytes_t> states;
};

class xtop_state_sync {
public:
    xtop_state_sync(const std::string & table, const xhash256_t & root, base::xvdbstore_t * db);
    ~xtop_state_sync() = default;

    void run_state_sync();
    void run();
    void wait();
    void cancel();
    void loop(std::error_code & ec);

    void commit(bool force, std::error_code & ec);

    void assign_tasks();
    void fill_tasks(uint32_t n, state_req & req, std::vector<xhash256_t> & nodes, std::vector<xhash256_t> & codes);
    void process(state_req & req, std::error_code & ec);
    xhash256_t process_node_data(xbytes_t & blob, std::error_code & ec);


    // d *Downloader // Downloader instance to access and manage current peerset

    std::shared_ptr<state_mpt::xstate_mpt_db_t> m_db;
    std::shared_ptr<evm_common::trie::Sync> m_sched;
    // keccak hash.Hash  // Keccak256 hasher to verify deliveries with

    std::map<xhash256_t, trie_task> m_trie_tasks;
    std::map<xhash256_t, unit_task> m_unit_tasks;

    uint32_t m_num_uncommitted{0};
    uint32_t m_bytes_uncommitted{0};

    // deliver    chan *stateReq // Delivery channel multiplexing peer responses
    // cancelOnce sync.Once      // Ensures cancel only ever gets called once
    bool m_started{false};
    bool m_done{false};
    bool m_cancel{false};

    std::map<uint32_t, state_req> m_active_req;
    uint32_t m_req_sequence_id;

    std::list<state_res> m_res_list;
    std::list<state_req> m_deliver_list;


    std::error_code m_ec;

    xhash256_t m_root;
};
using xstate_sync_t = xtop_state_sync;

}  // namespace state_sync
}  // namespace top