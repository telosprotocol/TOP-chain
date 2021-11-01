// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xpending_account.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_utl.h"

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

#define xpending_accounts_num_max 256
#define xpending_account_send_tx_num_max 9
#define xpending_account_send_tx_pack_num_max 3

int32_t xcandidate_account_entry::push_tx(std::shared_ptr<xtx_entry> tx_ent, const std::string & table_addr) {
    if (m_txs.size() >= xpending_account_send_tx_num_max) {
        return xtxpool_error_pending_account_reached_upper_limit;
    }
    if (m_txs.size() > 0) {
        // send tx nonce must upwards!
        uint64_t new_tx_last_nonce = tx_ent->get_tx()->get_transaction()->get_last_nonce();
        uint64_t txs_back_nonce = m_txs[m_txs.size() - 1]->get_tx()->get_transaction()->get_tx_nonce();
        if (new_tx_last_nonce < txs_back_nonce) {
            return xtxpool_error_tx_nonce_expired;
        } else if (new_tx_last_nonce == txs_back_nonce) {
            m_txs.push_back(tx_ent);
        } else {
            return xtxpool_error_tx_nonce_uncontinuous;
        }
    } else {
        m_txs.push_back(tx_ent);
    }

    xtxpool_info("xxcandidate_account_entry::push_tx success push tx to pending,table:%s tx:%s", table_addr.c_str(), tx_ent->get_tx()->dump().c_str());
    return xsuccess;
}

const std::vector<std::shared_ptr<xtx_entry>> & xcandidate_account_entry::get_txs() const {
    return m_txs;
}

std::shared_ptr<xtx_entry> xcandidate_account_entry::pop_tx(const uint256_t & hash, bool clear_follower, const std::string & table_addr) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    auto it = find_tx_ent_by_hash(m_txs, hash);
    if (it == m_txs.end()) {
        return nullptr;
    }

    std::shared_ptr<xtx_entry> tx_ent = *it;
    uint32_t pos = it->get()->get_tx()->get_transaction()->get_tx_nonce() - m_txs.front()->get_tx()->get_transaction()->get_tx_nonce();
    if (clear_follower) {
        for (uint32_t i = pos; i < m_txs.size(); i++) {
            xtxpool_info("xcandidate_account_entry::pop_tx pop tx from pending table:%s clear fowllower,tx:%s", table_addr.c_str(), m_txs[i]->get_tx()->dump().c_str());
        }
        m_txs.erase(it, m_txs.end());
    } else {
        for (uint32_t i = 0; i <= pos; i++) {
            xtxpool_info("xcandidate_account_entry::pop_tx pop tx from pending table:%s clear ahead,tx:%s", table_addr.c_str(), m_txs[i]->get_tx()->dump().c_str());
        }
        m_txs.erase(m_txs.begin(), it + 1);
    }

    return tx_ent;
}

std::vector<std::shared_ptr<xtx_entry>>::iterator xcandidate_account_entry::find_tx_ent_by_hash(std::vector<std::shared_ptr<xtx_entry>> & txs, const uint256_t & hash) const {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    for (auto it = txs.begin(); it != txs.end(); it++) {
        if ((*it)->get_tx()->get_tx_hash() == hash_str) {
            return it;
        }
    }
    return txs.end();
}

void xcandidate_account_entry::updata_latest_nonce(uint64_t latest_nonce, const std::string & table_addr) {
    for (auto it = m_txs.begin(); it != m_txs.end();) {
        uint64_t tx_nonce = it->get()->get_tx()->get_transaction()->get_tx_nonce();
        if (tx_nonce <= latest_nonce) {
            xtxpool_info("xcandidate_account_entry::updata_latest_nonce pop tx from pending table:%s erase,tx:%s", table_addr.c_str(), it->get()->get_tx()->dump().c_str());
            it = m_txs.erase(it);
        } else {
            break;
        }
    }
    // if (!m_txs.empty() && latest_nonce != m_txs[0]->get_tx()->get_tx_last_nonce()) {
    //     xtxpool_error("xcandidate_account_entry::updata_latest_nonce latest nonce:%llu not match with first tx:%s", latest_nonce, m_txs[0]->get_tx()->dump().c_str());
    // }
}

const std::shared_ptr<xtx_entry> xcandidate_account_entry::find(const uint256_t & hash) const {
    auto it = find_tx_ent_by_hash(m_txs, hash);
    if (it != m_txs.end()) {
        return *it;
    }
    return nullptr;
}

void xcandidate_account_entry::set_select_count(uint32_t count) {
    m_selected_count = count;
}

