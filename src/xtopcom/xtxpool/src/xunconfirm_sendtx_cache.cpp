// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool/xunconfirm_sendtx_cache.h"

#include "xconfig/xconfig_register.h"
#include "xdata/xlightunit.h"
#include "xdata/xtableblock.h"
#include "xtxpool/xtxpool_error.h"
#include "xtxpool/xtxpool_log.h"
#include "xtxpool/xtxpool_face.h"
#include "xtxpool/xtxpool_utl.h"

#include <string>

NS_BEG2(top, xtxpool)

void xunconfirm_sendtx_account_cache_t::push_receipt(const std::string & table_addr, const std::string & address, const uint64_t & unit_height, const uint256_t & txhash, const xcons_transaction_ptr_t & receipt) {
    auto iter = m_receipts.find(txhash);
    if (iter == m_receipts.end()) {
        m_receipts[txhash] = receipt;
        xkinfo("unconfirm cache push receipt, table=%s,account=%s,height=%ld %s", table_addr.c_str(), address.c_str(), unit_height, data::to_hex_str(txhash).c_str());
        XMETRICS_COUNTER_INCREMENT("txpool_unconfirm_cache", 1);
    }
}

void xunconfirm_sendtx_account_cache_t::pop_receipt(const std::string & table_addr, const std::string & address, const uint64_t & unit_height, const uint256_t & txhash) {
    auto iter = m_receipts.find(txhash);
    if (iter != m_receipts.end()) {
        iter->second->set_confirmed();
        m_receipts.erase(iter);
        xkinfo("unconfirm cache pop receipt, table=%s,account=%s,height=%ld %s", table_addr.c_str(), address.c_str(), unit_height, data::to_hex_str(txhash).c_str());
        XMETRICS_COUNTER_INCREMENT("txpool_unconfirm_cache", -1);
    }
}

void xunconfirm_sendtx_account_cache_t::pop_all_receipt(const std::string & table_addr, const std::string & address) {
    uint32_t num = m_receipts.size();
    for (auto iter = m_receipts.begin(); iter != m_receipts.end();) {
        iter->second->set_confirmed();
        xkinfo("unconfirm cache pop receipt all, table=%s,account=%s %s", table_addr.c_str(), address.c_str(), data::to_hex_str(iter->first).c_str());
        iter = m_receipts.erase(iter);
    }
    XMETRICS_COUNTER_DECREMENT("txpool_unconfirm_cache", num);
}

void xunconfirm_sendtx_account_cache_t::set_cache_max_height(uint64_t height) {
    xassert(height > m_cache_max_height);
    m_cache_max_height = height;
}

void xunconfirm_sendtx_account_cache_t::set_account_max_height(uint64_t height) {
    xassert(height >= m_account_max_height);
    m_account_max_height = height;
}

bool xunconfirm_sendtx_account_cache_t::has_txhash_receipt(const uint256_t & txhash) {
    auto iter = m_receipts.find(txhash);
    if (iter != m_receipts.end()) {
        return true;
    }
    xdbg("unconfirm cache cache not find txhash:%s", data::to_hex_str(txhash).c_str());
    return false;
}

const xcons_transaction_ptr_t xunconfirm_sendtx_account_cache_t::get_tx(const uint256_t & txhash) const {
    auto iter = m_receipts.find(txhash);
    if (iter == m_receipts.end()) {
        return nullptr;
    }
    return iter->second;
}

xunconfirm_sendtx_cache_t::xunconfirm_sendtx_cache_t(const std::shared_ptr<xtxpool_resources_face> &para, std::string & table_addr) : m_para(para), m_table_addr(table_addr) {
}

xunconfirm_sendtx_account_cache_ptr_t xunconfirm_sendtx_cache_t::get_account_cache(const std::string & address) const {
    auto iter = m_accounts_cache.find(address);
    if (iter == m_accounts_cache.end()) {
        return nullptr;
    }
    return iter->second;
}

xunconfirm_sendtx_account_cache_ptr_t xunconfirm_sendtx_cache_t::find_and_create_account_cache(const std::string & address, uint64_t account_height) {
    xunconfirm_sendtx_account_cache_ptr_t account_cache;
    auto                                  iter = m_accounts_cache.find(address);
    if (iter == m_accounts_cache.end()) {
        account_cache = std::make_shared<xunconfirm_sendtx_account_cache_t>();
        m_accounts_cache[address] = account_cache;
    } else {
        account_cache = iter->second;
    }
    account_cache->set_account_max_height(account_height);
    return account_cache;
}

