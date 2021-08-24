// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xtxpool_v2/xreceipt_state_cache.h"
#include "xtxpool_v2/xtxpool_info.h"
#include "xtxpool_v2/xtxpool_resources_face.h"

#include <atomic>
#include <string>

NS_BEG2(top, xtxpool_v2)

class xunconfirmed_tx_comp {
public:
    bool operator()(const xcons_transaction_ptr_t left, const xcons_transaction_ptr_t right) const {
        return left->get_receipt_gmtime() < right->get_receipt_gmtime();
    }
};

using xall_unconfirm_tx_set_t = std::multiset<xcons_transaction_ptr_t, xunconfirmed_tx_comp>;

class xpeer_table_unconfirmed_txs_t {
public:
    void push_tx(const xcons_transaction_ptr_t & tx);
    void erase(uint64_t receipt_id, xall_unconfirm_tx_set_t & all_unconfirm_tx_set);
    const xcons_transaction_ptr_t find(uint64_t receipt_id) const;
    void update_receipt_id(uint64_t max_confirm_id, xall_unconfirm_tx_set_t & all_unconfirm_tx_set);
    const xcons_transaction_ptr_t get_latest_receipt() const;
    const xcons_transaction_ptr_t get_first_receipt() const;

private:
    uint64_t m_max_confirm_id{0};
    std::map<uint64_t, xcons_transaction_ptr_t> m_unconfirmed_txs;  // key:receipt id, value:transaction
};

class xpeer_tables_t {
public:
    void push_tx(const xcons_transaction_ptr_t & tx);
    void erase(uint16_t peer_table_sid, uint64_t receipt_id);
    const xcons_transaction_ptr_t find(uint16_t peer_table_sid, uint64_t receipt_id) const;
    void update_receiptid_state(const xtable_state_cache_t & table_state_cache);
    const xall_unconfirm_tx_set_t & get_all_txs() const {
        return m_all_unconfirm_txs;
    }
    const std::vector<xcons_transaction_ptr_t> get_latest_receipts_for_resend(uint64_t now) const;

private:
    std::map<base::xtable_shortid_t, std::shared_ptr<xpeer_table_unconfirmed_txs_t>> m_peer_tables;  // key:peer table sid, value:table with unconfirmed txs
    xall_unconfirm_tx_set_t m_all_unconfirm_txs;                                                     // all unconfirmed txs ordered by time
};

class xunconfirmed_tx_info_t {
public:
    xunconfirmed_tx_info_t(uint16_t peer_table_sid, uint64_t receipt_id) : m_peer_table_sid(peer_table_sid), m_receipt_id(receipt_id) {
    }
    uint16_t get_peer_table_sid() const {
        return m_peer_table_sid;
    }
    uint64_t get_receipt_id() const {
        return m_receipt_id;
    }

private:
    uint16_t m_peer_table_sid;
    uint64_t m_receipt_id;
};

class xunconfirmed_account_t {
public:
    xunconfirmed_account_t(xtxpool_resources_face * para, xpeer_tables_t * peer_tables) : m_para(para), m_peer_tables(peer_tables) {
    }
    int32_t update(xblock_t * latest_committed_block, const xtable_state_cache_t & table_state_cache);
    const xcons_transaction_ptr_t find(const uint256_t & hash) const;
    uint32_t size() const {
        return m_unconfirmed_txs.size();
    }
    uint64_t get_height() const {return m_highest_height;}

    // // for test only, should delete!!!!!
    // void set_test_receipt_id(uint64_t id) {
    //     test_receipt_id = id;
    // }
    // uint64_t get_test_receipt_id() {
    //     uint64_t tmp_id = test_receipt_id;
    //     test_receipt_id++;
    //     return tmp_id;
    // }

private:
    uint64_t m_highest_height{0};
    xtxpool_resources_face * m_para;
    xpeer_tables_t * m_peer_tables;
    std::map<std::string, std::shared_ptr<xunconfirmed_tx_info_t>> m_unconfirmed_txs;  // key:tx hash, value:tx

    // // for test only, should delete!!!!!
    // uint64_t test_receipt_id{1};
};

class xunconfirmed_tx_queue_t {
public:
    xunconfirmed_tx_queue_t(xtxpool_resources_face * para, xtxpool_table_info_t * table_info) : m_para(para), m_table_info(table_info) {
    }
    ~xunconfirmed_tx_queue_t();

public:
    void udpate_latest_confirmed_block(xblock_t * block, const xtable_state_cache_t & table_state_cache);
    void recover(const xtable_state_cache_t & table_state_cache, const xtablestate_ptr_t & tablestate);
    const xcons_transaction_ptr_t find(const std::string & account_addr, const uint256_t & hash) const;
    const std::vector<xcons_transaction_ptr_t> get_resend_txs(uint64_t now);
    uint32_t size() const;
    uint64_t find_account_cache_height(const std::string & account_addr) const;
    xcons_transaction_ptr_t get_unconfirmed_tx(const std::string & to_table_addr, uint64_t receipt_id) const;
    xcons_transaction_ptr_t get_unconfirmed_tx(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id) const;

private:
    xtxpool_resources_face * m_para;
    xtxpool_table_info_t * m_table_info;
    xpeer_tables_t m_peer_tables;
    std::map<std::string, std::shared_ptr<xunconfirmed_account_t>> m_unconfirmed_accounts;  // key:tx hash, value:tx
};

NS_END2
