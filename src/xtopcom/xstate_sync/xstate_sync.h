// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xsimple_message.hpp"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xevm_common/trie/xtrie_sync.h"
#include "xstate_sync/xstate_sync_face.h"
#include "xvnetwork/xmessage.h"

namespace top {
namespace state_sync {

class xtop_state_sync : public xstate_sync_face_t {
public:
    xtop_state_sync() = default;
    ~xtop_state_sync() override = default;

    static std::shared_ptr<xtop_state_sync> new_state_sync(const common::xaccount_address_t & table,
                                                           const uint64_t height,
                                                           const xhash256_t & block_hash,
                                                           const xhash256_t & state_hash,
                                                           const xhash256_t & root_hash,
                                                           std::function<state_sync_peers_t()> peers,
                                                           std::function<void(const state_req &)> track_req,
                                                           base::xvdbstore_t * db,
                                                           bool sync_unit);

    void run() override;
    void cancel() override;
    bool is_done() const override;
    std::error_code error() const override;
    std::string symbol() const override;
    sync_result result() const override;
    void push_deliver_state(const single_state_detail & detail) override;
    void push_deliver_req(const state_req & req) override;

private:
    void wait() const;
    void sync_table(std::error_code & ec);
    void loop(std::error_code & ec);
    void assign_tasks(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network, const std::vector<common::xnode_address_t> & peers);
    void fill_tasks(uint32_t n, state_req & req, std::vector<xhash256_t> & nodes, std::vector<xbytes_t> & units);
    void process(state_req & req, std::error_code & ec);
    xhash256_t process_node_data(xbytes_t & blob, std::error_code & ec);
    xhash256_t process_unit_data(xbytes_t & blob, std::error_code & ec);
    void pop_deliver_req();
    void send_message(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                      const std::vector<common::xnode_address_t> & peers,
                      const xbytes_t & msg,
                      common::xmessage_id_t id);
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> available_network() const;
    std::vector<common::xnode_address_t> available_peers(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network) const;

    common::xaccount_address_t m_table;
    uint64_t m_height;
    xhash256_t m_table_block_hash;
    xhash256_t m_table_state_hash;
    xhash256_t m_root;
    std::string m_symbol;
    base::xvdbstore_t * m_db{nullptr};
    evm_common::trie::xkv_db_face_ptr_t m_kv_db{nullptr};
    std::shared_ptr<evm_common::trie::Sync> m_sched{nullptr};
    std::function<state_sync_peers_t()> m_peers_func{nullptr};
    std::function<void(const state_req &)> m_track_func{nullptr};

    std::map<xhash256_t, std::set<std::string>> m_trie_tasks;
    std::map<xhash256_t, std::pair<xbytes_t, std::set<std::string>>> m_unit_tasks;

    bool m_sync_table_finish{false};
    bool m_done{false};
    bool m_cancel{false};
    std::error_code m_ec;

    std::list<state_req> m_deliver_list;
    std::mutex m_deliver_mutex;
    uint32_t m_req_sequence_id{0};
};
using xstate_sync_t = xtop_state_sync;

}  // namespace state_sync
}  // namespace top