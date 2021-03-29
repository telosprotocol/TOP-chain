// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xpending_account.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

#define xpending_accounts_num_max 256
#define xpending_account_send_tx_num_max 3
#define xpending_account_recv_tx_num_max 4
#define xpending_account_conf_tx_num_max 8

int32_t xcandidate_account_entry::push_tx(std::shared_ptr<xtx_entry> tx_ent) {
    enum_transaction_subtype new_tx_subtype = tx_ent->get_tx()->get_tx_subtype();
    if (new_tx_subtype == enum_transaction_subtype_self) {
        new_tx_subtype = enum_transaction_subtype_send;
    }

    if (m_subtype != 0 && new_tx_subtype != m_subtype) {
        return xtxpool_error_tx_not_same_type;
    }

    if ((m_subtype == enum_transaction_subtype_send && m_txs.size() >= xpending_account_send_tx_num_max) ||
        (m_subtype == enum_transaction_subtype_recv && m_txs.size() >= xpending_account_recv_tx_num_max) ||
        (m_subtype == enum_transaction_subtype_confirm && m_txs.size() >= xpending_account_conf_tx_num_max)) {
        return xtxpool_error_pending_account_reached_upper_limit;
    }
    if (m_subtype == enum_transaction_subtype_send && m_txs.size() > 0) {
        // send tx nonce must upwards!
        uint64_t new_tx_last_nonce = tx_ent->get_tx()->get_transaction()->get_last_nonce();
        uint64_t txs_back_nonce = m_txs[m_txs.size() - 1]->get_tx()->get_transaction()->get_tx_nonce();
        if (new_tx_last_nonce == txs_back_nonce) {
            if (tx_ent->get_tx()->get_transaction()->check_last_trans_hash(m_txs[m_txs.size() - 1]->get_tx()->get_transaction()->digest())) {
                m_txs.push_back(tx_ent);
            } else {
                return xtxpool_error_tx_last_hash_not_match;
            }
        } else if (new_tx_last_nonce < txs_back_nonce) {
            return xtxpool_error_tx_nonce_expired;
        } else {
            return xtxpool_error_tx_nonce_uncontinuous;
        }
    } else {
        m_txs.push_back(tx_ent);
    }
    m_subtype = new_tx_subtype;
    return xsuccess;
}

const std::vector<std::shared_ptr<xtx_entry>> & xcandidate_account_entry::get_txs() const {
    return m_txs;
}

std::shared_ptr<xtx_entry> xcandidate_account_entry::pop_tx(const uint256_t & hash, enum_transaction_subtype subtype, bool clear_follower) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    if (subtype == enum_transaction_subtype_self) {
        subtype = enum_transaction_subtype_send;
    }
    if (subtype != m_subtype) {
        return nullptr;
    }
    auto it = find_tx_ent_by_hash(m_txs, hash);
    if (it == m_txs.end()) {
        return nullptr;
    }

    std::shared_ptr<xtx_entry> tx_ent = *it;
    if (subtype != enum_transaction_subtype_send) {
        m_txs.erase(it);
    } else {
        uint32_t pos = it->get()->get_tx()->get_transaction()->get_tx_nonce() - m_txs.front()->get_tx()->get_transaction()->get_tx_nonce();
        if (clear_follower) {
            for (uint32_t i = pos; i < m_txs.size(); i++) {
                xtxpool_warn("xcandidate_account_entry::pop_tx clear fowllower,tx:%s", m_txs[i]->get_tx()->dump().c_str());
            }
            m_txs.erase(it, m_txs.end());
        } else {
            for (uint32_t i = 0; i <= pos; i++) {
                xtxpool_warn("xcandidate_account_entry::pop_tx clear fowllower,tx:%s", m_txs[i]->get_tx()->dump().c_str());
            }
            m_txs.erase(m_txs.begin(), it + 1);
        }
    }

    return tx_ent;
}

std::vector<std::shared_ptr<xtx_entry>>::iterator xcandidate_account_entry::find_tx_ent_by_hash(std::vector<std::shared_ptr<xtx_entry>> & txs, const uint256_t & hash) const {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    for (auto it = txs.begin(); it != txs.end(); it++) {
        if ((*it)->get_tx()->get_transaction()->get_digest_str() == hash_str) {
            return it;
        }
    }
    return txs.end();
}

void xcandidate_account_entry::updata_latest_nonce(uint64_t latest_nonce, const uint256_t & latest_hash) {
    if (m_subtype != enum_transaction_subtype_send) {
        return;
    }
    for (auto it = m_txs.begin(); it != m_txs.end();) {
        uint64_t tx_nonce = it->get()->get_tx()->get_transaction()->get_tx_nonce();
        if (tx_nonce <= latest_nonce) {
            it = m_txs.erase(it);
        } else if (tx_nonce == latest_nonce + 1) {
            if (it->get()->get_tx()->get_transaction()->check_last_trans_hash(latest_hash)) {
                break;
            } else {
                m_txs.clear();
                return;
            }
        } else {
            break;
        }
    }
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

uint32_t xcandidate_account_entry::get_select_count() const {
    return m_selected_count;
}

const std::string & xcandidate_account_entry::get_addr() const {
    return m_account;
}

bool xcandidate_account_entry::empty() {
    return m_txs.empty();
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
        account_ent->push_tx(tx_ent);
        uint16_t selected_count = 0;
        m_account_selected_lru.get(addr, selected_count);
        account_ent->set_select_count(selected_count);
        auto iter = m_accounts_set.insert(account_ent);
        m_account_map[addr] = iter;
        xtxpool_dbg("xpending_accounts_t::push_tx table:%s, create account:%s tx:%s", m_xtable_info->get_table_addr().c_str(), addr.c_str(), tx_ent->get_tx()->dump().c_str());
    } else {
        auto & pending_account = *iter->second;
        ret = pending_account->push_tx(tx_ent);
    }
    if (ret == xsuccess) {
        xtxpool_dbg("xpending_accounts_t::push_tx success table:%s, tx:%s", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump().c_str());
        tx_count_inc(tx_ent->get_tx()->get_tx_subtype(), 1);
    }
    return ret;
}

