// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xtxpool_v2/xtxpool_table.h"

#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace top {
namespace xtxpool_v2 {

#define xtxpool_zone_type_max (6)

struct xtable {
    xtable() {}
    xtable(const xtable & table) {
        _table = table._table;
    }
    std::shared_ptr<xtxpool_table_t> _table{nullptr};
    std::mutex _table_mutex;
};

class xtables_mgr {
public:
    void add_tables(uint8_t zone, uint32_t size);
    bool subscribe_table(uint8_t zone,
                         uint16_t subaddr,
                         xtxpool_resources_face * para,
                         xtxpool_role_info_t * role,
                         xtxpool_statistic_t * statistic,
                         std::set<base::xtable_shortid_t> * all_sid_set);
    bool unsubscribe_table(uint8_t zone, uint16_t subaddr, xtxpool_role_info_t * role);
    std::shared_ptr<xtxpool_table_t> get_table(uint8_t zone, uint16_t subaddr) const;
public:
    mutable std::vector<xtable> m_tables[xtxpool_zone_type_max];
};

class xtxpool_t : public xtxpool_face_t {
public:
    xtxpool_t(const std::shared_ptr<xtxpool_resources_face> & para);

    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx) override;
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send, bool is_pulled) override;
    const xcons_transaction_ptr_t pop_tx(const tx_info_t & txinfo) override;
    xpack_resource get_pack_resource(const xtxs_pack_para_t & pack_para) override;
    data::xcons_transaction_ptr_t query_tx(const std::string & account_addr, const uint256_t & hash) const override;
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) override;
    void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) override;
    void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id, common::xnode_type_t node_type) override;
    void on_block_confirmed(xblock_t * block) override;
    int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) override;
    void refresh_table(uint8_t zone, uint16_t subaddr) override;
    // void update_non_ready_accounts(uint8_t zone, uint16_t subaddr) override;
    void update_table_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const data::xtablestate_ptr_t & table_state, base::xvblock_t* _lock_block, base::xvblock_t* _cert_block) override;
    void build_recv_tx(base::xtable_shortid_t from_table_sid,
                       base::xtable_shortid_t to_table_sid,
                       std::vector<uint64_t> receiptids,
                       std::vector<xcons_transaction_ptr_t> & receipts) override;
    void build_confirm_tx(base::xtable_shortid_t from_table_sid,
                          base::xtable_shortid_t to_table_sid,
                          std::vector<uint64_t> receiptids,
                          std::vector<xcons_transaction_ptr_t> & receipts) override;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const override;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_confirm_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t & total_num) const override;
    void print_statistic_values() const override;
    void update_peer_receipt_id_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const base::xreceiptid_state_ptr_t & receiptid_state) override;
    std::map<std::string, uint64_t> get_min_keep_heights() const override;
    xtransaction_ptr_t get_raw_tx(const std::string & account_addr, base::xtable_shortid_t peer_table_sid, uint64_t receipt_id) const override;
    const std::set<base::xtable_shortid_t> & get_all_table_sids() const override;
    uint32_t get_tx_cache_size(const std::string & table_addr) const override;

private:
    std::shared_ptr<xtxpool_table_t> get_txpool_table_by_addr(const std::string & address) const;
    std::shared_ptr<xtxpool_table_t> get_txpool_table_by_addr(const std::shared_ptr<xtx_entry> & tx) const;
    std::shared_ptr<xtxpool_table_t> get_txpool_table(uint8_t zone, uint16_t subaddr) const;

    xtables_mgr m_tables_mgr;
    std::vector<std::shared_ptr<xtxpool_role_info_t>> m_roles[xtxpool_zone_type_max];
    std::shared_ptr<xtxpool_resources_face> m_para;
    mutable std::mutex m_mutex[xtxpool_zone_type_max];
    xtxpool_statistic_t m_statistic;
    std::set<base::xtable_shortid_t> m_all_table_sids;
    std::map<base::xtable_shortid_t, uint64_t> m_peer_table_height_cache;
    mutable std::mutex m_peer_table_height_cache_mutex;
};

}  // namespace xtxpool_v2
}  // namespace top