void xcandidate_account_entry::set_select_count_inc(uint32_t count) {
    m_selected_count += count;
}

uint32_t xcandidate_account_entry::get_select_count() const {
    return m_selected_count;
}

const std::string & xcandidate_account_entry::get_addr() const {
    return m_account;
}

bool xcandidate_account_entry::empty() {
    return m_txs.empty();
}

void xcandidate_account_entry::clear_expired_txs(const std::string & table_addr) {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    int32_t ret = 0;
    for (auto it = m_txs.begin(); it != m_txs.end();) {
        if (ret == 0) {
            ret = xverifier::xtx_verifier::verify_tx_duration_expiration(it->get()->get_tx()->get_transaction(), now);
        }
        if (ret != 0) {
            xtxpool_info("xcandidate_account_entry::clear_expired_txs pop tx from pending table:%s erase tx:%s", table_addr.c_str(), it->get()->get_tx()->dump().c_str());
            it = m_txs.erase(it);
        } else {
            it++;
        }
    }
}

uint64_t xcandidate_account_entry::get_latest_nonce() const {
    return m_txs[0]->get_tx()->get_tx_last_nonce();
}

int32_t xpending_accounts_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    std::string addr = tx_ent->get_tx()->get_account_addr();
    int32_t ret = xsuccess;
    auto iter = m_account_map.find(addr);
    if (iter == m_account_map.end()) {
        if (m_account_map.size() > xpending_accounts_num_max) {
            return xtxpool_error_pending_reached_upper_limit;
        }
        std::shared_ptr<xcandidate_account_entry> account_ent = std::make_shared<xcandidate_account_entry>(addr);
        ret = account_ent->push_tx(tx_ent, m_xtable_info->get_table_addr());
        uint16_t selected_count = 0;
        m_account_selected_lru.get(addr, selected_count);
        account_ent->set_select_count(selected_count);
        auto iter = m_accounts_set.insert(account_ent);
        m_account_map[addr] = iter;
        xtxpool_dbg("xpending_accounts_t::push_tx table:%s, create account:%s tx:%s", m_xtable_info->get_table_addr().c_str(), addr.c_str(), tx_ent->get_tx()->dump().c_str());
    } else {
        auto & pending_account = *iter->second;
        ret = pending_account->push_tx(tx_ent, m_xtable_info->get_table_addr());
    }
    if (ret == xsuccess) {
        m_xtable_info->tx_inc(tx_ent->get_tx()->get_tx_subtype(), 1);
    }
    return ret;
}

std::shared_ptr<xtx_entry> xpending_accounts_t::pop_tx(const tx_info_t & txinfo, bool clear_follower) {
    auto iter = m_account_map.find(txinfo.get_addr());
    if (iter == m_account_map.end()) {
        return nullptr;
    } else {
        auto & pending_account = *iter->second;
        int32_t tx_num = pending_account->get_txs().size();
        int32_t delete_num = 0;
        std::shared_ptr<xtx_entry> tx_ent = pending_account->pop_tx(txinfo.get_hash(), clear_follower, m_xtable_info->get_table_addr());
        if (tx_ent != nullptr) {
            if (pending_account->empty()) {
                m_accounts_set.erase(iter->second);
                m_account_map.erase(iter);
                delete_num = tx_num;
            } else {
                delete_num = tx_num - pending_account->get_txs().size();
            }
            m_xtable_info->tx_dec(tx_ent->get_tx()->get_tx_subtype(), delete_num);
        }

        return tx_ent;
    }
}

ready_accounts_t xpending_accounts_t::pop_ready_accounts(uint32_t count) {
    ready_accounts_t accounts;
    auto iter = m_accounts_set.begin();
    while (iter != m_accounts_set.end() && count > 0) {
        std::shared_ptr<xready_account_t> account = std::make_shared<xready_account_t>(iter->get()->get_addr());
        for (auto & tx_ent : iter->get()->get_txs()) {
            account->put_tx(tx_ent->get_tx());
        }
        accounts.push_back(account);
        m_xtable_info->tx_dec(enum_transaction_subtype_send, (*iter)->get_txs().size());
        m_account_selected_lru.put((*iter)->get_addr(), (*iter)->get_select_count() + 1);
        m_account_map.erase((*iter)->get_addr());
        iter = m_accounts_set.erase(iter);
        count--;
    }
    return accounts;
}

