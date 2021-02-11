// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool/xaccountobj.h"

#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xdata/xlightunit.h"
#include "xconfig/xpredefined_configurations.h"
#include "xmetrics/xmetrics.h"
#include "xtxpool/xtxpool_error.h"
#include "xtxpool/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"

#include <assert.h>

NS_BEG2(top, xtxpool)

using namespace data;

xaccountobj_t::xaccountobj_t(const std::string & address, uint64_t account_tx_nonce, const uint256_t & account_tx_hash, tx_table * table)
  : m_address(address), m_latest_send_trans_number(account_tx_nonce), m_latest_send_trans_hash(account_tx_hash), m_table(table) {
    m_send_tx_queue_max_num = XGET_CONFIG(account_send_queue_tx_max_num);
    uint32_t unitblock_recv_transfer_tx_batch_num = XGET_CONFIG(unitblock_recv_transfer_tx_batch_num);
    xdbg("create accountobj:%s batch_tx_num:%d send_tx_queue_max_num:%d", address.c_str(), unitblock_recv_transfer_tx_batch_num, m_send_tx_queue_max_num);
}

xaccountobj_t::~xaccountobj_t() {
    for (auto iter = m_recv_ack_queue.begin(); iter != m_recv_ack_queue.end(); iter++) {
        pop_tx_event(iter->m_tx, false, xtxpool_error_accountobj_destroy, true);
    }
    for (auto iter = m_recv_queue.begin(); iter != m_recv_queue.end(); iter++) {
        pop_tx_event(iter->m_tx, false, xtxpool_error_accountobj_destroy, true);
    }
    for (auto iter = m_send_queue.begin(); iter != m_send_queue.end(); iter++) {
        pop_tx_event(iter->m_tx, true, xtxpool_error_accountobj_destroy, true);
    }
}

void xaccountobj_t::clear_send_queue_by_iter(std::multiset<xsendtx_queue_entry_t>::iterator iter, int32_t result) {
    uint32_t count = 0;
    while (iter != m_send_queue.end()) {
        // tx nonce duplicate, erase the old one(judge by timestamp).
        pop_tx_event(iter->m_tx, true, result, true);
        iter = m_send_queue.erase(iter);
        count++;
    }
}

int32_t xaccountobj_t::check_and_erase_old_nonce_duplicate_tx(const xtransaction_t * tx) {
    auto iter = m_send_queue.begin();
    while (iter != m_send_queue.end()) {
        if (tx->get_last_nonce() == iter->m_tx->get_transaction()->get_last_nonce()) {
            if (tx->get_fire_timestamp() < iter->m_tx->get_transaction()->get_fire_timestamp()) {
                // new tx is duplicate and timestamp is older, delete it.
                return xtxpool_error_tx_nonce_repeat;
            }
            break;
        }
        iter++;
    }

    // new tx is duplicate and timestamp is newer, delete old tx in sendqueue and followed txs
    clear_send_queue_by_iter(iter, xtxpool_error_tx_nonce_repeat);

    return xsuccess;
}

bool xaccountobj_t::check_send_tx(const xcons_transaction_ptr_t & tx, const uint256_t & latest_hash) {
    if (tx->get_transaction()->check_last_trans_hash(latest_hash)) {
        return true;
    }
    xwarn("xaccountobj_t::check_send_tx fail %s table:%s address:%s error:tx last nonce or hash not match, nonce:%llu hash:%lx:%lx %s",
          tx->get_transaction()->get_digest_hex_str().c_str(),
          m_table->get_table_name().c_str(),
          m_address.c_str(),
          tx->get_transaction()->get_last_nonce(),
          tx->get_transaction()->get_last_hash(),
          utl::xxh64_t::digest(latest_hash.data(), latest_hash.size()),
          data::to_hex_str(latest_hash).c_str());
    return false;
}

