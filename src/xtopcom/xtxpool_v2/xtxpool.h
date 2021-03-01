// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xdata/xcons_transaction.h"
#include "xtxpool_v2/xtxmgr_table.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxpool_v2/xtxpool_info.h"

#include <map>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace top {
namespace xtxpool_v2 {

enum enum_xtxpool_table_type { enum_xtxpool_table_type_max = 3 };

class xtxpool_table_t {
public:
    xtxpool_table_t(std::string table_addr, xtxpool_shard_info_t * shard) : m_xtable_info(table_addr, shard), m_txmgr_table(&m_xtable_info) {
    }
    int32_t init() {
        return m_txmgr_table.init();
    }
    int32_t deinit() {
        return m_txmgr_table.deinit();
    }
    int32_t push_tx(const xcons_transaction_ptr_t & tx, const xtx_para_t & tx_para) {
        return m_txmgr_table.push_tx(tx, tx_para);
    }
    std::shared_ptr<xtx_entry> pop_tx_by_hash(const std::string & account, const uint256_t & hash, uint8_t subtype, int32_t err) {
        return m_txmgr_table.pop_tx_by_hash(account, hash, subtype, err);
    }
    candidate_accounts get_accounts_txs(uint32_t count) {
        return m_txmgr_table.get_accounts_txs(count);
    }
    int32_t push_back_tx(std::shared_ptr<xtx_entry> tx_ent) {
        return m_txmgr_table.push_back_tx(tx_ent);
    }
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) {
        return m_txmgr_table.query_tx(account, hash);
    }

private:
    xtxpool_table_info_t m_xtable_info;
    xtxmgr_table_t m_txmgr_table;
    // todo: xtx_deduplicate_table_t m_tx_dd_table; // transaction deduplication
};

class xtxpool_t : public xtxpool_face_t {
public:
    int32_t push_tx(const xcons_transaction_ptr_t & tx, const xtx_para_t & tx_para) override;
    const xcons_transaction_ptr_t pop_tx_by_hash(const std::string & account_addr, const uint256_t & hash, uint8_t subtype, int32_t err) override;
    candidate_accounts get_candidate_accounts(const std::string & table_addr, uint32_t count) override;
    int32_t push_back_tx(std::shared_ptr<xtx_entry> tx_ent) override;
    const xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override;
    void subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override;
    void unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) override;

private:
    std::shared_ptr<xtxpool_table_t> get_txpool_table_by_addr(const std::string & address) const;
    mutable std::shared_ptr<xtxpool_table_t> m_tables[enum_xtxpool_table_type_max][enum_vbucket_has_tables_count];
    std::vector<std::shared_ptr<xtxpool_shard_info_t>> m_shards;
};

}  // namespace xtxpool_v2
}  // namespace top
