// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xtxpool/xtxpool_face.h"

NS_BEG3(top, tests, vnode)
class xtop_dummy_txpool : public top::xtxpool::xtxpool_face_t {
public:
    int32_t push_send_tx(const xtransaction_ptr_t & tx) override { return 0; }
    int32_t push_send_tx(const xcons_transaction_ptr_t & tx) override { return 0; }
    int32_t push_recv_tx(const xcons_transaction_ptr_t & tx) override { return 0; }
    int32_t push_recv_ack_tx(const xcons_transaction_ptr_t & tx) override { return 0; }
    std::map<std::string, std::vector<xcons_transaction_ptr_t>> get_txs(const std::string & tableblock_account, uint64_t commit_height) override { return {}; }
    int32_t verify_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, std::vector<xcons_transaction_ptr_t> txs, uint64_t last_nonce, const uint256_t & last_hash) override { return 0; }
    void on_block_confirmed(xblock_t * block) override { return; }
    top::xtxpool::xaccountobj_t * find_account(const std::string & address) override { return nullptr; }
    xcons_transaction_ptr_t pop_tx_by_hash(const std::string & address, const uint256_t & hash, uint8_t subtype, int32_t result) override { return {}; }
    top::xtxpool::xtxpool_table_face_t * get_txpool_table(const std::string & table_account) override { return nullptr; }
    int32_t on_receipt(const data::xcons_transaction_ptr_t & cons_tx) override { return 0; }
    void set_receipt_tranceiver(std::shared_ptr<top::xtxpool::xreceipt_tranceiver_face_t> receipt_tranceiver) override { return; }
    void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override { return; }
    void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override { return; }
    std::vector<std::string> get_accounts(const std::string & tableblock_account) override { return {}; }
    std::vector<xcons_transaction_ptr_t> get_account_txs(const std::string & account,
                                                         uint64_t commit_height,
                                                         uint64_t unit_height,
                                                         uint64_t last_nonce,
                                                         const uint256_t & last_hash,
                                                         uint64_t now) override {
        return {};
    }
    void dispatch(base::xcall_t & call) override { return; }
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const {return nullptr;}
    bool                    is_mailbox_over_limit() override {return false;}
    xcons_transaction_ptr_t get_unconfirm_tx(const std::string source_addr, const uint256_t & hash) {return nullptr;}
};

using xdummy_txpool_t = xtop_dummy_txpool;

extern xdummy_txpool_t dummy_txpool;

NS_END3