int32_t xaccountobj_t::push_send_tx(const xcons_transaction_ptr_t & tx) {
    xassert(tx->is_send_tx() || tx->is_self_tx());
    if (tx->get_transaction()->get_last_nonce() < m_latest_send_trans_number) {
        drop_invalid_tx(tx, xtxpool_error_tx_nonce_too_old);
        return xtxpool_error_tx_nonce_too_old;
    }

    // clear expired txs when push, so that need not timer to do clear thing
    clear_expire_txs(tx->get_transaction()->get_push_pool_timestamp());

    auto map_it = m_tx_map.find(tx->get_transaction()->get_digest_str());
    if (map_it != m_tx_map.end()) {
        drop_invalid_tx(tx, xtxpool_error_request_tx_repeat);
        return xtxpool_error_request_tx_repeat;
    }

    if (m_send_queue.empty()) {
        if (tx->get_transaction()->get_last_nonce() != m_latest_send_trans_number) {
            drop_invalid_tx(tx, xtxpool_error_tx_nonce_incontinuity);
            return xtxpool_error_tx_nonce_incontinuity;
        }
        if (!check_send_tx(tx, m_latest_send_trans_hash)) {
            drop_invalid_tx(tx, xtxpool_error_tx_last_hash_error);
            return xtxpool_error_tx_last_hash_error;
        }
    } else {
        auto iter = m_send_queue.rbegin();
        auto cons_tx_tmp = iter->m_tx->get_transaction();
        // tx nonce is continuous with the last tx in sendqueue
        if (tx->get_transaction()->get_last_nonce() == cons_tx_tmp->get_last_nonce() + 1) {
            // hash is not match, drop it
            if (!check_send_tx(tx, cons_tx_tmp->digest())) {
                drop_invalid_tx(tx, xtxpool_error_tx_last_hash_error);
                return xtxpool_error_tx_last_hash_error;
            }
        } else if (tx->get_transaction()->get_last_nonce() > cons_tx_tmp->get_last_nonce() + 1) {
            // tx nonce is incontinuity with sendqueue
            drop_invalid_tx(tx, xtxpool_error_tx_nonce_incontinuity);
            return xtxpool_error_tx_nonce_incontinuity;
        } else {
            // tx nonce is duplicate with one of sendqueue
            int32_t ret = check_and_erase_old_nonce_duplicate_tx(tx->get_transaction());
            if (ret != xsuccess) {
                drop_invalid_tx(tx, ret);
                return ret;
            }
        }

        // tx sendqueue is full, drop it
        if (m_send_queue.size() >= m_send_tx_queue_max_num) {
            drop_invalid_tx(tx, xtxpool_error_send_tx_queue_over_upper_limit);
            return xtxpool_error_send_tx_queue_over_upper_limit;
        }
    }

    m_tx_map[tx->get_transaction()->get_digest_str()] = tx;

    xsendtx_queue_entry_t entry(tx);

    m_send_queue.insert(entry);
    XMETRICS_FLOW_COUNT("txpool_push_tx_send_flow", 1);
    m_table->send_tx_increment(1);

    xkinfo("xaccountobj_t::tx_push success.table=%s,account=%s,tx=%s,send_queue_size:%ld,recv_queue_size:%ld,ack_queue_size:%ld",
           m_table->get_table_name().c_str(),
           m_address.c_str(),
           tx->dump(true).c_str(),
           m_send_queue.size(),
           m_recv_queue.size(),
           m_recv_ack_queue.size());
    return xsuccess;
}

int32_t xaccountobj_t::push_recv_tx(const xcons_transaction_ptr_t & tx) {
    xassert(tx->is_recv_tx());
    auto map_it = m_tx_map.find(tx->get_transaction()->get_digest_str());
    if (map_it != m_tx_map.end()) {
        drop_invalid_tx(tx, xtxpool_error_request_tx_repeat);
        return xtxpool_error_request_tx_repeat;
    }

    m_tx_map[tx->get_transaction()->get_digest_str()] = tx;

    xrecvtx_queue_entry_t entry(tx);

    m_recv_queue.insert(entry);
    XMETRICS_FLOW_COUNT("txpool_push_tx_recv_flow", 1);
    m_table->recv_tx_increment(1);

    xkinfo("xaccountobj_t::tx_push success.table=%s,account=%s,tx=%s,send_queue_size:%ld,recv_queue_size:%ld,ack_queue_size:%ld",
           m_table->get_table_name().c_str(),
           m_address.c_str(),
           tx->dump(true).c_str(),
           m_send_queue.size(),
           m_recv_queue.size(),
           m_recv_ack_queue.size());
    return 0;
}

