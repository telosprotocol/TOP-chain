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

enum enum_xtxpool_table_type { enum_xtxpool_table_type_max = 3 };

class xtxpool_t : public xtxpool_face_t {
public:
    xtxpool_t(const std::shared_ptr<xtxpool_resources_face> & para);

    int32_t push_send_tx(const std::shared_ptr<xtx_entry> & tx) override;
    int32_t push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send, bool is_pulled) override;
    const xcons_transaction_ptr_t pop_tx(const tx_info_t & txinfo) override;
    ready_accounts_t get_ready_accounts(const xtxs_pack_para_t & pack_para) override;
    std::vector<xcons_transaction_ptr_t> get_ready_txs(const xtxs_pack_para_t & pack_para) override;
    const std::shared_ptr<xtx_entry> query_tx(const std::string & account_addr, const uint256_t & hash) const override;
    void updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) override;
    void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override;
    void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override;
    void on_block_confirmed(xblock_t * block) override;
    int32_t verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) override;
    const std::vector<xcons_transaction_ptr_t> get_resend_txs(uint8_t zone, uint16_t subaddr, uint64_t now) override;
    void refresh_table(uint8_t zone, uint16_t subaddr, bool refresh_unconfirm_txs) override;
    // void update_non_ready_accounts(uint8_t zone, uint16_t subaddr) override;
    void update_table_state(const data::xtablestate_ptr_t & table_state) override;
    xcons_transaction_ptr_t get_unconfirmed_tx(const std::string & from_table_addr, const std::string & to_table_addr, uint64_t receipt_id) const override;
    const std::vector<xtxpool_table_lacking_receipt_ids_t> get_lacking_recv_tx_ids(uint8_t zone, uint16_t subaddr, uint32_t max_num) const override;
    const std::vector<xtxpool_table_lacking_confirm_tx_hashs_t> get_lacking_confirm_tx_hashs(uint8_t zone, uint16_t subaddr, uint32_t max_num) const override;
    bool need_sync_lacking_receipts(uint8_t zone, uint16_t subaddr) const override;
    void print_statistic_values() const override;
    bool is_consensused_recv_receiptid(const std::string & from_addr, const std::string & to_addr, uint64_t receipt_id) const override;
    bool is_consensused_confirm_receiptid(const std::string & from_addr, const std::string & to_addr, uint64_t receipt_id) const override;

private:
    bool is_table_subscribed(uint8_t zone, uint16_t table_id) const;
    std::shared_ptr<xtxpool_table_t> get_txpool_table_by_addr(const std::string & address) const;
    std::shared_ptr<xtxpool_table_t> get_txpool_table_by_addr(const std::shared_ptr<xtx_entry> & tx) const;

    mutable std::shared_ptr<xtxpool_table_t> m_tables[enum_xtxpool_table_type_max][enum_vbucket_has_tables_count];
    bool m_table_recover_flag_arr[enum_xtxpool_table_type_max][enum_vbucket_has_tables_count];
    std::vector<std::shared_ptr<xtxpool_shard_info_t>> m_shards;
    std::shared_ptr<xtxpool_resources_face> m_para;
    mutable std::mutex m_mutex[enum_xtxpool_table_type_max];
    xtxpool_statistic_t m_statistic;
};

}  // namespace xtxpool_v2
}  // namespace top
