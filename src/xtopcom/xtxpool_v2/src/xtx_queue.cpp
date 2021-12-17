// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtx_queue.h"

#include "xbasic/xmodule_type.h"
#include "xdata/xdatautil.h"
#include "xdata/xtransaction.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_utl.h"

namespace top {
namespace xtxpool_v2 {

using namespace top::data;

#define account_send_tx_queue_size_max (16)

#define account_send_tx_move_num_max (3)

void xsend_tx_queue_internal_t::insert_ready_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    tx_ent->get_tx()->set_push_pool_timestamp(now);
    auto it = m_ready_tx_queue.insert(tx_ent);
    m_ready_tx_map[tx_ent->get_tx()->get_tx_hash()] = it;
    m_xtable_info->send_tx_inc(1);
    xtxpool_info("xsend_tx_queue_internal_t::insert_ready_tx push tx to ready txs,table:%s,tx:%s", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump(true).c_str());
}

void xsend_tx_queue_internal_t::insert_non_ready_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    tx_ent->get_tx()->set_push_pool_timestamp(now);
    auto it = m_non_ready_tx_queue.insert(tx_ent);
    m_non_ready_tx_map[tx_ent->get_tx()->get_tx_hash()] = it;
    m_xtable_info->send_tx_inc(1);
    xtxpool_info("xsend_tx_queue_internal_t::insert_non_ready_tx push tx to non ready txs,table:%s,tx:%s", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump(true).c_str());
}

void xsend_tx_queue_internal_t::erase_ready_tx(const uint256_t & hash) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto it_ready = m_ready_tx_map.find(hash_str);
    if (it_ready != m_ready_tx_map.end()) {
        auto & tx_ent = *it_ready->second;
        uint64_t delay = xverifier::xtx_utl::get_gmttime_s() - tx_ent->get_tx()->get_push_pool_timestamp();
        xtxpool_info("xsend_tx_queue_internal_t::erase_ready_tx pop tx from ready txs,table:%s,tx:%s,delay:%llu",
                     m_xtable_info->get_table_addr().c_str(),
                     tx_ent->get_tx()->dump(true).c_str(),
                     delay);
        XMETRICS_GAUGE(metrics::txpool_tx_delay_from_push_to_commit_send, delay);
        m_ready_tx_queue.erase(it_ready->second);
        m_ready_tx_map.erase(it_ready);
        m_xtable_info->send_tx_dec(1);
        return;
    }
}

void xsend_tx_queue_internal_t::erase_non_ready_tx(const uint256_t & hash) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto it_non_ready = m_non_ready_tx_map.find(hash_str);
    if (it_non_ready != m_non_ready_tx_map.end()) {
        auto & tx_ent = *it_non_ready->second;
        uint64_t delay = xverifier::xtx_utl::get_gmttime_s() - tx_ent->get_tx()->get_push_pool_timestamp();
        xtxpool_info("xsend_tx_queue_internal_t::erase_non_ready_tx pop tx from non-ready txs,table:%s,tx:%s,delay:%llu",
                     m_xtable_info->get_table_addr().c_str(),
                     (*it_non_ready->second)->get_tx()->dump(true).c_str(),
                     delay);
        XMETRICS_GAUGE(metrics::txpool_tx_delay_from_push_to_commit_send, delay);
        m_non_ready_tx_queue.erase(it_non_ready->second);
        m_non_ready_tx_map.erase(it_non_ready);
        m_xtable_info->send_tx_dec(1);
        return;
    }
}

const std::shared_ptr<xtx_entry> xsend_tx_queue_internal_t::find(const uint256_t & hash) const {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto it_ready = m_ready_tx_map.find(hash_str);
    if (it_ready != m_ready_tx_map.end()) {
        return *it_ready->second;
    }
    auto it_non_ready = m_non_ready_tx_map.find(hash_str);
    if (it_non_ready != m_non_ready_tx_map.end()) {
        return *it_non_ready->second;
    }
    return nullptr;
}