int32_t xaccountobj_t::push_recv_ack_tx(const xcons_transaction_ptr_t & tx) {
    xassert(tx->is_confirm_tx());
    auto map_it = m_tx_map.find(tx->get_transaction()->get_digest_str());
    if (map_it != m_tx_map.end()) {
        drop_invalid_tx(tx, xtxpool_error_request_tx_repeat);
        return xtxpool_error_request_tx_repeat;
    }

    m_tx_map[tx->get_transaction()->get_digest_str()] = tx;

    xrecvtx_queue_entry_t entry(tx);

    m_recv_ack_queue.insert(entry);
    XMETRICS_FLOW_COUNT("txpool_push_tx_confirm_flow", 1);
    m_table->conf_tx_increment(1);

    xkinfo("xaccountobj_t::tx_push success.table=%s,account=%s,tx=%s,send_queue_size:%ld,recv_queue_size:%ld,ack_queue_size:%ld",
           m_table->get_table_name().c_str(),
           m_address.c_str(),
           tx->dump(true).c_str(),
           m_send_queue.size(),
           m_recv_queue.size(),
           m_recv_ack_queue.size());
    return 0;
}

xcons_transaction_ptr_t xaccountobj_t::get_tx() {
    if (false == m_recv_queue.empty()) {
        auto iter = m_recv_queue.begin();
        return iter->m_tx;
    }

    if (false == m_send_queue.empty()) {
        auto iter = m_send_queue.begin();
        return iter->m_tx;
    }

    return nullptr;
}

void xaccountobj_t::on_unit_to_db(xblock_t * block, uint64_t account_tx_nonce, const uint256_t & account_tx_hash) {
    uint64_t max_tx_nonce = account_tx_nonce;
    uint256_t max_tx_hash = account_tx_hash;
    if (block->is_lightunit() && !block->is_genesis_block()) {
        data::xlightunit_block_t * lightunit = dynamic_cast<data::xlightunit_block_t *>(block);
        const std::vector<xlightunit_tx_info_ptr_t> & txs = lightunit->get_txs();
        for (auto & tx : txs) {
            pop_tx_by_hash(tx->get_tx_hash_256(), tx->get_tx_subtype(), 0);
        }

        // blockchain maybe fall behind unit block to db event, update nonce by send/self tx here is a wise choice.
        uint64_t number;
        uint256_t hash;
        bool has_sendtx = lightunit->get_send_trans_info(number, hash);
        if (has_sendtx && number > max_tx_nonce) {
            max_tx_nonce = number;
            max_tx_hash = hash;
        }
    }

    if (false == update_latest_send_trans_number_hash(max_tx_nonce, max_tx_hash)) {
        xwarn("xaccountobj_t::on_unit_to_db fail nonce behind. account=%s,tx_nonce %ld %ld %ld,unit=%s",
            m_address.c_str(), account_tx_nonce, max_tx_nonce, m_latest_send_trans_number, block->dump().c_str());
    }
}

uint32_t xaccountobj_t::get_recv_ack_txs(std::vector<xcons_transaction_ptr_t> & cons_txs) {
    if (m_recv_ack_queue.empty()) {
        return 0;
    }

    uint32_t count = 0;
    auto     recv_ack_tx_batch_max_num = XGET_CONFIG(unitblock_confirm_tx_batch_num);
    for (auto iter = m_recv_ack_queue.begin(); iter != m_recv_ack_queue.end(); iter++) {
        xcons_transaction_ptr_t entry = iter->m_tx;
        cons_txs.push_back(entry);
        count++;
        if (count >= recv_ack_tx_batch_max_num) {
            break;
        }
    }

    return count;
}

