// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxpool_base.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xpending_account.h"
#include "xtxpool_v2/xtxpool_error.h"

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

#define xpending_account_send_tx_num_max 3
#define xpending_account_recv_tx_num_max 4
#define xpending_account_conf_tx_num_max 8

int32_t xcandidate_account_entry::add_tx(std::shared_ptr<xtx_entry> tx_ent) {
    if (tx_ent->get_tx()->is_self_tx() || tx_ent->get_tx()->is_send_tx()) {
        if (is_duplicate(m_send_txs, tx_ent, true)) {
            return xtxpool_error_tx_duplicate;
        }
        if (m_send_txs.size() >= xpending_account_send_tx_num_max) {
            return xtxpool_error_pending_account_reached_upper_limit;
        }
        if (m_send_txs.size() > 0) {
            if (tx_ent->get_tx()->get_transaction()->get_last_nonce() == m_send_txs[m_send_txs.size() - 1]->get_tx()->get_transaction()->get_last_nonce() + 1 &&
                tx_ent->get_tx()->get_transaction()->check_last_trans_hash(m_send_txs[m_send_txs.size() - 1]->get_tx()->get_transaction()->digest())) {
                m_send_txs.push_back(tx_ent);
            } else if (tx_ent->get_tx()->get_transaction()->get_last_nonce() == m_send_txs[0]->get_tx()->get_transaction()->get_last_nonce() - 1 &&
                       m_send_txs[0]->get_tx()->get_transaction()->check_last_trans_hash(tx_ent->get_tx()->get_transaction()->digest())) {
                m_send_txs.insert(m_send_txs.begin(), tx_ent);
            } else {
                return xtxpool_error_tx_nonce_incontinuity;
            }
        } else {
            m_send_txs.push_back(tx_ent);
        }
    } else if (tx_ent->get_tx()->is_recv_tx()) {
        if (is_duplicate(m_recv_txs, tx_ent, false)) {
            return xtxpool_error_tx_duplicate;
        }
        if (m_recv_txs.size() >= xpending_account_recv_tx_num_max) {
            return xtxpool_error_pending_account_reached_upper_limit;
        }
        m_recv_txs.push_back(tx_ent);
    } else {
        if (is_duplicate(m_conf_txs, tx_ent, true)) {
            return xtxpool_error_tx_duplicate;
        }
        if (m_conf_txs.size() >= xpending_account_conf_tx_num_max) {
            return xtxpool_error_pending_account_reached_upper_limit;
        }
        m_conf_txs.push_back(tx_ent);
    }
    return xsuccess;
}

const std::vector<std::shared_ptr<xtx_entry>> & xcandidate_account_entry::get_txs(enum_transaction_subtype subtype) const {
    if (subtype == enum_transaction_subtype_confirm) {
        return m_conf_txs;
    } else if (subtype == enum_transaction_subtype_recv) {
        return m_recv_txs;
    } else {
        return m_send_txs;
    }
}

std::shared_ptr<xtx_entry> xcandidate_account_entry::pop_tx_by_hash(const uint256_t & hash, uint8_t subtype, int32_t err) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    std::shared_ptr<xtx_entry> tx_ent = nullptr;
    if (subtype == enum_transaction_subtype_self || subtype == enum_transaction_subtype_send) {
        auto it = find_tx_ent_by_hash(m_send_txs, hash);
        if (it != m_send_txs.end()) {
            tx_ent = *it;
            m_send_txs.erase(m_send_txs.begin(), it + 1);
        }
    } else if (subtype == enum_transaction_subtype_recv) {
        auto it = find_tx_ent_by_hash(m_recv_txs, hash);
        if (it != m_recv_txs.end()) {
            tx_ent = *it;
            m_recv_txs.erase(it);
        }
    } else {
        auto it = find_tx_ent_by_hash(m_conf_txs, hash);
        if (it != m_conf_txs.end()) {
            tx_ent = *it;
            m_conf_txs.erase(it);
        }
    }
    return tx_ent;
}

std::vector<std::shared_ptr<xtx_entry>>::iterator xcandidate_account_entry::find_tx_ent_by_hash(std::vector<std::shared_ptr<xtx_entry>> & txs, const uint256_t & hash) const {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    for (auto it = txs.begin(); it != txs.end(); it++) {
        xdbg("xcandidate_account_entry::find_tx_ent_by_hash hash_str:%s, tx:%s", hash_str.c_str(), (*it)->get_tx()->get_transaction()->get_digest_str().c_str());
        if ((*it)->get_tx()->get_transaction()->get_digest_str() == hash_str) {
            return it;
        }
    }
    return txs.end();
}

const xcons_transaction_ptr_t xcandidate_account_entry::query_tx(const uint256_t & hash) const {
    auto it = find_tx_ent_by_hash(m_send_txs, hash);
    if (it != m_send_txs.end()) {
        return it->get()->get_tx();
    }
    it = find_tx_ent_by_hash(m_recv_txs, hash);
    if (it != m_recv_txs.end()) {
        return it->get()->get_tx();
    }
    it = find_tx_ent_by_hash(m_conf_txs, hash);
    if (it != m_conf_txs.end()) {
        return it->get()->get_tx();
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
    return m_send_txs.empty() && m_recv_txs.empty() && m_conf_txs.empty();
}

bool xcandidate_account_entry::is_duplicate(const std::vector<std::shared_ptr<xtx_entry>> & txs, std::shared_ptr<xtx_entry> tx, bool is_send_tx) const {
    for (auto it : txs) {
        if (it->get_tx()->get_transaction()->get_digest_str() == tx->get_tx()->get_transaction()->get_digest_str()) {
            return true;
        }
        if (is_send_tx && it->get_tx()->get_transaction()->get_last_nonce() == tx->get_tx()->get_transaction()->get_last_nonce()) {
            return true;
        }
    }
    return false;
}

}  // namespace xtxpool_v2
}  // namespace top