const std::vector<std::shared_ptr<xtx_entry>> xsend_tx_queue_internal_t::get_expired_txs() const {
    std::vector<std::shared_ptr<xtx_entry>> expired_txs;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    for (auto & tx : m_non_ready_tx_queue) {
        auto ret = xverifier::xtx_verifier::verify_tx_duration_expiration(tx->get_tx()->get_transaction(), now);
        if (ret == 0) {
            break;
        }
        expired_txs.push_back(tx);
    }

    return expired_txs;
}

const std::shared_ptr<xtx_entry> xsend_tx_queue_internal_t::pick_to_be_droped_tx() const {
    if (m_non_ready_tx_queue.empty()) {
        return nullptr;
    }
    return *m_non_ready_tx_queue.begin();
}

int32_t xcontinuous_txs_t::nonce_check(uint64_t last_nonce) {
    if (last_nonce < m_latest_nonce) {
        return xtxpool_error_tx_nonce_expired;
    }
    if (last_nonce >= m_latest_nonce + account_send_tx_queue_size_max) {
        return xtxpool_error_tx_nonce_out_of_scope;
    }
    return xsuccess;
}

uint64_t xcontinuous_txs_t::get_back_nonce() const {
    if (m_txs.empty()) {
        return m_latest_nonce;
    }
    return m_txs.back()->get_tx()->get_transaction()->get_tx_nonce();
}

void xcontinuous_txs_t::update_latest_nonce(uint64_t latest_nonce) {
    if (latest_nonce <= m_latest_nonce) {
        return;
    }

    if (!m_txs.empty()) {
        uint64_t nonce_diff = latest_nonce - m_latest_nonce;
        uint32_t max_del_num = m_txs.size();
        if (nonce_diff < max_del_num) {
            max_del_num = nonce_diff;
        }
        batch_erase(0, max_del_num);
    }

    m_latest_nonce = latest_nonce;
}

void xcontinuous_txs_t::batch_erase(uint32_t from_idx, uint32_t to_idx) {
    for (uint32_t i = from_idx; i < to_idx; i++) {
        xtxpool_info("xcontinuous_txs_t::batch_erase delete tx:%s", m_txs[i]->get_tx()->dump().c_str());
        m_send_tx_queue_internal->erase_ready_tx(m_txs[i]->get_tx()->get_tx_hash_256());
    }
    m_txs.erase(m_txs.begin() + from_idx, m_txs.begin() + to_idx);
}

int32_t xcontinuous_txs_t::insert(std::shared_ptr<xtx_entry> tx_ent) {
    uint64_t new_tx_last_nonce = tx_ent->get_tx()->get_transaction()->get_last_nonce();
    int32_t ret = nonce_check(new_tx_last_nonce);
    if (ret != xsuccess) {
        return ret;
    }

    uint64_t back_nonce = get_back_nonce();

    // already checked hash duplication before, no need check again here.
    if (new_tx_last_nonce > back_nonce) {
        return xtxpool_error_tx_nonce_uncontinuous;
    } else if (new_tx_last_nonce == back_nonce) {
        m_txs.push_back(tx_ent);
        m_send_tx_queue_internal->insert_ready_tx(tx_ent);
    } else {
        // simple solution: nonce duplicate, drop the old tx.
        // todo: account tx replace strategy!
        uint32_t try_replace_idx = new_tx_last_nonce - m_latest_nonce;
        if (tx_ent->get_tx()->get_transaction()->get_fire_timestamp() <= m_txs[try_replace_idx]->get_tx()->get_transaction()->get_fire_timestamp()) {
            return xtxpool_error_tx_nonce_duplicate;
        }
        m_send_tx_queue_internal->erase_ready_tx(m_txs[try_replace_idx]->get_tx()->get_tx_hash_256());
        m_txs.at(try_replace_idx) = tx_ent;
        m_send_tx_queue_internal->insert_ready_tx(tx_ent);
        XMETRICS_GAUGE(metrics::txpool_push_send_fail_replaced, 1);
    }
    return xsuccess;
}