uint32_t xaccountobj_t::get_recv_txs(std::vector<xcons_transaction_ptr_t> & cons_txs) {
    if (m_recv_queue.empty()) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t unitblock_recv_transfer_tx_batch_num = XGET_CONFIG(unitblock_recv_transfer_tx_batch_num);
    for (auto iter = m_recv_queue.begin(); iter != m_recv_queue.end(); iter++) {
        xcons_transaction_ptr_t entry = iter->m_tx;
        if (iter->m_tx->get_transaction()->get_tx_type() != xtransaction_type_transfer && count != 0) {
            break;
        }
        cons_txs.push_back(entry);
        count++;
        if (iter->m_tx->get_transaction()->get_tx_type() != xtransaction_type_transfer || count >= unitblock_recv_transfer_tx_batch_num) {
            break;
        }
    }
    return count;
}

bool xaccountobj_t::update_latest_send_trans_number_hash(uint64_t account_tx_nonce, const uint256_t & account_tx_hash) {
    if (account_tx_nonce < m_latest_send_trans_number) {
        xwarn("xaccountobj_t::update_latest_send_trans_number_hash table=%s,account=%s,nonce:hash %llu:%s %llu:%s",
            m_table->get_table_name().c_str(), m_address.c_str(), account_tx_nonce, to_hex_str(account_tx_hash).c_str(),
            m_latest_send_trans_number, to_hex_str(m_latest_send_trans_hash).c_str());
        return false;
    } else if (account_tx_nonce == m_latest_send_trans_number) {
        xassert(account_tx_hash == m_latest_send_trans_hash);
        return true;
    }

    m_latest_send_trans_number = account_tx_nonce;
    m_latest_send_trans_hash = account_tx_hash;

    auto     iter = m_send_queue.begin();
    uint32_t count = 0;
    while (iter != m_send_queue.end() && iter->m_tx->get_transaction()->get_last_nonce() < m_latest_send_trans_number) {
        pop_tx_event(iter->m_tx, true, xtxpool_error_tx_nonce_repeat, true);
        iter = m_send_queue.erase(iter);
        count++;
    }

    if (iter != m_send_queue.end() && !check_send_tx(iter->m_tx, m_latest_send_trans_hash)) {
        clear_send_queue_by_iter(iter, xtxpool_error_tx_last_hash_error);
    }
    return true;
}

uint32_t xaccountobj_t::get_send_txs(uint64_t account_tx_nonce, const uint256_t & account_tx_hash, std::vector<xcons_transaction_ptr_t> & cons_txs) {
    if (false == update_latest_send_trans_number_hash(account_tx_nonce, account_tx_hash)) {
        xwarn("xaccountobj_t::get_send_txs fail nonce behind. table=%s,account=%s,nonce:hash %llu:%s %llu:%s",
            m_table->get_table_name().c_str(), m_address.c_str(), account_tx_nonce, to_hex_str(account_tx_hash).c_str(),
            m_latest_send_trans_number, to_hex_str(m_latest_send_trans_hash).c_str());
        return 0;
    }

    if (m_send_queue.empty()) {
        return 0;
    }

    uint32_t count = 0;
    uint32_t unitblock_send_transfer_tx_batch_num = XGET_CONFIG(unitblock_send_transfer_tx_batch_num);

    auto iter = m_send_queue.begin();
    while (iter != m_send_queue.end()) {
        if (iter->m_tx->get_transaction()->get_tx_type() != xtransaction_type_transfer && count != 0) {
            break;
        }
        cons_txs.push_back(iter->m_tx);
        count++;
        // only transfer batch consensus
        if (iter->m_tx->get_transaction()->get_tx_type() != xtransaction_type_transfer || count >= unitblock_send_transfer_tx_batch_num) {
            break;
        }

        iter++;
    }
    xassert(count != 0);
    xassert(account_tx_nonce == cons_txs[0]->get_transaction()->get_last_nonce());
    xassert(cons_txs[0]->get_transaction()->check_last_trans_hash(account_tx_hash));
    return count;
}

