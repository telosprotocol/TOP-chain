// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xtxpool_v2/xtxpool_face.h"

using namespace top::xtxpool_v2;
NS_BEG3(top, tests, vnode)
class xtop_dummy_txpool : public top::xtxpool_v2::xtxpool_face_t {
public:
    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx) override {
        return 0;
    }
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send, bool is_pulled) override {
        return 0;
    }
    const xcons_transaction_ptr_t pop_tx(const tx_info_t & txinfo) override {
        return nullptr;
    }
    ready_accounts_t get_ready_accounts(const xtxs_pack_para_t & pack_para) override {
        return {};
    }
    std::vector<xcons_transaction_ptr_t> get_ready_txs(const xtxs_pack_para_t & pack_para) override {
        return {};
    }
    const std::shared_ptr<xtx_entry> query_tx(const std::string & account, const uint256_t & hash) const override {
        return nullptr;
    }
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) override {
    }
    void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) override {
    }
    void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) override {
    }
    void on_block_confirmed(xblock_t * block) override {
    }
    int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) override {
        return 0;
    }
    void refresh_table(uint8_t zone, uint16_t subaddr) override {
    }
    // void update_non_ready_accounts(uint8_t zone, uint16_t subaddr) override {}
    void update_table_state(const data::xtablestate_ptr_t & table_state) override {
    }
    void build_recv_tx(base::xtable_shortid_t from_table_sid,
                       base::xtable_shortid_t to_table_sid,
                       std::vector<uint64_t> receiptids,
                       std::vector<xcons_transaction_ptr_t> & receipts) override {
    }
    void build_confirm_tx(base::xtable_shortid_t from_table_sid,
                          base::xtable_shortid_t to_table_sid,
                          std::vector<uint64_t> receiptids,
                          std::vector<xcons_transaction_ptr_t> & receipts) override {
    }
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const override {
        return {};
    }
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_confirm_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const override {
        return {};
    }
    bool need_sync_lacking_receipts(uint8_t zone, uint16_t subaddr) const override {
        return false;
    }
    void print_statistic_values() const override{};
    void update_peer_receipt_id_state(const base::xreceiptid_state_ptr_t & receiptid_state) override {
    }
};

using xdummy_txpool_t = xtop_dummy_txpool;

extern xdummy_txpool_t dummy_txpool;

NS_END3
