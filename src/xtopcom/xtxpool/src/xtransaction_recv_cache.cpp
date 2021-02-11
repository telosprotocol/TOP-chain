// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool/xtransaction_recv_cache.h"

#include "xbase/xlog.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xblocktool.h"
#include "xdata/xchain_param.h"
#include "xdata/xdatautil.h"
#include "xdata/xgenesis_data.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xtableblock.h"
#include "xtxpool/xtxpool_error.h"
#include "xtxpool/xtxpool_log.h"
#include "xverifier/xverifier_utl.h"

#include <assert.h>

#include <algorithm>

namespace top {
namespace xtxpool {

#define MORE_BLOCK_HEIGHT (20)

xtransaction_recv_cache_t::xtransaction_recv_cache_t(const std::shared_ptr<xtxpool_resources_face> & para,
                                                     std::string                                table_account)
  : m_table_account(table_account), m_para(para) {}

void xtransaction_recv_cache_t::clear() {
    xdbg("xtransaction_recv_cache table=%s clear tx recv cache", m_table_account.c_str());
    std::lock_guard<std::mutex> lck(m_recv_cache_mutex);
    m_transaction_recv_cache.clear();
    m_min_cache_block_height = MAX_HEIGHT;
    m_max_cache_block_height = 0;
    m_latest_block_height = 0;
    m_old_cache_full = false;
}

std::vector<std::pair<std::string, uint256_t>> xtransaction_recv_cache_t::update_tx_cache(xblock_t * block) {
    std::vector<std::pair<std::string, uint256_t>> committed_recv_txs;
    std::lock_guard<std::mutex> lck(m_recv_cache_mutex);
    m_latest_block_height = (m_latest_block_height > block->get_height()) ? m_latest_block_height : block->get_height();
    xdbg("xtransaction_recv_cache table=%s update tx cache. latest height:%llu, max cache height:%llu, block height:%llu",
          m_table_account.c_str(), m_latest_block_height, m_max_cache_block_height, block->get_height());

    if (block->get_height() == m_max_cache_block_height + 1 || m_max_cache_block_height == 0) {
        insert_block_tx_to_cache(block, committed_recv_txs);
    }
    cache_latest_blocks(committed_recv_txs);
    delete_expired_cache();

    return committed_recv_txs;
}

bool xtransaction_recv_cache_t::cache_from_db_with_timerblock_height(uint64_t height, uint64_t & min_timer_height, std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs) {
    auto blockobj = m_para->get_vblockstore()->load_block_object(m_table_account, height);
    if (blockobj == nullptr) {
        xwarn("xtransaction_recv_cache table=%s block read from db fail. height:%llu", m_table_account.c_str(), height);
        return false;
    }
    if (!blockobj->check_block_flag(base::enum_xvblock_flag::enum_xvblock_flag_committed)) {
        xwarn("xtransaction_recv_cache table=%s block is not committed. height:%llu", m_table_account.c_str(), height);
        return false;
    }
    xblock_t * block = dynamic_cast<xblock_t *>(blockobj.get());
    min_timer_height = insert_block_tx_to_cache(block, committed_recv_txs);
    return true;
}

bool xtransaction_recv_cache_t::cache_latest_blocks(std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs, uint64_t commit_height) {
    if (commit_height == 0) {
        // no block stored to db, no cache
        return true;
    }
    uint64_t min_timer_height;
    // latest blocks insert to cache from db
    if (m_latest_block_height == 0) {
        auto committed_block = m_para->get_vblockstore()->get_latest_committed_block(m_table_account);
        if (committed_block != nullptr) {
            m_latest_block_height = committed_block->get_height();
            xblock_t * block = dynamic_cast<xblock_t*>(committed_block.get());
            insert_block_tx_to_cache(block, committed_recv_txs);
        } else {
            // no block stored to db, no cache
            return true;
        }
    }
    for (uint64_t h = m_max_cache_block_height + 1; h <= m_latest_block_height; h++) {
        if (!cache_from_db_with_timerblock_height(h, min_timer_height, committed_recv_txs)) {
            // TODO(Nathan):sync blocks
            return false;
        }
    }
    if (commit_height != MAX_HEIGHT) {
        for (uint64_t h = m_latest_block_height + 1; h <= commit_height; h++) {
            if (!cache_from_db_with_timerblock_height(h, min_timer_height, committed_recv_txs)) {
                // TODO(Nathan):sync blocks
                return false;
            }
            m_latest_block_height = m_latest_block_height + 1;
        }
    }
    return true;
}

bool xtransaction_recv_cache_t::cache_old_blocks(uint64_t old_timer_height, std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs) {
    // old block cache is already cached full, need not load more cache from db.
    if (m_old_cache_full) {
        return true;
    }

    uint64_t h = m_min_cache_block_height;
    if (m_min_cache_block_height != MAX_HEIGHT && m_transaction_recv_cache.empty()) {
        while (h > 1 && m_transaction_recv_cache.empty()) {
            uint64_t min_timer_height;
            if (!cache_from_db_with_timerblock_height(h - 1, min_timer_height, committed_recv_txs)) {
                // TODO(Nathan):sync blocks
                xwarn("cache block fail: table=%s height:%llu", m_table_account.c_str(), h - 1);
                return false;
            }
            h--;
        }
    }

    // old blocks insert to cache
    auto it = m_transaction_recv_cache.begin();
    if (it == m_transaction_recv_cache.end()) {
        return true;
    }
    uint64_t min_trusted_timer_height = it->first + MORE_BLOCK_HEIGHT;
    if (old_timer_height >= min_trusted_timer_height) {
        return true;
    }

    while (h > 1 && old_timer_height >= min_trusted_timer_height) {
        uint64_t min_timer_height;
        if (!cache_from_db_with_timerblock_height(h - 1, min_timer_height, committed_recv_txs)) {
            // TODO(Nathan):sync blocks
            xwarn("cache block fail: table=%s height:%llu", m_table_account.c_str(), h - 1);
            return false;
        }
        if (min_timer_height != (uint64_t)-1) {
            min_trusted_timer_height = min_timer_height + MORE_BLOCK_HEIGHT;
        }
        h--;
    }

    if (min_trusted_timer_height + m_para->get_receipt_valid_window() <= m_para->get_chain_timer()->logic_time()) {
        xinfo("xtransaction_recv_cache table=%s old cache is full.From then on, need not cache old blocks", m_table_account.c_str());
        m_old_cache_full = true;
    }
    return true;
}

uint64_t xtransaction_recv_cache_t::insert_block_tx_to_cache(xblock_t * block, std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs) {
    uint64_t min_timer_height = (uint64_t)-1;
    m_min_cache_block_height = m_min_cache_block_height < block->get_height() ? m_min_cache_block_height : block->get_height();
    m_max_cache_block_height = m_max_cache_block_height > block->get_height() ? m_max_cache_block_height : block->get_height();
    int32_t count = 0;

    const auto & units = block->get_tableblock_units(false);
    if (!units.empty()) {
        for (auto & unit : units) {
            if (unit->get_block_class() != base::enum_xvblock_class_light) {
                continue;
            }
            data::xlightunit_block_t * lightunit = dynamic_cast<data::xlightunit_block_t *>(unit.get());
            const std::vector<xlightunit_tx_info_ptr_t> & txs = lightunit->get_txs();
            for (auto & tx : txs) {
                /* only recv tx need to be cached */
                if (tx->is_recv_tx()) {
                    uint64_t timer_height = tx->get_tx_clock();
                    if (timer_height < min_timer_height) {
                        min_timer_height = timer_height;
                    }
                    xinfo("xtransaction_recv_cache table=%s account=%s update tx cache. timer_height:%ull, tx:%s",
                          m_table_account.c_str(), tx->get_raw_tx()->get_target_addr().c_str(), timer_height, tx->get_raw_tx()->get_digest_hex_str().c_str());
                    auto const it = m_transaction_recv_cache.find(timer_height);
                    if (it == std::end(m_transaction_recv_cache)) {
                        m_transaction_recv_cache.insert({timer_height, {tx->get_raw_tx()->digest()}});
                    } else {
                        it->second.insert(tx->get_raw_tx()->digest());
                    }
                    committed_recv_txs.push_back(make_pair(tx->get_raw_tx()->get_target_addr(), tx->get_raw_tx()->digest()));
                    count++;
                }
            }
        }
        XMETRICS_COUNTER_INCREMENT("txpool_receipt_deweight_cache", count);
    }
    return min_timer_height;
}

void xtransaction_recv_cache_t::delete_expired_cache() {
    uint64_t cur_timer_height = m_para->get_chain_timer()->logic_time();
    if (!m_old_cache_full || cur_timer_height < m_para->get_receipt_valid_window()) {
        return;
    }

    xinfo("xtransaction_recv_cache table=%s delete old cache. cur_timer_height:%llu", m_table_account.c_str(), cur_timer_height);
    int32_t count = 0;
    for (auto iter = m_transaction_recv_cache.begin(); iter != m_transaction_recv_cache.end() && iter->first + m_para->get_receipt_valid_window() < cur_timer_height;) {
        xdbg("xtransaction_recv_cache table=%s delete old cache.timer height:%llu", m_table_account.c_str(), iter->first);
        iter = m_transaction_recv_cache.erase(iter);
        count++;
    }
    XMETRICS_COUNTER_DECREMENT("txpool_receipt_deweight_cache", count);
    // Notice: If m_transaction_recv_cache is empty m_max_cache_block_height and m_min_cache_block_height should not be set to 0.
    //         Because it's posible that there is no transaction in the temporal interval.
}

bool xtransaction_recv_cache_t::check_timer_height_trusted_section_by_left(uint64_t commit_height) {
    if (m_max_cache_block_height == m_latest_block_height) {
        if (commit_height == MAX_HEIGHT || commit_height == m_latest_block_height) {
            return true;
        }
    }
    xwarn("cache not continous, table=%s max cache h:%llu latest h:%llu commit height:%llu",
        m_table_account.c_str(), m_max_cache_block_height, m_latest_block_height, commit_height);
    return false;
}

int32_t xtransaction_recv_cache_t::check_duplicate_in_cache(uint64_t tx_timer_height, const xtransaction_t * receipt_tx) {
    auto iter = m_transaction_recv_cache.find(tx_timer_height);
    if (iter != m_transaction_recv_cache.end()) {
        const auto & tx_hash_set = iter->second;
        auto         iter1 = tx_hash_set.find(receipt_tx->digest());
        // found from cache, the receipt is duplicate.
        if (iter1 != tx_hash_set.end()) {
            xwarn("xtransaction_recv_cache table=%s tx send receipt duplicate.timer_height:%ld txHash:%s", m_table_account.c_str(), tx_timer_height, receipt_tx->get_digest_hex_str().c_str());
            return xtxpool_error_sendtx_receipt_duplicate;
        }
    }
    xdbg("xtransaction_recv_cache table=%s tx is not duplicate. timer_height:%ld txHash:%s", m_table_account.c_str(), tx_timer_height, receipt_tx->get_digest_hex_str().c_str());
    return xsuccess;
}

int32_t xtransaction_recv_cache_t::is_receipt_duplicated(uint64_t tx_timer_height, const xtransaction_t * receipt_tx, std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs, uint64_t commit_height) {
    uint64_t cur_timer_height = m_para->get_chain_timer()->logic_time();
    int32_t  ret = xtxpool_error_sendtx_receipt_data_not_synced;
    std::lock_guard<std::mutex> lck(m_recv_cache_mutex);
    // Receipt timer height is not in N days.
    if (tx_timer_height + m_para->get_receipt_valid_window() < cur_timer_height) {
        xwarn("xtransaction_recv_cache table=%s expired tx. timer_height:%llu, cur_timer_height:%llu, txHash:%s",
              m_table_account.c_str(),
              tx_timer_height,
              cur_timer_height,
              receipt_tx->get_digest_hex_str().c_str());
        return xtxpool_error_sendtx_receipt_expired;
    }

    do {
        // Step1: update right side of the trusted section, by this step, we can not judge the TX is trusted.
        if (!cache_latest_blocks(committed_recv_txs, commit_height)) {
            break;
        }

        // Step2: update left side of the trusted section, return false if old block cache is not enough to judge whether the receipt is dulicated, need sync more block.
        //        that means the TX is not in the cache trusted section, need sync.
        if (!cache_old_blocks(tx_timer_height, committed_recv_txs)) {
            xwarn("xtransaction_recv_cache table=%s tx recv cache cannot reach this tx.timer_height:%llu, cur_timer_height:%llu, txHash:%s",
                m_table_account.c_str(),
                tx_timer_height,
                cur_timer_height,
                receipt_tx->get_digest_hex_str().c_str());
            break;
        }

        // Step3: return true if cache is integrated, or receipt timer height is in the trusted section [min+5,max-5].
        if (!check_timer_height_trusted_section_by_left(commit_height)) {
            break;
        }
        ret = xsuccess;
    } while(0);

    // Step4: TX timer height is in the trusted section, if it's found in the cache, the TX is duplicated, or else it's valid.
    int32_t duplicate_ret = check_duplicate_in_cache(tx_timer_height, receipt_tx);
    return (duplicate_ret == xsuccess) ? ret : duplicate_ret;
}

}  // namespace xtxpool
}  // namespace top