std::vector<xcons_transaction_ptr_t> xaccountobj_t::get_cons_txs(uint64_t account_tx_nonce, const uint256_t & account_tx_hash) {
    std::vector<xcons_transaction_ptr_t> cons_txs;
    uint32_t                             count = 0;
    count = get_recv_ack_txs(cons_txs);
    if (count != 0) {
        xassert(cons_txs.size() == count);
        return cons_txs;
    }

    count = get_recv_txs(cons_txs);
    if (count != 0) {
        xassert(cons_txs.size() == count);
        return cons_txs;
    }

    get_send_txs(account_tx_nonce, account_tx_hash, cons_txs);
    return cons_txs;
}

void xaccountobj_t::clear_expire_txs(uint64_t now) {
    if (m_send_queue.empty()) {
        return;
    }

    auto iter = m_send_queue.begin();
    while (iter != m_send_queue.end()) {
        auto ret = xverifier::xtx_verifier::verify_tx_duration_expiration(iter->m_tx->get_transaction(), now);
        if (ret) {
            break;
        }
        iter++;
    }
    // if one send tx is expired, all txs in send queue followed should be pop out.
    while (iter != m_send_queue.end()) {
        pop_tx_event(iter->m_tx, true, xtxpool_error_tx_expired, true);
        iter = m_send_queue.erase(iter);
    }
}

void xaccountobj_t::clear_invalid_send_tx(uint64_t now) {
    clear_expire_txs(now);
}

xcons_transaction_ptr_t xaccountobj_t::get_tx_by_hash(const uint256_t & hash, uint8_t subtype) {
    auto map_it = m_tx_map.find(std::string(reinterpret_cast<char *>(hash.data()), hash.size()));
    if (map_it != m_tx_map.end()) {
        if (map_it->second->get_transaction()->get_tx_subtype() == subtype) {
            return map_it->second;
        }
    }
    return nullptr;
}

xcons_transaction_ptr_t xaccountobj_t::pop_tx() {
    if (false == m_recv_queue.empty()) {
        auto                    iter = m_recv_queue.begin();
        xcons_transaction_ptr_t tx = iter->m_tx;
        m_recv_queue.erase(iter);
        return tx;
    }

    if (false == m_send_queue.empty()) {
        auto                    iter = m_send_queue.begin();
        xcons_transaction_ptr_t tx = iter->m_tx;
        m_send_queue.erase(iter);
        return tx;
    }

    return nullptr;
}

xcons_transaction_ptr_t xaccountobj_t::pop_tx_by_hash(const uint256_t & hash, uint8_t subtype, int32_t result) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto map_it = m_tx_map.find(hash_str);
    if (map_it == m_tx_map.end()) {
        return nullptr;
    }

    xcons_transaction_ptr_t pop_tx = map_it->second;
    xtransaction_t *        tx = pop_tx->get_transaction();
    if (tx->get_tx_subtype() != subtype) {
        return nullptr;
    }

    m_tx_map.erase(map_it);
    if (tx->get_tx_subtype() == data::enum_transaction_subtype_send || tx->get_tx_subtype() == data::enum_transaction_subtype_self) {
        xsendtx_queue_entry_t entry(pop_tx);
        for (auto pos = m_send_queue.equal_range(entry); pos.first != pos.second; ++pos.first) {
            if (pos.first->m_tx->get_transaction()->get_digest_str() == hash_str) {
                pop_tx_event(pos.first->m_tx, true, result, false);
                m_send_queue.erase(pos.first);
                clear_send_queue_by_iter(pos.second, result);
                break;
            }
        }
    } else if (tx->get_tx_subtype() == data::enum_transaction_subtype_recv) {
        xrecvtx_queue_entry_t entry(pop_tx);
        for (auto pos = m_recv_queue.equal_range(entry); pos.first != pos.second; ++pos.first) {
            if (pos.first->m_tx->get_transaction()->get_digest_str() == hash_str) {
                pop_tx_event(pos.first->m_tx, false, result, false);
                m_recv_queue.erase(pos.first);
                break;
            }
        }
    } else if (tx->get_tx_subtype() == data::enum_transaction_subtype_confirm) {
        xrecvtx_queue_entry_t entry(pop_tx);
        for (auto pos = m_recv_ack_queue.equal_range(entry); pos.first != pos.second; ++pos.first) {
            if (pos.first->m_tx->get_transaction()->get_digest_str() == hash_str) {
                pop_tx_event(pos.first->m_tx, false, result, false);
                m_recv_ack_queue.erase(pos.first);
                break;
            }
        }
    }

    return pop_tx;
}

