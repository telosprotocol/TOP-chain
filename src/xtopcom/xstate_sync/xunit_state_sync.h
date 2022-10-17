// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xsimple_message.hpp"
#include "xevm_common/trie/xtrie_kv_db.h"
#include "xevm_common/trie/xtrie_sync.h"
#include "xstate_sync/xstate_sync_face.h"
#include "xstatestore/xstatestore_face.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvnetwork_driver_face.h"

#include <atomic>

namespace top {
namespace state_sync {

class xtop_unit_state_sync : public xstate_sync_face_t {
public:
    xtop_unit_state_sync() = default;
    ~xtop_unit_state_sync() override = default;

    static std::shared_ptr<xtop_unit_state_sync> new_state_sync(const common::xaccount_address_t & account,
                                                                const base::xaccount_index_t & index,
                                                                std::function<sync_peers(const common::xtable_id_t & id)> peers,
                                                                std::function<void(const state_req &)> track_req,
                                                                base::xvdbstore_t * db,
                                                                statestore::xstatestore_face_t * store);

    void run() override;
    void cancel() override;
    bool is_done() const override;
    std::error_code error() const override;
    std::string symbol() const override;
    sync_result result() const override;
    void deliver_req(const state_req & req) override;

private:
    void wait() const;
    void sync_unit(std::error_code & ec);
    void assign_unit_tasks(const sync_peers & peers);
    void process_unit(state_req & req, std::error_code & ec);
    void loop(std::function<bool()> condition,
              std::function<void(sync_peers const &)> add_task,
              std::function<void(state_req &, std::error_code &)> process_task,
              std::error_code & ec);
    common::xnode_address_t send_message(const sync_peers & peers, const xbytes_t & msg, common::xmessage_id_t id);

    common::xaccount_address_t m_account;
    base::xaccount_index_t m_index;
    std::string m_symbol;
    base::xvdbstore_t * m_db{nullptr};
    statestore::xstatestore_face_t * m_store{nullptr};
    std::function<sync_peers(const common::xtable_id_t & id)> m_peers_func{nullptr};
    std::function<void(const state_req &)> m_track_func{nullptr};

    std::atomic<bool> m_done{false};
    std::atomic<bool> m_cancel{false};
    std::atomic<bool> m_sync_unit_finish{false};
    std::error_code m_ec;

    std::list<state_req> m_deliver_list;
    std::condition_variable m_condition;
    std::mutex m_mutex;

    uint32_t m_req_sequence_id{0};
};
using xunit_state_sync_t = xtop_unit_state_sync;

}  // namespace state_sync
}  // namespace top