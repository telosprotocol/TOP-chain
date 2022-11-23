// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
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
    xtxmgr_table_t(xtxpool_table_info_t * xtable_info, xtxpool_resources_face * para)
      : m_xtable_info(xtable_info), m_send_tx_queue(xtable_info), m_new_receipt_queue(xtable_info, para) {
    }

    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx, uint64_t latest_nonce);
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx);
    data::xcons_transaction_ptr_t pop_tx(const std::string & tx_hash, base::enum_transaction_subtype subtype, bool clear_follower);
    void update_id_state(const tx_info_t & txinfo, base::xtable_shortid_t table_sid, uint64_t receiptid, uint64_t nonce);
    std::vector<xcons_transaction_ptr_t> get_ready_txs(const xtxs_pack_para_t & pack_para, const xunconfirm_id_height & unconfirm_id_height);
    data::xcons_transaction_ptr_t query_tx(const std::string & account_addr, const std::string & hash_str) const;
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce);
    bool is_repeat_tx(const std::shared_ptr<xtx_entry> & tx) const;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(const std::set<base::xtable_shortid_t> & all_table_sids, uint32_t & total_num) const;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_confirm_tx_ids(uint32_t & total_num) const;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_discrete_confirm_tx_ids(const std::map<base::xtable_shortid_t, xneed_confirm_ids> & need_confirm_ids_map,
                                                                                               uint32_t & total_num) const;
    void clear_expired_txs();
    void update_receiptid_state(const base::xreceiptid_state_ptr_t & receiptid_state);
    uint32_t get_tx_cache_size() const;

private:
    xtxpool_table_info_t * m_xtable_info;
    xsend_tx_queue_t m_send_tx_queue;
    xreceipt_queue_new_t m_new_receipt_queue;
};

}  // namespace xtxpool_v2
}  // namespace top