void xaccountobj_t::pop_tx_event(const xcons_transaction_ptr_t & cons_tx, bool is_send, int32_t result, bool clear_map) {
    xtransaction_t * tx = cons_tx->get_transaction();
    if (clear_map) {
        auto map_it = m_tx_map.find(tx->get_digest_str());
        if (map_it == m_tx_map.end()) {
            xwarn("xaccountobj_t not found table=%s,tx:%s", m_table->get_table_name().c_str(), cons_tx->dump().c_str());
            return;
        } else {
            m_tx_map.erase(tx->get_digest_str());
        }
    }

    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    uint64_t delay = now - tx->get_push_pool_timestamp();
    if (result != 0) {
        xwarn("xaccountobj_t::tx_pop fail,table=%s,account=%s,tx=%s,error:%s,send_queue_size:%d recv_queue_size:%d ack_queue_size:%ld delay:%ld",
              m_table->get_table_name().c_str(),
              m_address.c_str(),
              cons_tx->dump().c_str(),
              get_error_str(result).c_str(),
              m_send_queue.size(),
              m_recv_queue.size(),
              m_recv_ack_queue.size(),
              delay);
    } else {
        if (tx->get_tx_subtype() == data::enum_transaction_subtype_recv) {
            XMETRICS_COUNTER_INCREMENT("txpool_pop_tx_recv_total", 1);
            XMETRICS_COUNTER_INCREMENT("txpool_succ_recv_residence_time", delay);
        } else if (tx->get_tx_subtype() == data::enum_transaction_subtype_confirm) {
            XMETRICS_COUNTER_INCREMENT("txpool_pop_tx_confirm_total", 1);
            XMETRICS_COUNTER_INCREMENT("txpool_succ_confirm_residence_time", delay);
        } else {
            XMETRICS_COUNTER_INCREMENT("txpool_pop_tx_send_total", 1);
            XMETRICS_COUNTER_INCREMENT("txpool_succ_send_residence_time", delay);
        }
        xkinfo("xaccountobj_t::tx_pop success,table=%s,account=%s,tx=%s,send_queue_size:%d recv_queue_size:%d ack_queue_size:%ld delay:%ld",
            m_table->get_table_name().c_str(),
            m_address.c_str(),
            cons_tx->dump().c_str(),
            m_send_queue.size(),
            m_recv_queue.size(),
            m_recv_ack_queue.size(),
            delay);
    }

    if (tx->get_tx_subtype() == data::enum_transaction_subtype_recv) {
        m_table->recv_tx_increment(-1);
    } else if (tx->get_tx_subtype() == data::enum_transaction_subtype_confirm) {
        m_table->conf_tx_increment(-1);
    } else {
        m_table->send_tx_increment(-1);
    }
}

void xaccountobj_t::drop_invalid_tx(const xcons_transaction_ptr_t & cons_tx, int32_t result) {
    xwarn("xaccountobj_t::tx_push fail,table=%s,account=%s,tx=%s,error=%s,send_queue_size:%d recv_queue_size:%d ack_queue_size:%ld",
          m_table->get_table_name().c_str(),
          m_address.c_str(),
          cons_tx->dump().c_str(),
          get_error_str(result).c_str(),
          m_send_queue.size(),
          m_recv_queue.size(),
          m_recv_ack_queue.size());
    XMETRICS_COUNTER_INCREMENT("txpool_push_tx_fail", 1);
}

xcons_transaction_ptr_t xaccountobj_t::query_tx(const uint256_t & hash) const {
    auto map_it = m_tx_map.find(std::string(reinterpret_cast<char*>(hash.data()), hash.size()));
    if (map_it != m_tx_map.end()) {
        return map_it->second;
    }
    return nullptr;
}

NS_END2