int32_t xunconfirm_sendtx_cache_t::check_and_update_cache(const std::string & address, uint64_t now, uint64_t unit_height) {
    base::xauto_ptr<base::xvblock_t> unit = m_para->get_vblockstore()->get_latest_committed_block(address);
    xassert(unit != nullptr);
    if (unit == nullptr) {
        xwarn("[unconfirm cache]unit is null, can't update. %s", address.c_str());
        return xtxpool_error_unconfirm_sendtx_cache_update_fail_state_behind;
    }
    xblock_t * block = dynamic_cast<xblock_t *>(unit.get());

    if (unit->is_genesis_block() || !xblocktool_t::is_connect_and_executed_block(unit.get())) {
        xwarn("[unconfirm cache]account state behind, can't update.table=%s,account=%s,block=%s", m_table_addr.c_str(), address.c_str(), unit->dump().c_str());
        return xtxpool_error_unconfirm_sendtx_cache_update_fail_state_behind;
    }

    if (unit_height != 0 && unit_height != unit->get_height()) {
        xwarn("unconfirm cache account state behind, can't update.table=%s,account=%s,height=%ld chain_height=%ld", m_table_addr.c_str(), address.c_str(), unit_height, unit->get_height());
        return xtxpool_error_unconfirm_sendtx_cache_height_not_match_consensusing_height;
    }

    if (block->is_fullunit()) {
        clear_account_cache(address);
        return xtxpool_error_unconfirm_sendtx_not_exist;
    }

    data::xlightunit_block_t * lightunit = (data::xlightunit_block_t *)unit->query_interface(data::xlightunit_block_t::get_object_type());
    xassert(lightunit != nullptr);
    if (lightunit->get_unconfirm_sendtx_num() == 0) {
        clear_account_cache(address);
        return xtxpool_error_unconfirm_sendtx_not_exist;
    }

    xunconfirm_sendtx_account_cache_ptr_t account_cache = find_and_create_account_cache(address, unit->get_height());
    xassert(account_cache->get_cache_max_height() <= unit->get_height());
    if (account_cache->get_cache_max_height() == unit->get_height()) {
        return xsuccess;
    }

    // if account cache not exist, may be this address not operate long time, should update cache again
    update_cache(account_cache, address, now, unit);
    xassert(account_cache->get_cache_max_height() == unit->get_height());
    return xsuccess;
}

int32_t xunconfirm_sendtx_cache_t::has_txhash_receipt(const std::string & address, const uint256_t & txhash, uint64_t now, uint64_t unit_height) {
    std::lock_guard<std::mutex> lck(m_cache_mutex);
    int32_t ret = check_and_update_cache(address, now, unit_height);
    if (ret != xsuccess) {
        return ret;
    }

    auto account_cache = get_account_cache(address);
    return account_cache->has_txhash_receipt(txhash) ? xtxpool_error_unconfirm_sendtx_exist : xtxpool_error_unconfirm_sendtx_not_exist;
}

void xunconfirm_sendtx_cache_t::clear_account_cache(const std::string & address) {
    auto iter = m_accounts_cache.find(address);
    if (iter != m_accounts_cache.end()) {
        iter->second->pop_all_receipt(m_table_addr, address);
        m_accounts_cache.erase(iter);
    }
}

void xunconfirm_sendtx_cache_t::update_cache(xunconfirm_sendtx_account_cache_ptr_t & account_cache, const std::string & address, uint64_t now, base::xauto_ptr<base::xvblock_t> & last_committed_unit) {
    // update receipts to account cache
    std::vector<xblock_t *> units;
    traverse_and_get_unconfirm_units(address, account_cache->get_cache_max_height(), units, last_committed_unit);
    traverse_units_and_update_receipt_cache(units, account_cache, now);
    for (auto & unit : units) {
        unit->release_ref();
    }
}

void xunconfirm_sendtx_cache_t::clear() {
    std::lock_guard<std::mutex> lck(m_cache_mutex);
    for (auto iter = m_accounts_cache.begin(); iter != m_accounts_cache.end();) {
        iter->second->pop_all_receipt(m_table_addr, iter->first);
        iter = m_accounts_cache.erase(iter);
    }
    XMETRICS_COUNTER_DECREMENT("txpool_receipt_retry_cache", m_retry_cache.size());
    m_unconfirm_tx_num -= m_retry_cache.size();
    m_retry_cache.clear();
}

