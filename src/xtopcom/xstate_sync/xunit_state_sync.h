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

namespace top {
namespace state_sync {

class xtop_unit_state_sync : public xstate_sync_face_t {
public:
    xtop_unit_state_sync() = default;
    ~xtop_unit_state_sync() override = default;

    static std::shared_ptr<xtop_unit_state_sync> new_state_sync(const common::xaccount_address_t & account,
                                                                const base::xaccount_index_t & index,
                                                                std::function<state_sync_peers_t()> peers,
                                                                base::xvdbstore_t * db,
                                                                statestore::xstatestore_face_t * store);

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
    void sync_unit(std::error_code & ec);
    void send_message(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network,
                      const std::vector<common::xnode_address_t> & peers,
                      const xbytes_t & msg,
                      common::xmessage_id_t id);
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> available_network() const;
    std::vector<common::xnode_address_t> available_peers(std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network) const;

    common::xaccount_address_t m_account;
    base::xaccount_index_t m_index;
    base::xvdbstore_t * m_db{nullptr};
    statestore::xstatestore_face_t * m_store{nullptr};
    std::function<state_sync_peers_t()> m_peers_func{nullptr};

    bool m_done{false};
    bool m_cancel{false};
    std::error_code m_ec;
};
using xunit_state_sync_t = xtop_unit_state_sync;

}  // namespace state_sync
}  // namespace top