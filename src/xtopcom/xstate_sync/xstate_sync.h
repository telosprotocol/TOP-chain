// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xsimple_message.hpp"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xevm_common/trie/xtrie_sync.h"
#include "xstate_sync/xstate_sync_face.h"
#include "xvnetwork/xmessage.h"

#include <atomic>
#if !defined(NDEBUG)
#include <thread>
#endif

namespace top {
namespace state_sync {

class xtop_state_sync : public xstate_sync_face_t {
private:
#if !defined(NDEBUG)
    std::thread::id running_thead_id_;
#endif

    common::xaccount_address_t m_table;
    uint64_t m_height;
    evm_common::xh256_t m_table_block_hash;
    evm_common::xh256_t m_table_state_hash;
    evm_common::xh256_t m_root;
    std::string m_symbol;
    base::xvdbstore_t * m_db{nullptr};
    evm_common::trie::xkv_db_face_ptr_t m_kv_db{nullptr};
    std::shared_ptr<evm_common::trie::Sync> m_sched{nullptr};
    std::function<sync_peers(const common::xtable_id_t & id)> m_peers_func{nullptr};
    std::function<void(const state_req &)> m_track_func{nullptr};

    std::set<evm_common::xh256_t> m_trie_tasks;
    std::map<evm_common::xh256_t, xbytes_t> m_unit_tasks;

    std::atomic<bool> m_sync_table_finish{false};
    std::atomic<bool> m_done{false};
    std::atomic<bool> m_cancel{false};
    std::error_code m_ec;

    std::queue<state_req> m_deliver_list;
    std::condition_variable m_condition;
    std::mutex m_mutex;
    uint32_t m_items_per_task{0};
    uint32_t m_req_sequence_id{0};
    uint32_t m_unit_bytes_uncommitted{0};

public:
    xtop_state_sync();
    ~xtop_state_sync() override;

    static std::shared_ptr<xtop_state_sync> new_state_sync(const common::xaccount_address_t & table,
                                                           const uint64_t height,
                                                           const evm_common::xh256_t & block_hash,
                                                           const evm_common::xh256_t & state_hash,
                                                           const evm_common::xh256_t & root_hash,
                                                           std::function<sync_peers(const common::xtable_id_t & id)> peers,
                                                           std::function<void(const state_req &)> track_req,
                                                           base::xvdbstore_t * db,
                                                           bool sync_unit);

    void run() override;
    void cancel() override;
    bool is_done() const override;
    std::error_code error() const override;
    std::string symbol() const override;
    sync_result result() const override;
    void deliver_req(const state_req & req) override;

private:
    void wait() const;
    void sync_table(std::error_code & ec);
    void sync_trie(std::error_code & ec);
    void loop(std::function<bool()> condition, std::function<void(sync_peers const &)> add_task, std::function<void(state_req &, std::error_code &)> process, std::error_code & ec);
    void assign_table_tasks(const sync_peers & sync_peers);
    void assign_trie_tasks(const sync_peers & sync_peers);
    void fill_tasks(uint32_t n, state_req & req, std::vector<evm_common::xh256_t> & nodes, std::vector<xbytes_t> & units);
    void process_table(state_req & req, std::error_code & ec);
    void process_trie(state_req & req, std::error_code & ec);
    evm_common::xh256_t process_node_data(const xbytes_t & blob, std::error_code & ec);
    evm_common::xh256_t process_unit_data(const xbytes_t & blob, std::error_code & ec);
    common::xnode_address_t send_message(const sync_peers & sync_peers, const xbytes_t & msg, common::xmessage_id_t id);
};
using xstate_sync_t = xtop_state_sync;

}  // namespace state_sync
}  // namespace top