void xunconfirm_sendtx_cache_t::traverse_and_get_unconfirm_units(const std::string & address, uint64_t begin_height, std::vector<xblock_t *> & units, base::xauto_ptr<base::xvblock_t> & last_committed_unit) {
    xblock_t * block = dynamic_cast<xblock_t *>(last_committed_unit.get());
    block->add_ref();
    xassert(block->is_lightunit());

    do {
        units.push_back(block);

        data::xlightunit_block_t * lightunit = (data::xlightunit_block_t *)block->query_interface(data::xlightunit_block_t::get_object_type());
        xassert(lightunit != nullptr);
        if (lightunit->is_prev_sendtx_confirmed() || block->get_height() <= (begin_height + 1)) {
            xdbg("[unconfirm cache]read units finish. %s, units_size:%ld prev_sendtx_confirmed:%d height:%ld begin_height:%ld",
                 address.c_str(),
                 units.size(),
                 lightunit->is_prev_sendtx_confirmed(),
                 block->get_height(),
                 begin_height);
            break;
        }

        auto unit_tmp = m_para->get_vblockstore()->load_block_object(address, block->get_height() - 1);
        xassert(unit_tmp != nullptr);
        block = dynamic_cast<xblock_t *>(unit_tmp.get());
        block->add_ref();
    } while (1);
}

void xunconfirm_sendtx_cache_t::traverse_units_and_update_receipt_cache(const std::vector<xblock_t *> & units,
                                                                        xunconfirm_sendtx_account_cache_ptr_t & account_cache,
                                                                        uint64_t                                now) {
    xassert(units.size() != 0);

    // should drop old cache and rebuild new cache, when the begin unit height not continuous with old cache height
    xblock_t * begin_unit = *units.rbegin();
    xassert(begin_unit->get_height() > account_cache->get_cache_max_height());
    if (account_cache->get_cache_max_height() != 0 && begin_unit->get_height() > (account_cache->get_cache_max_height() + 1)) {
        account_cache->pop_all_receipt(m_table_addr, begin_unit->get_account());
    }

    for (auto iter = units.rbegin(); iter != units.rend(); iter++) {
        xblock_t * unit = *iter;
        data::xlightunit_block_t *           lightunit = dynamic_cast<data::xlightunit_block_t *>(unit);
        std::vector<xcons_transaction_ptr_t> sendtx_receipts;
        lightunit->create_send_txreceipts(sendtx_receipts);

        for (auto & send_receipt : sendtx_receipts) {
            account_cache->push_receipt(m_table_addr, unit->get_account(), unit->get_height(), send_receipt->get_transaction()->digest(), send_receipt);
            insert_retry_cache(send_receipt, send_receipt->get_transaction()->digest(), now);
        }

        const auto & txs = lightunit->get_txs();
        for (auto & confirm_tx : txs) {
            if (confirm_tx->is_confirm_tx()) {
                account_cache->pop_receipt(m_table_addr, unit->get_account(), unit->get_height(), confirm_tx->get_tx_hash_256());
            }
        }

        account_cache->set_cache_max_height(unit->get_height());
    }
}

void xunconfirm_sendtx_cache_t::insert_retry_cache(const xcons_transaction_ptr_t & receipt, const uint256_t & txhash, uint64_t now) {
    xunconfirm_sendtx_retry_cache_entry_t entry(receipt, txhash);
    refresh_receipt_retry_timestamp(entry, now);
    m_retry_cache.insert(entry);
    XMETRICS_COUNTER_INCREMENT("txpool_receipt_retry_cache", 1);
    m_unconfirm_tx_num++;
}

void xunconfirm_sendtx_cache_t::refresh_receipt_retry_timestamp(const xunconfirm_sendtx_retry_cache_entry_t & entry, uint64_t now) {
    entry.set_timestamp(get_next_retry_timestamp(entry.m_receipt->get_unit_cert()->get_gmtime(), now));
}

bool xunconfirm_sendtx_cache_t::is_account_cache_complete(const std::string & address) {
    auto iter = m_accounts_cache.find(address);
    if (iter == m_accounts_cache.end()) {
        xassert(0);  // should not appear
        return false;
    }
    return iter->second->cache_complete();
}

