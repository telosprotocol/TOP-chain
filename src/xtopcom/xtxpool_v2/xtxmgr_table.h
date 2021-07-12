// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xtxpool_v2/xpending_account.h"
#include "xtxpool_v2/xtx_queue.h"
#include "xtxpool_v2/xtx_receipt_queue.h"
#include "xtxpool_v2/xtxpool_face.h"

#include <set>
#include <string>

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

class xtxmgr_table_t {
public:
    xtxmgr_table_t(xtxpool_table_info_t * xtable_info)
      : m_xtable_info(xtable_info), m_send_tx_queue(xtable_info), m_new_receipt_queue(xtable_info), m_pending_accounts(xtable_info) {
    }

    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx, uint64_t latest_nonce);
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx);
    std::shared_ptr<xtx_entry> pop_tx(const tx_info_t & txinfo, bool clear_follower);
    void update_id_state(const tx_info_t & txinfo, base::xtable_shortid_t table_sid, uint64_t receiptid, uint64_t nonce);
    ready_accounts_t get_ready_accounts(const xtxs_pack_para_t & pack_para);
    std::vector<xcons_transaction_ptr_t> get_ready_txs(const xtxs_pack_para_t & pack_para);
    const std::shared_ptr<xtx_entry> query_tx(const std::string & account_addr, const uint256_t & hash) const;
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce);
    bool is_account_need_update(const std::string & account_addr) const;
    void update_receiptid_state(const base::xreceiptid_state_ptr_t & receiptid_state);
    bool is_repeat_tx(const std::shared_ptr<xtx_entry> & tx) const;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(uint32_t max_num) const;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_confirm_tx_ids(uint32_t max_num) const;
    void clear_expired_txs();
    uint64_t get_latest_recv_receipt_id(base::xtable_shortid_t peer_table_sid) const;
    uint64_t get_latest_confirm_receipt_id(base::xtable_shortid_t peer_table_sid) const;
    bool get_account_nonce_cache(const std::string & account_addr, uint64_t & latest_nonce) const;

private:
    void queue_to_pending();
    void send_tx_queue_to_pending();

    xtxpool_table_info_t * m_xtable_info;
    xsend_tx_queue_t m_send_tx_queue;
    xreceipt_queue_new_t m_new_receipt_queue;
    xpending_accounts_t m_pending_accounts;
};

}  // namespace xtxpool_v2
}  // namespace top
