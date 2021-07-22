// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xdata/xtable_bstate.h"
#include "xtxpool_v2/xnon_ready_account.h"
#include "xtxpool_v2/xtxmgr_table.h"
#include "xtxpool_v2/xtxpool_info.h"
#include "xtxpool_v2/xtxpool_resources_face.h"
#include "xtxpool_v2/xunconfirmed_tx_queue.h"
#include "xtxpool_v2/xreceipt_state_cache.h"

#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace top {
namespace xtxpool_v2 {

// TODO(jimmy) should delete later
class xaccount_basic_info_t {
 public:
    xaccount_basic_info_t() = default;
   //  xaccount_basic_info_t(const xblock_ptr_t & block, const base::xaccount_index_t & index)
   //  : m_latest_block(block), m_account_index(index) {}

 public:
   //  void    set_latest_block(const xblock_ptr_t & block) {m_latest_block = block;}
    void    set_account_index(const base::xaccount_index_t & index) {m_account_index = index;}
    void    set_latest_state(const xaccount_ptr_t & state) {m_latest_state = state;}
    void    set_sync_height_start(uint64_t height) {m_sync_height_start = height;}
    void    set_sync_num(uint32_t num) {m_sync_num = num;}

   //  const xblock_ptr_t &            get_latest_block() const {return m_latest_block;}
    const xaccount_ptr_t &          get_latest_state() const {return m_latest_state;}
    const base::xaccount_index_t &  get_accout_index() const {return m_account_index;}
    const uint64_t                  get_sync_height_start() const {return m_sync_height_start;}
    const uint32_t                  get_sync_num() const {return m_sync_num;}

 private:
   //  xblock_ptr_t            m_latest_block{nullptr};
    xaccount_ptr_t          m_latest_state{nullptr};
    base::xaccount_index_t  m_account_index;
    uint64_t                m_sync_height_start{0};
    uint32_t                m_sync_num{0};
};


#define table_unconfirm_txs_num_max (100)

class xtxpool_table_t {
public:
    xtxpool_table_t(xtxpool_resources_face * para, std::string table_addr, xtxpool_shard_info_t * shard, xtxpool_statistic_t * statistic)
      : m_para(para)
      , m_xtable_info(table_addr, shard, statistic)
      , m_txmgr_table(&m_xtable_info)
      // , m_table_filter(para->get_vblockstore())
      , m_unconfirmed_tx_queue(para, &m_xtable_info)
      , m_table_state_cache(para, table_addr) {
    }
    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx);
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send);
    std::shared_ptr<xtx_entry> pop_tx(const tx_info_t & txinfo, bool clear_follower);
    ready_accounts_t get_ready_accounts(const xtxs_pack_para_t & pack_para);
    std::vector<xcons_transaction_ptr_t> get_ready_txs(const xtxs_pack_para_t & pack_para);
    const std::shared_ptr<xtx_entry> query_tx(const std::string & account, const uint256_t & hash);
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce);
    const std::vector<xcons_transaction_ptr_t> get_resend_txs(uint64_t now);
    void on_block_confirmed(xblock_t * table_block);
    int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs);
    void refresh_table(bool refresh_unconfirm_txs);
    // void update_non_ready_accounts();
    void update_table_state(const data::xtablestate_ptr_t & table_state);
    xcons_transaction_ptr_t get_unconfirmed_tx(const std::string & to_table_addr, uint64_t receipt_id) const;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(uint32_t max_num) const;
    const std::vector<xtxpool_table_lacking_confirm_tx_hashs_t> get_lacking_confirm_tx_hashs(uint32_t max_num) const;
    bool need_sync_lacking_receipts() const;
    void add_shard(xtxpool_shard_info_t * shard);
    void remove_shard(xtxpool_shard_info_t * shard);
    bool no_shard() const;
    bool is_consensused_recv_receiptid(const std::string & from_addr, uint64_t receipt_id) const;
    bool is_consensused_confirm_receiptid(const std::string & to_addr, uint64_t receipt_id) const;

private:
    bool is_account_need_update(const std::string & account_addr) const;
    int32_t verify_tx_common(const xcons_transaction_ptr_t & tx) const;
    int32_t verify_send_tx(const xcons_transaction_ptr_t & tx) const;
    int32_t verify_receipt_tx(const xcons_transaction_ptr_t & tx) const;
    int32_t verify_cons_tx(const xcons_transaction_ptr_t & tx) const;
    bool get_account_latest_nonce(const std::string account_addr, uint64_t & latest_nonce) const;
    bool  get_account_basic_info(const std::string & account, xaccount_basic_info_t & account_index_info) const;
    void update_id_state(const tx_info_t & txinfo, base::xtable_shortid_t peer_table_sid, uint64_t receiptid, uint64_t nonce);
    bool is_reach_limit(const std::shared_ptr<xtx_entry> & tx) const;
    int32_t push_send_tx_real(const std::shared_ptr<xtx_entry> & tx);
    int32_t push_receipt_real(const std::shared_ptr<xtx_entry> & tx);

    xtxpool_resources_face * m_para;
    xtxpool_table_info_t m_xtable_info;
    xtxmgr_table_t m_txmgr_table;
    xunconfirmed_tx_queue_t m_unconfirmed_tx_queue;
    // xnon_ready_accounts_t m_non_ready_accounts;
    mutable std::mutex m_mgr_mutex;  // lock m_txmgr_table
    mutable std::mutex m_unconfirm_mutex;  // lock m_unconfirmed_tx_queue
    // mutable std::mutex m_non_ready_mutex;  // lock m_non_ready_accounts
    xtable_state_cache_t m_table_state_cache;
    uint64_t m_unconfirmed_tx_num{0};
};

}  // namespace xtxpool_v2
}  // namespace top