const std::vector<std::shared_ptr<xtx_entry>> xcontinuous_txs_t::get_txs(uint64_t upper_nonce, uint32_t max_num) const {
    xassert(upper_nonce > m_latest_nonce);
    if (upper_nonce > get_back_nonce() || upper_nonce - m_latest_nonce > max_num || m_txs.empty()) {
        return {};
    }
    std::vector<std::shared_ptr<xtx_entry>> txs;
    uint32_t nonce_diff = upper_nonce - m_latest_nonce;
    uint32_t max_insert_num = (max_num < nonce_diff) ? max_num : nonce_diff;
    txs.insert(txs.begin(), m_txs.begin(), m_txs.begin() + max_insert_num);
    return txs;
}

void xcontinuous_txs_t::erase(uint64_t nonce, bool clear_follower) {
    if (nonce <= m_latest_nonce || nonce > get_back_nonce()) {
        return;
    }
    uint32_t from_idx = 0;
    uint32_t to_idx = m_txs.size();  // not include
    // always keep nonce continuous, clear follower or clear ahead!
    if (clear_follower) {
        from_idx = nonce - m_latest_nonce - 1;

    } else {
        to_idx = nonce - m_latest_nonce;
    }

    batch_erase(from_idx, to_idx);
}

const std::vector<std::shared_ptr<xtx_entry>> xcontinuous_txs_t::pop_uncontinuous_txs() {
    if (m_txs.empty() || m_latest_nonce == m_txs.front()->get_tx()->get_transaction()->get_last_nonce()) {
        return {};
    }
    for (auto & tx : m_txs) {
        m_send_tx_queue_internal->erase_ready_tx(tx->get_tx()->get_tx_hash_256());
    }
    return std::move(m_txs);
}

int32_t xuncontinuous_txs_t::insert(std::shared_ptr<xtx_entry> tx_ent) {
    // already checked hash duplication before, no need check again here.
    uint64_t new_tx_nonce = tx_ent->get_tx()->get_transaction()->get_tx_nonce();
    auto it = m_txs.find(new_tx_nonce);
    if (it != m_txs.end()) {
        auto & tx_ent_tmp = it->second;
        if (tx_ent->get_tx()->get_transaction()->get_fire_timestamp() <= tx_ent_tmp->get_tx()->get_transaction()->get_fire_timestamp()) {
            return xtxpool_error_tx_nonce_duplicate;
        } else {
            m_send_tx_queue_internal->erase_non_ready_tx(tx_ent_tmp->get_tx()->get_tx_hash_256());
            m_txs.erase(it);
            XMETRICS_GAUGE(metrics::txpool_push_send_fail_replaced, 1);
        }
    }
    m_send_tx_queue_internal->insert_non_ready_tx(tx_ent);
    m_txs[new_tx_nonce] = tx_ent;
    return xsuccess;
}

const std::shared_ptr<xtx_entry> xuncontinuous_txs_t::pop_by_last_nonce(uint64_t last_nonce) {
    // erase all txs those with last nonce less than "last_nonce", and return tx with last nonce equal to "last_nonce".
    for (auto it = m_txs.begin(); it != m_txs.end();) {
        auto & tx_ent_tmp = it->second;
        auto raw_tx = tx_ent_tmp->get_tx()->get_transaction();
        if (raw_tx->get_last_nonce() < last_nonce) {
            m_send_tx_queue_internal->erase_non_ready_tx(raw_tx->digest());
            it = m_txs.erase(it);
        } else if (raw_tx->get_last_nonce() == last_nonce) {
            std::shared_ptr<xtx_entry> tx_ent = m_txs.begin()->second;
            m_send_tx_queue_internal->erase_non_ready_tx(raw_tx->digest());
            m_txs.erase(it);
            return tx_ent;
        } else {
            break;
        }
    }
    return nullptr;
}

void xuncontinuous_txs_t::erase(uint64_t nonce) {
    auto it = m_txs.find(nonce);
    if (it != m_txs.end()) {
        auto & tx_ent_tmp = it->second;
        m_send_tx_queue_internal->erase_non_ready_tx(tx_ent_tmp->get_tx()->get_tx_hash_256());
        m_txs.erase(it);
    }
}

int32_t xsend_tx_account_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    int32_t ret = m_continuous_txs.insert(tx_ent);
    if (ret == xsuccess) {
        try_continue();
    } else if (ret == xtxpool_error_tx_nonce_uncontinuous) {
        return m_uncontinuous_txs.insert(tx_ent);
    }
    return ret;
}

