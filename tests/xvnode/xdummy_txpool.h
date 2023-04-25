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
    xpack_resource get_pack_resource(const xtxs_pack_para_t & pack_para) override {
        return {};
    }
    data::xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override {
        return nullptr;
    }
    data::xcons_transaction_ptr_t query_tx(const std::string & account, const std::string & hash_hex) const override {
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
    bool on_block_confirmed(const std::string table_addr, base::enum_xvblock_class blk_class, uint64_t height) override {
    }
    int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) override {
        return 0;
    }
    void refresh_table(uint8_t zone, uint16_t subaddr) override {
    }
    // void update_non_ready_accounts(uint8_t zone, uint16_t subaddr) override {}
    void update_table_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const data::xtablestate_ptr_t & table_state) override {
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

    void print_statistic_values() const override{};
    void update_peer_receipt_id_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const base::xreceiptid_state_ptr_t & receiptid_state) override {
    }
    std::map<std::string, uint64_t> get_min_keep_heights() const override {
        return {};
    xtransaction_ptr_t get_raw_tx(const std::string & account_addr, base::xtable_shortid_t peer_table_sid, uint64_t receipt_id) const override {
        return nullptr;
    }

    const std::set<base::xtable_shortid_t> & get_all_table_sids() const override {
        return m_all_table_sids;
    }

    uint32_t get_tx_cache_size(const std::string & table_addr) const override {
        return 0;
    }

    void update_uncommit_txs(base::xvblock_t * _lock_block, base::xvblock_t * _cert_block) override {}

    void add_tx_action_cache(base::xvblock_t * block, std::shared_ptr<std::vector<base::xvaction_t>> txactions) override {}
private:
    std::set<base::xtable_shortid_t> m_all_table_sids;
};

using xdummy_txpool_t = xtop_dummy_txpool;

extern xdummy_txpool_t dummy_txpool;

NS_END3