std::vector<xcons_transaction_ptr_t> xunconfirm_sendtx_cache_t::on_timer_check_cache(uint64_t now, xtxpool_receipt_receiver_counter & counter) {
    std::vector<xcons_transaction_ptr_t> cons_txs{};
    std::lock_guard<std::mutex>          lck(m_cache_mutex);
    if (m_retry_cache.empty()) {
        return {};
    }

    uint32_t process_count = 0;
    uint64_t cur_timer_height = m_para->get_chain_timer()->logic_time();
    // clear confirmed entries
    for (auto iter = m_retry_cache.begin(); iter != m_retry_cache.end();) {
        // always erase first, then insert again
        auto current_entry = *iter;

        if (current_entry.m_receipt->is_confirmed()) {
            // remove confirmed
            xdbg(" [unconfirm cache]on_timer_check_cache is_confirmed");
            XMETRICS_COUNTER_INCREMENT("txpool_receipt_retry_cache", -1);
            m_unconfirm_tx_num--;
            iter = m_retry_cache.erase(iter);
            continue;
        }

        if (current_entry.m_receipt->get_clock() + m_para->get_receipt_valid_window() < cur_timer_height) {
            xwarn("[unconfirm cache]receipt too old:table=%s,receipt=%s,timer_height=%llu,cur_timer_height:%llu",
                  m_table_addr.c_str(), current_entry.m_receipt->dump().c_str(), current_entry.m_receipt->get_clock(), cur_timer_height);
            XMETRICS_COUNTER_INCREMENT("txpool_receipt_retry_cache", -1);
            m_unconfirm_tx_num--;
            iter = m_retry_cache.erase(iter);
            continue;
        }
        iter++;
    }

    retry_cache_t tmp_retry_cache;
    // get time up entries to retry send
    for (auto iter = m_retry_cache.begin(); iter != m_retry_cache.end();) {
        if (process_count++ >= MAX_ONCE_PROCESS_RETRY_SENDTX_RECEIPT) {
            break;
        }

        if (iter->get_timestamp() > now) {
            xdbg(" [unconfirm cache]on_timer_check_cache timestamp:%llu, now:%llu", iter->get_timestamp(), now);
            break;
        }

        bool ret = is_account_cache_complete(iter->m_receipt->get_source_addr());
        if (ret) {
            if (!counter.is_receiver_count_full_and_insert(iter->m_receipt->get_source_addr())) {
                xdbg("[unconfirm cache]on_timer_check_cache retry send receipt:%s", iter->m_receipt->dump().c_str());
                cons_txs.push_back(iter->m_receipt);
                // for sort by timestamp, erase first, then modify retry timestamp, and insert again in the end
                tmp_retry_cache.insert(*iter);
                iter = m_retry_cache.erase(iter);
                continue;
            }
        }
        iter++;
    }

    for (auto & item : tmp_retry_cache) {
        refresh_receipt_retry_timestamp(item, now);
        m_retry_cache.insert(item);
    }

    return cons_txs;
}

uint64_t xunconfirm_sendtx_cache_t::get_account_cache_height(const std::string & address) const {
    auto account_cache = get_account_cache(address);
    if (account_cache != nullptr) {
        return account_cache->get_cache_max_height();
    }
    return 0;
}

void xunconfirm_sendtx_cache_t::on_unit_update(const std::string & address, uint64_t now) {
    std::lock_guard<std::mutex>           lck(m_cache_mutex);
    check_and_update_cache(address, now, 0);
}

xcons_transaction_ptr_t xunconfirm_sendtx_cache_t::get_unconfirm_tx(const std::string source_addr, const uint256_t & hash, uint64_t now) {
    std::lock_guard<std::mutex> lck(m_cache_mutex);
    check_and_update_cache(source_addr, now, 0);
    auto account_cache = get_account_cache(source_addr);
    if (account_cache == nullptr) {
        return nullptr;
    }
    return account_cache->get_tx(hash);
}

void xunconfirm_sendtx_cache_t::accounts_update(std::set<std::string> accounts, uint64_t now) {
    std::lock_guard<std::mutex> lck(m_cache_mutex);
    for (auto account : accounts) {
        check_and_update_cache(account, now, 0);
    }
}

NS_END2