ready_accounts_t xpending_accounts_t::get_ready_accounts(uint32_t txs_max_num, const data::xtablestate_ptr_t & table_state) {
    candidate_accounts_t c_accounts;
    ready_accounts_t accounts;
    auto iter = m_accounts_set.begin();
    uint32_t txs_num = 0;
    while (iter != m_accounts_set.end() && txs_num < txs_max_num) {
        std::shared_ptr<xcandidate_account_entry> acccount_ent = *iter;

        uint32_t account_tx_num = 0;
        uint64_t locked_nonce = 0;
        std::shared_ptr<xready_account_t> account = std::make_shared<xready_account_t>(acccount_ent->get_addr());
        // auto it_locked_nonce_map = locked_nonce_map.find(acccount_ent->get_addr());
        // if (it_locked_nonce_map != locked_nonce_map.end()) {
        //     locked_nonce = it_locked_nonce_map->second;
        // } else {
            base::xaccount_index_t account_index;
            table_state->get_account_index(acccount_ent->get_addr(), account_index);
            locked_nonce = account_index.get_latest_tx_nonce();
        // }
        for (auto & tx_ent : acccount_ent->get_txs()) {
            if (tx_ent->get_tx()->get_tx_nonce() > locked_nonce) {
                account->put_tx(tx_ent->get_tx());
                account_tx_num++;
                txs_num++;
                if (account_tx_num >= xpending_account_send_tx_pack_num_max || txs_num >= txs_max_num) {
                    break;
                }
            }
        }
        if (account_tx_num == 0) {
            iter++;
            continue;
        }
        accounts.push_back(account);

        acccount_ent->set_select_count_inc(1);
        c_accounts.push_back(acccount_ent);
        xtxpool_dbg("xpending_accounts_t::get_ready_accounts table:%s,account:%s,tx num:%u",
                    m_xtable_info->get_table_addr().c_str(),
                    acccount_ent->get_addr().c_str(),
                    acccount_ent->get_txs().size());
        m_account_selected_lru.put(acccount_ent->get_addr(), acccount_ent->get_select_count());
        m_account_map.erase(acccount_ent->get_addr());
        iter = m_accounts_set.erase(iter);
    }

    // reorder
    for (auto & it_c : c_accounts) {
        auto iter_s = m_accounts_set.insert(it_c);
        m_account_map[it_c->get_addr()] = iter_s;
    }
    return accounts;
}

const std::shared_ptr<xtx_entry> xpending_accounts_t::find(const std::string & account_addr, const uint256_t & hash) const {
    auto iter = m_account_map.find(account_addr);
    if (iter != m_account_map.end()) {
        xtxpool_dbg("xpending_accounts_t::query_tx found account:%s", account_addr.c_str());
        auto & pending_account = *iter->second;
        return pending_account->find(hash);
    }
    return nullptr;
}

void xpending_accounts_t::updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) {
    auto iter = m_account_map.find(account_addr);
    if (iter != m_account_map.end()) {
        auto & pending_account = *iter->second;
        int32_t tx_num = pending_account->get_txs().size();
        pending_account->updata_latest_nonce(latest_nonce, m_xtable_info->get_table_addr());
        m_xtable_info->tx_dec(enum_transaction_subtype_send, (tx_num - pending_account->get_txs().size()));
        if (pending_account->empty()) {
            m_accounts_set.erase(iter->second);
            m_account_map.erase(iter);
        }
    }
}

void xpending_accounts_t::clear_expired_txs() {
    uint32_t deleted_num = 0;

    for (auto it = m_accounts_set.begin(); it != m_accounts_set.end();) {
        auto & account_entry = *it;
        int32_t tx_num = account_entry->get_txs().size();
        account_entry->clear_expired_txs(m_xtable_info->get_table_addr());
        deleted_num += (tx_num - account_entry->get_txs().size());
        if (account_entry->empty()) {
            m_account_map.erase(account_entry->get_addr());
            it = m_accounts_set.erase(it);
        } else {
            it++;
        }
    }

    if (deleted_num > 0) {
        XMETRICS_GAUGE(metrics::txpool_send_tx_timeout, deleted_num);
        m_xtable_info->tx_dec(enum_transaction_subtype_send, deleted_num);
    }
}

bool xpending_accounts_t::get_account_nonce_cache(const std::string & account_addr, uint64_t & latest_nonce) const {
    auto iter = m_account_map.find(account_addr);
    if (iter != m_account_map.end()) {
        auto & pending_account = *iter->second;
        latest_nonce = pending_account->get_latest_nonce();
        return true;
    }
    return false;
}

}  // namespace xtxpool_v2
}  // namespace top
