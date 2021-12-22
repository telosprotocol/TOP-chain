// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xdata/xtable_bstate.h"
#include "xtxpool_v2/xnon_ready_account.h"
#include "xtxpool_v2/xreceipt_state_cache.h"
#include "xtxpool_v2/xtxmgr_table.h"
#include "xtxpool_v2/xtxpool_info.h"
#include "xtxpool_v2/xtxpool_resources_face.h"
#include "xtxpool_v2/xunconfirm_id_height.h"

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
    void set_account_index(const base::xaccount_index_t & index) {
        m_account_index = index;
    }
    void set_latest_state(const xaccount_ptr_t & state) {
        m_latest_state = state;
    }
    void set_sync_height_start(uint64_t height) {
        m_sync_height_start = height;
    }
    void set_sync_num(uint32_t num) {
        m_sync_num = num;
    }

    //  const xblock_ptr_t &            get_latest_block() const {return m_latest_block;}
    const xaccount_ptr_t & get_latest_state() const {
        return m_latest_state;
    }
    const base::xaccount_index_t & get_accout_index() const {
        return m_account_index;
    }
    const uint64_t get_sync_height_start() const {
        return m_sync_height_start;
    }
    const uint32_t get_sync_num() const {
        return m_sync_num;
    }

private:
    //  xblock_ptr_t            m_latest_block{nullptr};
    xaccount_ptr_t m_latest_state{nullptr};
    base::xaccount_index_t m_account_index;
    uint64_t m_sync_height_start{0};
    uint32_t m_sync_num{0};
};

#define table_unconfirm_txs_num_max (100)

struct update_id_state_para {
    update_id_state_para(const tx_info_t & txinfo, base::xtable_shortid_t peer_table_sid, uint64_t receiptid, uint64_t nonce)
      : m_txinfo(txinfo), m_peer_table_sid(peer_table_sid), m_receiptid(receiptid), m_nonce(nonce) {
    }
    tx_info_t m_txinfo;
    base::xtable_shortid_t m_peer_table_sid;
    uint64_t m_receiptid;
    uint64_t m_nonce;
};

class xtxpool_table_t {
public:
    xtxpool_table_t(xtxpool_resources_face * para,
                    std::string table_addr,
                    xtxpool_role_info_t * shard,
                    xtxpool_statistic_t * statistic,
                    std::set<base::xtable_shortid_t> * all_sid_set = nullptr)
      : m_para(para)
      , m_table_state_cache(para, table_addr)
      , m_xtable_info(table_addr, shard, statistic, &m_table_state_cache, all_sid_set)
      , m_txmgr_table(&m_xtable_info, para)
      , m_unconfirm_id_height(m_xtable_info.get_short_table_id()) {
    }
    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx);
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send);
    std::shared_ptr<xtx_entry> pop_tx(const tx_info_t & txinfo, bool clear_follower);
    ready_accounts_t get_ready_accounts(const xtxs_pack_para_t & pack_para);
    std::vector<xcons_transaction_ptr_t> get_ready_txs(const xtxs_pack_para_t & pack_para);
    const std::shared_ptr<xtx_entry> query_tx(const std::string & account, const uint256_t & hash);
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce);
    void on_block_confirmed(xblock_t * table_block);
    int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs);
    void refresh_table();
    // void update_non_ready_accounts();
    void update_table_state(const data::xtablestate_ptr_t & table_state);
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(uint32_t & total_num) const;
    bool need_sync_lacking_receipts() const;
    void add_role(xtxpool_role_info_t * shard);
    void remove_role(xtxpool_role_info_t * shard);
    bool no_role() const;

    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_confirm_tx_ids(uint32_t & total_num) const;
    void build_recv_tx(base::xtable_shortid_t peer_table_sid, std::vector<uint64_t> receiptids, std::vector<xcons_transaction_ptr_t> & receipts);
    void build_confirm_tx(base::xtable_shortid_t peer_table_sid, std::vector<uint64_t> receiptids, std::vector<xcons_transaction_ptr_t> & receipts);
    base::xtable_shortid_t table_sid() {
        return m_xtable_info.get_short_table_id();
    }
    void unconfirm_cache_status(uint32_t & sender_cache_size, uint32_t & receiver_cache_size, uint32_t & height_record_size) const;

private:
    bool is_account_need_update(const std::string & account_addr) const;
    int32_t verify_tx_common(const xcons_transaction_ptr_t & tx) const;
    int32_t verify_send_tx(const xcons_transaction_ptr_t & tx) const;
    int32_t verify_receipt_tx(const xcons_transaction_ptr_t & tx) const;
    int32_t verify_cons_tx(const xcons_transaction_ptr_t & tx) const;
    bool get_account_latest_nonce(const std::string account_addr, uint64_t & latest_nonce) const;
    void update_id_state(const std::vector<update_id_state_para> & para_vec);
    bool is_reach_limit(const std::shared_ptr<xtx_entry> & tx) const;
    int32_t push_send_tx_real(const std::shared_ptr<xtx_entry> & tx);
    int32_t push_receipt_real(const std::shared_ptr<xtx_entry> & tx);
    void deal_commit_table_block(xblock_t * table_block, bool update_txmgr);
    xcons_transaction_ptr_t build_receipt(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, uint64_t commit_height, enum_transaction_subtype subtype);

    xtxpool_resources_face * m_para;
    xtable_state_cache_t m_table_state_cache;
    xtxpool_table_info_t m_xtable_info;
    xtxmgr_table_t m_txmgr_table;
    mutable std::mutex m_mgr_mutex;  // lock m_txmgr_table

    xunconfirm_id_height m_unconfirm_id_height;

    // xnon_ready_accounts_t m_non_ready_accounts;
    // mutable std::mutex m_non_ready_mutex;  // lock m_non_ready_accounts
};

}  // namespace xtxpool_v2
}  // namespace top