void xsend_tx_account_t::try_continue() {
    while (!m_uncontinuous_txs.empty()) {
        std::shared_ptr<xtx_entry> tx_ent = m_uncontinuous_txs.pop_by_last_nonce(m_continuous_txs.get_back_nonce());
        if (tx_ent == nullptr) {
            break;
        }
        int32_t ret = m_continuous_txs.insert(tx_ent);
        if (ret != xsuccess) {
            break;
        }
    }
}

void xsend_tx_account_t::update_latest_nonce(uint64_t latest_nonce) {
    m_continuous_txs.update_latest_nonce(latest_nonce);
    try_continue();
}

void xsend_tx_account_t::refresh() {
    std::vector<std::shared_ptr<xtx_entry>> txs = m_continuous_txs.pop_uncontinuous_txs();
    for (uint32_t i = 0; i < txs.size(); i++) {
        m_uncontinuous_txs.insert(txs[i]);
    }
}

const std::vector<std::shared_ptr<xtx_entry>> xsend_tx_account_t::get_continuous_txs(uint64_t upper_nonce, uint32_t max_num) const {
    return m_continuous_txs.get_txs(upper_nonce, max_num);
}

void xsend_tx_account_t::erase(uint64_t nonce, bool clear_follower) {
    // already checked tx is exist in queue, use nonce is enough to find tx here.
    m_continuous_txs.erase(nonce, clear_follower);
    m_uncontinuous_txs.erase(nonce);
}

int32_t xsend_tx_queue_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent, uint64_t latest_nonce) {
    clear_expired_txs();
    std::shared_ptr<xtx_entry> to_be_droped_tx = nullptr;
    int32_t err = m_send_tx_queue_internal.check_full();
    if (err != xsuccess) {
        to_be_droped_tx = m_send_tx_queue_internal.pick_to_be_droped_tx();
        if (to_be_droped_tx == nullptr) {
            return err;
        }
    }

    std::shared_ptr<xsend_tx_account_t> send_tx_account;
    auto & account_addr = tx_ent->get_tx()->get_source_addr();
    auto it = m_send_tx_accounts.find(account_addr);
    if (it == m_send_tx_accounts.end()) {
        send_tx_account = std::make_shared<xsend_tx_account_t>(&m_send_tx_queue_internal, latest_nonce);
        m_send_tx_accounts[account_addr] = send_tx_account;
    } else {
        send_tx_account = it->second;
        send_tx_account->update_latest_nonce(latest_nonce);
    }
    int32_t ret = send_tx_account->push_tx(tx_ent);
    if ((ret == xsuccess) && (to_be_droped_tx != nullptr)) {
        // in case of to_be_droped_tx maybe changed to a continuous tx, pick "to be droped tx" again and drop it,
        // if there is no uncontinuous tx, dorp "to_be_droped_tx".
        auto to_be_droped_tx_after_insert = m_send_tx_queue_internal.pick_to_be_droped_tx();
        if (to_be_droped_tx_after_insert != nullptr) {
            to_be_droped_tx = to_be_droped_tx_after_insert;
        }
        tx_info_t txinfo(to_be_droped_tx->get_tx());
        pop_tx(txinfo, true);
        if (to_be_droped_tx->get_tx()->get_tx_hash_256() == tx_ent->get_tx()->get_tx_hash_256()) {
            return err;
        }
    }
    if (send_tx_account->empty()) {
        m_account_nonce_lru.put(account_addr, send_tx_account->get_latest_nonce());
        m_send_tx_accounts.erase(account_addr);
    }
    return ret;
}

