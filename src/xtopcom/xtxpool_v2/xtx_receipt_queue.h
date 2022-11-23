// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xdata/xgenesis_data.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxpool_v2/xtxpool_info.h"
#include "xvledger/xreceiptid.h"
#include "xvledger/xvaccount.h"
#include "xtxpool_v2/xunconfirm_id_height.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

class xreceipt_set_comp {
public:
    bool operator()(const std::shared_ptr<xtx_entry> left, const std::shared_ptr<xtx_entry> right) const {
        if (left->get_para().get_tx_type_score() == right->get_para().get_tx_type_score()) {
            if (left->get_para().get_timestamp() == right->get_para().get_timestamp()) {
                return left->get_para().get_charge_score() > right->get_para().get_charge_score();
            }
            return left->get_para().get_timestamp() < right->get_para().get_timestamp();
        }
        return left->get_para().get_tx_type_score() > right->get_para().get_tx_type_score();
    }
};

using xreceipt_set_t = std::multiset<std::shared_ptr<xtx_entry>, xreceipt_set_comp>;
using xreceipt_map_t = std::unordered_map<std::string, xreceipt_set_t::iterator>;

class xreceipt_queue_internal_t {
public:
    xreceipt_queue_internal_t(xtxpool_table_info_t * xtable_info) : m_xtable_info(xtable_info) {
    }
    void insert_tx(const std::shared_ptr<xtx_entry> & tx_ent);
    void erase_tx(const std::string & hash);
    const std::shared_ptr<xtx_entry> find(const std::string & hash_str) const;
    const xreceipt_set_t & get_queue() const {
        return m_tx_queue;
    }
    uint32_t size() const {
        return m_tx_queue.size();
    }
    bool recv_tx_full() const {
        return m_xtable_info->is_recv_tx_reached_upper_limit();
    }
    bool confirm_tx_full() const {
        return m_xtable_info->is_confirm_tx_reached_upper_limit();
    }
    const std::string & get_table_addr() const {
        return m_xtable_info->get_table_addr();
    }
    xtxpool_table_info_t * get_table_info() const {
        return m_xtable_info;
    }

private:
    xreceipt_set_t m_tx_queue;
    xreceipt_map_t m_tx_map;
    xtxpool_table_info_t * m_xtable_info;
};

class xpeer_table_receipts_t {
public:
    xpeer_table_receipts_t(xreceipt_queue_internal_t * receipt_queue_internal) : m_receipt_queue_internal(receipt_queue_internal) {
    }
    int32_t push_tx(const std::shared_ptr<xtx_entry> & tx_ent);
    void update_latest_receipt_id(uint64_t latest_receipt_id);
    const std::vector<xcons_transaction_ptr_t> get_txs(uint64_t lower_receipt_id, uint64_t upper_receipt_id, bool continuous) const;
    void erase(uint64_t receipt_id);
    bool empty() const {
        return m_txs.empty();
    }
    void get_lacking_ids(std::vector<uint64_t> & lacking_ids, uint64_t max_pull_id) const;
    // uint64_t get_latest_receipt_id() const {
    //     return m_latest_receipt_id;
    // }
    uint32_t size() const {
        return m_txs.size();
    }
    const std::shared_ptr<xtx_entry> get_tx_by_receipt_id(uint64_t receipt_id) const;

private:
    std::map<uint64_t, std::shared_ptr<xtx_entry>> m_txs;
    xreceipt_queue_internal_t * m_receipt_queue_internal;
    uint64_t m_latest_receipt_id{0};
};

using xtx_peer_table_map_t = std::map<base::xtable_shortid_t, std::shared_ptr<xpeer_table_receipts_t>>;

class xreceipt_queue_new_t {
public:
    xreceipt_queue_new_t(xtxpool_table_info_t * xtable_info, xtxpool_resources_face * para) : m_receipt_queue_internal(xtable_info), m_para(para) {
    }
    int32_t push_tx(const std::shared_ptr<xtx_entry> & tx_ent);
    const std::vector<xcons_transaction_ptr_t> get_txs(uint32_t confirm_and_recv_txs_max_num,
                                                       uint32_t confirm_txs_max_num,
                                                       const base::xreceiptid_state_ptr_t & receiptid_state,
                                                       const xunconfirm_id_height & unconfirm_id_height,
                                                       uint32_t & confirm_txs_num) const;                                              
    const std::shared_ptr<xtx_entry> pop_tx(const std::string & tx_hash, base::enum_transaction_subtype subtype);
    const std::shared_ptr<xtx_entry> find(const std::string & account_addr, const std::string & hash_str) const;
    void update_receiptid_state(const base::xreceiptid_state_ptr_t & receiptid_state);
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(const std::set<base::xtable_shortid_t> & all_table_sids, uint32_t & total_num) const;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_confirm_tx_ids(uint32_t & total_num) const;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_discrete_confirm_tx_ids(const std::map<base::xtable_shortid_t, xneed_confirm_ids> & need_confirm_ids_map,
                                                                                               uint32_t & total_num) const;
    void update_receipt_id_by_confirmed_tx(const tx_info_t & txinfo, base::xtable_shortid_t peer_table_sid, uint64_t receiptid);
    // uint64_t get_latest_recv_receipt_id(base::xtable_shortid_t peer_table_sid) const;
    // uint64_t get_latest_confirm_receipt_id(base::xtable_shortid_t peer_table_sid) const;
    uint32_t size() const;

private:
    xtx_peer_table_map_t & get_peer_table_map(bool is_recv_tx) {
        if (is_recv_tx) {
            return m_recv_tx_peer_table_map;
        } else {
            return m_confirm_tx_peer_table_map;
        }
    }

    xreceipt_queue_internal_t m_receipt_queue_internal;
    xtx_peer_table_map_t m_recv_tx_peer_table_map;
    xtx_peer_table_map_t m_confirm_tx_peer_table_map;
    xtxpool_resources_face * m_para;
};

}  // namespace xtxpool_v2
}  // namespace top
