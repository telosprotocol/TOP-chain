// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xtxpool_v2/xtxpool_face.h"

using namespace top::xtxpool_v2;
NS_BEG3(top, tests, vnode)
class xtop_dummy_txpool : public top::xtxpool_v2::xtxpool_face_t {
public:
    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx) override {return 0;}
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx) override {return 0;}
    const xcons_transaction_ptr_t pop_tx(const tx_info_t & txinfo) override {return nullptr;}
    ready_accounts_t pop_ready_accounts(const std::string & table_addr, uint32_t count) override {return {};}
    ready_accounts_t get_ready_accounts(const xtxs_pack_para_t & pack_para) override {return {};}
    std::vector<xcons_transaction_ptr_t> get_ready_txs(const xtxs_pack_para_t & pack_para) override {return {};}
    ready_accounts_t get_ready_accounts(const std::string & table_addr, uint32_t count) override {return {};}
    std::vector<xcons_transaction_ptr_t> get_ready_txs(const std::string & table_addr, uint32_t count) override {return {};}
    const std::shared_ptr<xtx_entry> query_tx(const std::string & account, const uint256_t & hash) const override {return nullptr;}
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce, const uint256_t & latest_hash) override {}
    void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override {}
    void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override {}
    void on_block_confirmed(xblock_t * block) override {}
    xcons_transaction_ptr_t get_unconfirm_tx(const std::string source_addr, const uint256_t & hash) const override {return nullptr;}
    int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs, uint64_t latest_commit_unit_height) override {return 0;}
    int32_t reject(const xcons_transaction_ptr_t & tx, uint64_t latest_commit_unit_height, bool & deny) override {return false;}
    const std::vector<xcons_transaction_ptr_t> get_resend_txs(uint8_t zone, uint16_t subaddr, uint64_t now) override {return {};}
    void update_unconfirm_accounts(uint8_t zone, uint16_t subaddr) override {}
    void update_non_ready_accounts(uint8_t zone, uint16_t subaddr) override {}
    void update_locked_txs(const std::string & table_addr, const std::vector<tx_info_t> & locked_tx_vec, const base::xreceiptid_state_ptr_t & receiptid_state) override {}
    void update_receiptid_state(const std::string & table_addr, const base::xreceiptid_state_ptr_t & receiptid_state) override {}
};

using xdummy_txpool_t = xtop_dummy_txpool;

extern xdummy_txpool_t dummy_txpool;

NS_END3