const std::vector<std::shared_ptr<xtx_entry>> xsend_tx_queue_t::get_txs(uint32_t max_num) const {
    std::map<std::string, std::vector<std::shared_ptr<xtx_entry>>> accounts_map;
    std::vector<std::shared_ptr<xtx_entry>> ret_txs;
    auto & ready_txs = m_send_tx_queue_internal.get_ready_queue();
    for (auto it_ready_tx = ready_txs.begin(); (ret_txs.size() < max_num) && (it_ready_tx != ready_txs.end()); it_ready_tx++) {
        // use nonce continuous peculiarity of send txs to deduplicate, if tx's account is found from tx_ents and nonce is bigger, update it in tx_ents.
        auto & account_addr = it_ready_tx->get()->get_tx()->get_source_addr();
        uint64_t nonce = it_ready_tx->get()->get_tx()->get_transaction()->get_tx_nonce();
        auto it_accounts_map = accounts_map.find(account_addr);
        uint32_t old_tx_num = 0;
        if (it_accounts_map != accounts_map.end()) {
            if (nonce <= it_accounts_map->second.back()->get_tx()->get_transaction()->get_tx_nonce()) {
                continue;
            } else {
                old_tx_num = it_accounts_map->second.size();
            }
        }
        auto send_tx_account = m_send_tx_accounts.find(account_addr);
        xassert(send_tx_account != m_send_tx_accounts.end());
        auto txs_tmp = send_tx_account->second->get_continuous_txs(nonce, account_send_tx_move_num_max);
        if (txs_tmp.size() <= old_tx_num) {
            continue;
        }
        accounts_map[account_addr] = txs_tmp;
        xtxpool_dbg("xsend_tx_queue_t::get_txs nonce:%llu,old_tx_num:%u,txs_tmp size=%u", nonce, old_tx_num, txs_tmp.size());
        ret_txs.insert(ret_txs.end(), txs_tmp.begin() + old_tx_num, txs_tmp.end());
    }
    return ret_txs;
}

const std::shared_ptr<xtx_entry> xsend_tx_queue_t::pop_tx(const tx_info_t & txinfo, bool clear_follower) {
    auto tx_ent = m_send_tx_queue_internal.find(txinfo.get_hash());
    if (tx_ent == nullptr) {
        return nullptr;
    }

    auto send_tx_account = m_send_tx_accounts.find(txinfo.get_addr());
    xassert(send_tx_account != m_send_tx_accounts.end());
    send_tx_account->second->erase(tx_ent->get_tx()->get_transaction()->get_tx_nonce(), clear_follower);
    send_tx_account->second->refresh();

    if (send_tx_account->second->empty()) {
        m_account_nonce_lru.put(txinfo.get_addr(), send_tx_account->second->get_latest_nonce());
        m_send_tx_accounts.erase(txinfo.get_addr());
    }
    return tx_ent;
}

const std::shared_ptr<xtx_entry> xsend_tx_queue_t::find(const std::string & account_addr, const uint256_t & hash) const {
    return m_send_tx_queue_internal.find(hash);
}

void xsend_tx_queue_t::updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) {
    auto send_tx_account = m_send_tx_accounts.find(account_addr);
    if (send_tx_account != m_send_tx_accounts.end()) {
        send_tx_account->second->update_latest_nonce(latest_nonce);
        if (send_tx_account->second->empty()) {
            m_account_nonce_lru.put(account_addr, latest_nonce);
            m_send_tx_accounts.erase(account_addr);
        }
    } else {
        m_account_nonce_lru.put(account_addr, latest_nonce);
    }
}

bool xsend_tx_queue_t::is_account_need_update(const std::string & account_addr) const {
    auto send_tx_account = m_send_tx_accounts.find(account_addr);
    if (send_tx_account != m_send_tx_accounts.end()) {
        return send_tx_account->second->need_update();
    }
    return false;
}

void xsend_tx_queue_t::clear_expired_txs() {
    auto expired_txs = m_send_tx_queue_internal.get_expired_txs();
    for (auto & tx : expired_txs) {
        tx_info_t txinfo(tx->get_tx());
        pop_tx(txinfo, false);
    }
}

bool xsend_tx_queue_t::get_account_nonce_cache(const std::string & account_addr, uint64_t & latest_nonce) const {
    auto send_tx_account = m_send_tx_accounts.find(account_addr);
    if (send_tx_account != m_send_tx_accounts.end()) {
        latest_nonce = send_tx_account->second->get_latest_nonce();
        return true;
    }
    uint64_t nonce;
    bool ret = m_account_nonce_lru.get(account_addr, nonce);
    if (ret) {
        latest_nonce = nonce;
    }
    return ret;
}

}  // namespace xtxpool_v2
}  // namespace top
