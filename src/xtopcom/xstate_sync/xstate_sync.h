// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_sync.h"
#include "xstate_mpt/xstate_mpt_db.h"
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
    uint32_t n_items{0};
    std::map<xhash256_t, trie_task> trie_tasks;
    std::map<xhash256_t, unit_task> unit_tasks;
    uint64_t start{0};
    uint64_t delivered{0};
    std::vector<xbytes_t> response;
};

struct state_res {
    std::string table;
    uint32_t id;
    std::vector<xbytes_t> states;
};

class xtop_state_sync {
public:
    xtop_state_sync() = default;
    ~xtop_state_sync() = default;

    void run_state_sync(const std::string & table, const xhash256_t & root, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, base::xvdbstore_t * db);
    void run(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network);
    void wait();
    void cancel();
    void loop(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, std::error_code & ec);

    void commit(bool force, std::error_code & ec);

    void assign_tasks(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network);
    void fill_tasks(uint32_t n, state_req & req, std::vector<xhash256_t> & nodes, std::vector<xhash256_t> & codes);
    void process(state_req & req, std::error_code & ec);
    xhash256_t process_node_data(xbytes_t & blob, std::error_code & ec);
    void deliver_node_data(top::vnetwork::xvnode_address_t const & sender, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, top::vnetwork::xmessage_t const & message, base::xvdbstore_t * db);
    void clear();

    static std::shared_ptr<xtop_state_sync> new_state_sync(const std::string & table,
                                                           const xhash256_t & root,
                                                           std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                                                           std::shared_ptr<std::list<state_req>> track_req,
                                                           base::xvdbstore_t * db);
    void run2();
    xhash256_t root() const;
    bool is_done() const;
    void push_deliver_req(const state_req & req);

private:
    std::string m_table;
    xhash256_t m_root;
    std::shared_ptr<state_mpt::xstate_mpt_db_t> m_db;
    std::shared_ptr<evm_common::trie::Sync> m_sched;

    std::map<xhash256_t, trie_task> m_trie_tasks;
    std::map<xhash256_t, unit_task> m_unit_tasks;

    uint32_t m_num_uncommitted{0};
    uint32_t m_bytes_uncommitted{0};

    bool m_started{false};
    bool m_done{false};
    bool m_cancel{false};
    bool m_running{false};

    std::map<uint32_t, state_req, std::less<uint32_t>> m_active_req;
    uint32_t m_req_sequence_id;

    std::list<state_res> m_res_list;
    std::list<state_req> m_deliver_list;

    std::error_code m_ec;

    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_network_ptr{nullptr};
    std::shared_ptr<std::list<state_req>> m_track_list_ptr{nullptr};
};
using xstate_sync_t = xtop_state_sync;

}  // namespace state_sync
}  // namespace top