std::shared_ptr<xtx_entry> xpending_accounts_t::pop_tx(const std::string & account_addr, const uint256_t & hash, enum_transaction_subtype subtype, bool clear_follower) {
    auto iter = m_account_map.find(account_addr);
    if (iter == m_account_map.end()) {
        return nullptr;
    } else {
        auto & pending_account = *iter->second;
        int32_t tx_num = pending_account->get_txs().size();
        int32_t delete_num = 0;
        std::shared_ptr<xtx_entry> tx_ent = pending_account->pop_tx(hash, subtype, clear_follower);
        if (tx_ent != nullptr) {
            if (pending_account->empty()) {
                m_accounts_set.erase(iter->second);
                m_account_map.erase(iter);
                delete_num = tx_num;
            } else {
                delete_num = tx_num - pending_account->get_txs().size();
            }
            tx_count_inc(tx_ent->get_tx()->get_tx_subtype(), -delete_num);
            xtxpool_info("xpending_accounts_t::pop_tx table:%s tx:%s", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump().c_str());
        }

        return tx_ent;
    }
}

ready_accounts_t xpending_accounts_t::pop_ready_accounts(uint32_t count) {
    ready_accounts_t accounts;
    auto iter = m_accounts_set.begin();
    while (iter != m_accounts_set.end() && count > 0) {
        std::shared_ptr<xready_account_t> account = std::make_shared<xready_account_t>(iter->get()->get_addr());
        for (auto tx_ent : iter->get()->get_txs()) {
            account->put_tx(tx_ent->get_tx());
        }
        accounts.push_back(account);
        tx_count_inc((*iter)->get_subtype(), -(*iter)->get_txs().size());
        m_account_selected_lru.put((*iter)->get_addr(), (*iter)->get_select_count() + 1);
        m_account_map.erase((*iter)->get_addr());
        iter = m_accounts_set.erase(iter);
        count--;
    }
    return accounts;
}

ready_accounts_t xpending_accounts_t::get_ready_accounts(uint32_t count) {
    candidate_accounts_t c_accounts;
    auto iter = m_accounts_set.begin();
    while (iter != m_accounts_set.end() && count > 0) {
        std::shared_ptr<xcandidate_account_entry> acccount_ent = *iter;
        c_accounts.push_back(acccount_ent);
        xtxpool_dbg("xpending_accounts_t::get_ready_accounts table:%s,account:%s,tx num:%u",
             m_xtable_info->get_table_addr().c_str(),
             acccount_ent->get_addr().c_str(),
             acccount_ent->get_txs().size());
        m_account_selected_lru.put(acccount_ent->get_addr(), acccount_ent->get_select_count() + 1);
        m_account_map.erase(acccount_ent->get_addr());
        iter = m_accounts_set.erase(iter);
        count--;
    }

    // reorder
    ready_accounts_t accounts;
    for (auto it_c : c_accounts) {
        auto iter_s = m_accounts_set.insert(it_c);
        m_account_map[it_c->get_addr()] = iter_s;
        std::shared_ptr<xready_account_t> account = std::make_shared<xready_account_t>(it_c->get_addr());
        for (auto tx_ent : it_c->get_txs()) {
            account->put_tx(tx_ent->get_tx());
        }
        accounts.push_back(account);
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

void xpending_accounts_t::updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce, const uint256_t & latest_hash) {
    auto iter = m_account_map.find(account_addr);
    if (iter != m_account_map.end()) {
        auto & pending_account = *iter->second;
        if (pending_account->get_subtype() != enum_transaction_subtype_send) {
            return;
        }
        int32_t tx_num = pending_account->get_txs().size();
        pending_account->updata_latest_nonce(latest_nonce, latest_hash);
        tx_count_inc(enum_transaction_subtype_send, -(tx_num - pending_account->get_txs().size()));
        if (pending_account->empty()) {
            m_accounts_set.erase(iter->second);
            m_account_map.erase(iter);
        }
    }
}

void xpending_accounts_t::tx_count_inc(uint8_t subtype, int32_t count) {
    if (subtype == enum_transaction_subtype_recv) {
        m_xtable_info->recv_tx_inc(count);
    } else if (subtype == enum_transaction_subtype_confirm) {
        m_xtable_info->conf_tx_inc(count);
    } else {
        m_xtable_info->send_tx_inc(count);
    }
}

}  // namespace xtxpool_v2
}  // namespace top
