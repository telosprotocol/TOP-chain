// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xtxpool_v2/xnon_ready_account.h"
#include "xtxpool_v2/xreceipt_state_cache.h"
#include "xtxpool_v2/xtxmgr_table.h"
#include "xtxpool_v2/xtxpool_info.h"
#include "xtxpool_v2/xtxpool_resources_face.h"
#include "xtxpool_v2/xunconfirm_id_height.h"
#include "xtxpool_v2/xunconfirm_raw_txs.h"
#include "xtxpool_v2/xuncommit_txs.h"

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
    void set_latest_state(const data::xunitstate_ptr_t & state) {
        m_latest_state = state;
    }
    void set_sync_height_start(uint64_t height) {
        m_sync_height_start = height;
    }
    void set_sync_num(uint32_t num) {
        m_sync_num = num;
    }

    //  const xblock_ptr_t &            get_latest_block() const {return m_latest_block;}
    const data::xunitstate_ptr_t & get_latest_state() const {
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
    data::xunitstate_ptr_t m_latest_state{nullptr};
    base::xaccount_index_t m_account_index;
    uint64_t m_sync_height_start{0};
    uint32_t m_sync_num{0};
};

#define table_unconfirm_txs_num_max (100)

struct update_id_state_para {
    update_id_state_para(const tx_info_t & txinfo, base::xtable_shortid_t peer_table_sid, uint64_t receiptid)
      : m_txinfo(txinfo), m_peer_table_sid(peer_table_sid), m_receiptid(receiptid) {
    }
    tx_info_t m_txinfo;
    base::xtable_shortid_t m_peer_table_sid;
    uint64_t m_receiptid;
};

class xtxpool_table_t {
public:
    xtxpool_table_t(xtxpool_resources_face * para,
                    std::string table_addr,
                    xtxpool_role_info_t * shard,
                    xtxpool_statistic_t * statistic,
                    std::set<base::xtable_shortid_t> * all_sid_set = nullptr)
      : m_table_address(table_addr)
      , m_para(para)
      , m_table_state_cache(para, table_addr)
      , m_xtable_info(table_addr, shard, statistic, &m_table_state_cache, all_sid_set)
      , m_txmgr_table(&m_xtable_info, para)
      , m_unconfirm_id_height(m_xtable_info.get_short_table_id())
      , m_unconfirm_raw_txs(m_xtable_info.get_short_table_id())
      , m_uncommit_txs(table_addr)
      , m_push_send_tx_metrics_name("txpool_push_send_tx_" + table_addr) {
    }
    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx);
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send);
    xcons_transaction_ptr_t pop_tx(const tx_info_t & txinfo, bool clear_follower);
    xpack_resource get_pack_resource(const xtxs_pack_para_t & pack_para);
    xcons_transaction_ptr_t query_tx(const uint256_t & hash);
    xcons_transaction_ptr_t query_tx(const std::string & hash_str);
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce);
    void on_block_confirmed(data::xblock_t * table_block);
    bool on_block_confirmed(base::enum_xvblock_class blk_class, uint64_t height);
    int32_t verify_txs(const std::vector<xcons_transaction_ptr_t> & txs);
    void refresh_table();
    // void update_non_ready_accounts();
    void update_table_state(const data::xtablestate_ptr_t & table_state);
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(uint32_t & total_num) const;
    void add_role(xtxpool_role_info_t * shard);
    void remove_role(xtxpool_role_info_t * shard);
    bool no_role() const;

    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_confirm_tx_ids(uint32_t & total_num) const;
    void build_recv_tx(base::xtable_shortid_t peer_table_sid, std::vector<uint64_t> receiptids, std::vector<xcons_transaction_ptr_t> & receipts);
    void build_confirm_tx(base::xtable_shortid_t peer_table_sid, std::vector<uint64_t> receiptids, std::vector<xcons_transaction_ptr_t> & receipts);
    base::xtable_shortid_t table_sid() {
        return m_xtable_info.get_short_table_id();
    }
    void unconfirm_cache_status(uint32_t & sender_cache_size, uint32_t & receiver_cache_size, uint32_t & height_record_size, uint32_t & unconfirm_raw_txs_size) const;
    data::xtransaction_ptr_t get_raw_tx(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id) const;

    void get_min_keep_height(std::string & table_addr, uint64_t & height) const;

    void update_uncommit_txs(base::xvblock_t * _lock_block, base::xvblock_t * _cert_block);
    uint32_t get_tx_cache_size() const;    

private:
    // bool is_account_need_update(const std::string & account_addr) const;
    int32_t verify_tx_common(const xcons_transaction_ptr_t & tx) const;
    int32_t verify_send_tx(const xcons_transaction_ptr_t & tx, bool is_first_time_push_tx) const;
    void filter_txs_by_black_white_list(std::vector<xcons_transaction_ptr_t> & txs);
    int32_t verify_receipt_tx(const xcons_transaction_ptr_t & tx) const;
    int32_t verify_cons_tx(const xcons_transaction_ptr_t & tx) const;
    bool get_account_latest_nonce(const std::string account_addr, uint64_t & latest_nonce);
    void update_id_state(const std::vector<update_id_state_para> & para_vec);
    bool is_reach_limit(const std::shared_ptr<xtx_entry> & tx) const;
    int32_t push_send_tx_real(const std::shared_ptr<xtx_entry> & tx, uint64_t latest_nonce);
    int32_t push_receipt_real(const std::shared_ptr<xtx_entry> & tx);
    void deal_commit_table_block(data::xblock_t * table_block, bool update_txmgr);
    xcons_transaction_ptr_t build_receipt(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, uint64_t commit_height, base::enum_transaction_subtype subtype);
    void move_uncommit_txs(base::xvblock_t * block);
    int32_t check_send_tx_nonce(const std::shared_ptr<xtx_entry> & tx, uint64_t & latest_nonce);

    common::xaccount_address_t m_table_address;
    xtxpool_resources_face * m_para;
    xtable_state_cache_t m_table_state_cache;
    xtxpool_table_info_t m_xtable_info;
    xtxmgr_table_t m_txmgr_table;
    mutable std::mutex m_mgr_mutex;        // lock m_txmgr_table

    xunconfirm_id_height m_unconfirm_id_height;
    xunconfirm_raw_txs m_unconfirm_raw_txs;

    xuncommit_txs_t m_uncommit_txs;
    // for test
    std::string m_push_send_tx_metrics_name;

    std::atomic<uint64_t> m_latest_commit_height{0};

    // xnon_ready_accounts_t m_non_ready_accounts;
    // mutable std::mutex m_non_ready_mutex;  // lock m_non_ready_accounts
};

}  // namespace xtxpool_v2
}  // namespace top
