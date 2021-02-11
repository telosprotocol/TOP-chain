// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xdata/xtransaction.h"
#include "xstore/xstore_face.h"
#include "xtxpool/xtxpool_resources_face.h"

#include <map>
#include <set>
#include <string>

namespace top {
namespace xtxpool {

#define MAX_HEIGHT (0XFFFFFFFFFFFFFFFFULL)

// receipt cache of table
class xtransaction_recv_cache_t {
public:
    xtransaction_recv_cache_t(const std::shared_ptr<xtxpool_resources_face> & para,
                              std::string table_account);
    std::vector<std::pair<std::string, uint256_t>> update_tx_cache(xblock_t * block);
    int32_t  is_receipt_duplicated(uint64_t tx_timer_height, const xtransaction_t * receipt_tx, std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs, uint64_t commit_height = MAX_HEIGHT);
    void     clear();

private:
    bool     cache_from_db_with_timerblock_height(uint64_t height, uint64_t & min_timer_height, std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs);
    bool     cache_latest_blocks(std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs, uint64_t commit_height = MAX_HEIGHT);
    bool     cache_old_blocks(uint64_t old_timer_height, std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs);
    uint64_t insert_block_tx_to_cache(xblock_t * block, std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs);
    void     delete_expired_cache();
    bool     check_timer_height_trusted_section_by_left(uint64_t commit_height);
    int32_t  check_duplicate_in_cache(uint64_t tx_timer_height, const xtransaction_t * receipt_tx);

    std::string                             m_table_account;
    std::map<uint64_t, std::set<uint256_t>> m_transaction_recv_cache;             // cache for received send transaction receipts<timer_height, tx_hash_set>,
                                                                                  // key should be timer height, or else we have no idea for how to delete the expired cache
    uint64_t                               m_min_cache_block_height{MAX_HEIGHT};  // min block height of transaction recv cache, the beginning for cache old blocks
    uint64_t                               m_max_cache_block_height{0};           // max block height of transaction recv cache, the beginning for cache discontinuous new blocks
    uint64_t                               m_latest_block_height{0};              // latest block height of table
    bool                                   m_old_cache_full{false};               // true if old cache is full, m_min_cache_block_height and m_min_cache_height_map have no use
    std::shared_ptr<xtxpool_resources_face> m_para;
    std::mutex                             m_recv_cache_mutex;
};

}  // namespace xtxpool
}  // namespace top
