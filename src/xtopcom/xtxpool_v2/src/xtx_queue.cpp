// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtx_queue.h"

#include "xbasic/xmodule_type.h"
#include "xdata/xtransaction.h"
#include "xtxpool_v2/xtxpool_error.h"

namespace top {
namespace xtxpool_v2 {

using namespace top::data;

#define account_send_tx_queue_size_max (16)
#define account_delay_count_default (1)

int32_t xtx_queue_t::push_tx(std::shared_ptr<xtx_entry> & tx_ent) {
    std::string hash_str = tx_ent->get_tx()->get_transaction()->get_digest_str();

    // todo:tx replace strategy

    if (tx_ent->get_tx()->is_self_tx() || tx_ent->get_tx()->is_send_tx()) {
        auto iter_m = m_send_tx_map.find(hash_str);
        if (iter_m != m_send_tx_map.end()) {
            return xtxpool_error_tx_duplicate;
        }

        if (m_xtable_info->is_send_tx_reached_upper_limit()) {
            return xtxpool_error_queue_reached_upper_limit;
        }

        auto iter_account = m_send_tx_accounts.find(tx_ent->get_tx()->get_source_addr());
        if (iter_account != m_send_tx_accounts.end()) {
            return push_send_tx(hash_str, tx_ent, iter_account->second);
        } else {
            return push_send_tx(hash_str, tx_ent);
        }
    } else {
        auto iter_m = m_receipt_map.find(hash_str);
        if (iter_m != m_receipt_map.end()) {
            return xtxpool_error_tx_duplicate;
        }
        if (tx_ent->get_tx()->is_recv_tx() && m_xtable_info->is_recv_tx_reached_upper_limit()) {
            return xtxpool_error_queue_reached_upper_limit;
        }

        return push_receipt(hash_str, tx_ent);
    }
}

std::vector<std::shared_ptr<xtx_entry>> xtx_queue_t::pop_send_txs(uint32_t count) {
    std::vector<std::shared_ptr<xtx_entry>> tx_ents;
    auto iter = m_send_tx_queue.begin();
    while (count > 0 && iter != m_send_tx_queue.end()) {
        // todo: clear expired send txs, maybe do it at "tx_executor"?
        if (m_send_tx_delay_accounts.find(iter->get()->get_tx()->get_source_addr()) != m_send_tx_delay_accounts.end()) {
            iter++;
            continue;
        }
        auto iter_account = m_send_tx_accounts.find(iter->get()->get_tx()->get_source_addr());
        xassert(iter_account != m_send_tx_accounts.end());
        auto & continuous_txs = iter_account->second;
        uint32_t max_idx = iter->get()->get_tx()->get_transaction()->get_tx_nonce() - continuous_txs.front()->get()->get_tx()->get_transaction()->get_tx_nonce();
        if (max_idx >= count) {
            iter++;
            continue;
        }
        for (uint32_t idx = 0; idx < max_idx; idx++) {
            tx_ents.push_back(*continuous_txs[idx]);
            erase_send_tx(continuous_txs[idx]);
        }
        tx_ents.push_back(*iter);
        iter = erase_send_tx(iter);
        continuous_txs.erase(continuous_txs.begin(), continuous_txs.begin() + max_idx + 1);
        if (continuous_txs.empty()) {
            m_send_tx_accounts.erase(iter_account);
        }
        count -= (max_idx + 1);
    }
    return tx_ents;
}

std::vector<std::shared_ptr<xtx_entry>> xtx_queue_t::pop_receipts(uint32_t count) {
    std::vector<std::shared_ptr<xtx_entry>> tx_ents;
    auto iter = m_receipt_queue.begin();
    while (count > 0 && iter != m_receipt_queue.end()) {
        if ((iter->get()->get_tx()->is_recv_tx() && m_recv_tx_delay_accounts.find(iter->get()->get_tx()->get_target_addr()) != m_recv_tx_delay_accounts.end()) ||
            (iter->get()->get_tx()->is_confirm_tx() && m_recv_tx_delay_accounts.find(iter->get()->get_tx()->get_source_addr()) != m_recv_tx_delay_accounts.end())) {
            iter++;
            continue;
        }

        tx_ents.push_back(*iter);
        iter = erase_receipt(iter);
        count--;
    }
    return tx_ents;
}

std::shared_ptr<xtx_entry> xtx_queue_t::pop_tx_by_hash(std::string account, const uint256_t & hash, uint8_t subtype) {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    std::shared_ptr<xtx_entry> tx_ent;
    if (subtype == enum_transaction_subtype_self || subtype == enum_transaction_subtype_send) {
        auto iter = m_send_tx_map.find(hash_str);
        if (iter == m_send_tx_map.end()) {
            return nullptr;
        }
        tx_ent = *(iter->second);
        auto iter_account = m_send_tx_accounts.find(tx_ent->get_tx()->get_source_addr());
        xassert(iter_account != m_send_tx_accounts.end());
        auto & continuous_txs = iter_account->second;

        uint32_t max_idx = tx_ent->get_tx()->get_transaction()->get_tx_nonce() - continuous_txs.front()->get()->get_tx()->get_transaction()->get_tx_nonce();
        // not consider about nonce and hash continuity here. delete txs whose nonce is less than the pop out tx
        for (uint32_t idx = 0; idx <= max_idx; idx++) {
            xwarn("xtx_queue_t::push_tx pop_tx_by_hash tx:%s", continuous_txs[idx]->get()->get_tx()->dump().c_str());
            erase_send_tx(continuous_txs[idx]);
        }
        continuous_txs.erase(continuous_txs.begin(), continuous_txs.begin() + max_idx + 1);
        if (continuous_txs.empty()) {
            m_send_tx_accounts.erase(iter_account);
        }
    } else {
        auto iter = m_receipt_map.find(hash_str);
        if (iter == m_receipt_map.end()) {
            return nullptr;
        }

        tx_ent = *(iter->second);
        erase_receipt(iter->second);
    }
    return tx_ent;
}

const xcons_transaction_ptr_t xtx_queue_t::query_tx(const std::string & account, const uint256_t & hash) const {
    std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    std::shared_ptr<xtx_entry> tx_ent;
    auto iter_s = m_send_tx_map.find(hash_str);
    if (iter_s != m_send_tx_map.end()) {
        return (*(iter_s->second))->get_tx();
    }
    auto iter_r = m_receipt_map.find(hash_str);
    if (iter_r != m_receipt_map.end()) {
        return (*(iter_r->second))->get_tx();
    }
    return nullptr;
}

int32_t xtx_queue_t::push_receipt(const std::string & hash_str, std::shared_ptr<xtx_entry> & tx_ent) {
    auto iter_q = m_receipt_queue.insert(tx_ent);
    m_receipt_map[hash_str] = iter_q;
    if (tx_ent->get_tx()->is_confirm_tx()) {
        m_xtable_info->conf_tx_inc(1);
    } else {
        m_xtable_info->recv_tx_inc(1);
    }
    return xsuccess;
}

int32_t xtx_queue_t::push_send_tx(const std::string & hash_str, std::shared_ptr<xtx_entry> & tx_ent) {
    // not find in m_send_tx_accounts, create one
    auto iter_q = m_send_tx_queue.insert(tx_ent);
    m_send_tx_map[hash_str] = iter_q;
    std::vector<xsend_tx_queue::iterator> continuous_txs;
    continuous_txs.push_back(iter_q);
    m_send_tx_accounts[tx_ent->get_tx()->get_source_addr()] = continuous_txs;
    m_xtable_info->send_tx_inc(1);
    return xsuccess;
}

int32_t xtx_queue_t::push_send_tx(const std::string & hash_str, std::shared_ptr<xtx_entry> & tx_ent, std::vector<xsend_tx_queue::iterator> & continuous_txs) {
    uint64_t new_tx_nonce = tx_ent->get_tx()->get_transaction()->get_tx_nonce();
    uint64_t cached_nonce_first = continuous_txs.front()->get()->get_tx()->get_transaction()->get_tx_nonce();
    uint64_t cached_nonce_last = continuous_txs.back()->get()->get_tx()->get_transaction()->get_tx_nonce();

    if (new_tx_nonce + 1 < cached_nonce_first) {
        // new send tx nonce is less than first continuous send tx nonce, and not continous with it, drop cached txs
        clear_old_send_txs(continuous_txs, 0, tx_ent);
    } else if (new_tx_nonce + 1 == cached_nonce_first) {
        // new send tx nonce is continuous with front of cached send txs

        if (!continuous_txs[0]->get()->get_tx()->get_transaction()->check_last_trans_hash(tx_ent->get_tx()->get_transaction()->digest())) {
            // hash not continuous!
            clear_old_send_txs(continuous_txs, 0, tx_ent);
        } else {
            if (continuous_txs.size() >= account_send_tx_queue_size_max) {
                xwarn("xtx_queue_t::push_tx send tx reached upper limit drop last continuous send tx:", continuous_txs.back()->get()->get_tx()->dump().c_str());
                erase_send_tx(continuous_txs.back());
                continuous_txs.pop_back();
            }
        }
        auto iter_q = m_send_tx_queue.insert(tx_ent);
        m_send_tx_map[hash_str] = iter_q;
        auto it = continuous_txs.begin();
        continuous_txs.insert(it, iter_q);
        m_xtable_info->send_tx_inc(1);
        return xsuccess;
    } else if (new_tx_nonce == cached_nonce_last + 1) {
        // new send tx nonce is continuous with back of cached send txs
        if (continuous_txs.size() >= account_send_tx_queue_size_max) {
            xwarn("xtx_queue_t::push_tx account send tx reached upper limit drop new tx:%s", tx_ent->get_tx()->dump().c_str());
            return xtxpool_error_account_send_txs_reached_upper_limit;
        }
    } else if (new_tx_nonce > cached_nonce_last + 1) {
        // new send tx nonce is more than first continuous send tx nonce, and not continous with it, drop new tx
        xwarn("xtx_queue_t::push_tx new tx:%s not continuous with last cached tx:%s ", tx_ent->get_tx()->dump().c_str(), continuous_txs.back()->get()->get_tx()->dump().c_str());
        return xtxpool_error_tx_nonce_incontinuity;
    } else {
        // new send tx nonce is same with one of the cached txs, replace the cached one
        uint32_t idx = new_tx_nonce - cached_nonce_first;
        xassert(idx >= 0 && idx < continuous_txs.size());
        xwarn("xtx_queue_t::push_tx same nonce, new tx:%s relpace cached tx:%s", tx_ent->get_tx()->dump().c_str(), continuous_txs[idx]->get()->get_tx()->dump().c_str());
        if (idx == 0) {
            if (tx_ent->get_para().get_charge_score() > (continuous_txs[0]->get()->get_para().get_charge_score() * 11 / 10)) {
                clear_old_send_txs(continuous_txs, 0, tx_ent);
            } else {
                return xtxpool_error_tx_nonce_duplicate;
            }
        } else {
            if (!tx_ent->get_tx()->get_transaction()->check_last_trans_hash(continuous_txs[idx - 1]->get()->get_tx()->get_transaction()->digest()) ||
                tx_ent->get_para().get_charge_score() <= (continuous_txs[idx]->get()->get_para().get_charge_score() * 11 / 10)) {
                return xtxpool_error_tx_nonce_duplicate;
            }
            clear_old_send_txs(continuous_txs, idx, tx_ent);
        }
    }

    auto iter_q = m_send_tx_queue.insert(tx_ent);
    m_send_tx_map[hash_str] = iter_q;
    continuous_txs.push_back(iter_q);
    m_xtable_info->send_tx_inc(1);
    return xsuccess;
}

xsend_tx_queue::iterator xtx_queue_t::erase_send_tx(xsend_tx_queue::iterator send_tx_it) {
    auto iter_m = m_send_tx_map.find(send_tx_it->get()->get_tx()->get_transaction()->get_digest_str());
    xassert(iter_m != m_send_tx_map.end());
    m_send_tx_map.erase(iter_m);
    m_xtable_info->send_tx_inc(-1);
    return m_send_tx_queue.erase(send_tx_it);
}

xreceipt_queue::iterator xtx_queue_t::erase_receipt(xreceipt_queue::iterator receipt_it) {
    auto iter_m = m_receipt_map.find(receipt_it->get()->get_tx()->get_transaction()->get_digest_str());
    xassert(iter_m != m_receipt_map.end());
    m_receipt_map.erase(iter_m);
    if (receipt_it->get()->get_tx()->is_confirm_tx()) {
        m_xtable_info->conf_tx_inc(-1);
    } else {
        m_xtable_info->recv_tx_inc(-1);
    }
    return m_receipt_queue.erase(receipt_it);
}

void xtx_queue_t::set_delay_accout(const std::string & account_addr, uint8_t subtype) {
    if (subtype == enum_transaction_subtype_recv) {
        m_recv_tx_delay_accounts[account_addr] = account_delay_count_default;
    } else if (subtype == enum_transaction_subtype_confirm) {
        m_conf_tx_delay_accounts[account_addr] = account_delay_count_default;
    } else {
        m_send_tx_delay_accounts[account_addr] = account_delay_count_default;
    }
}

void xtx_queue_t::refreash_delay_accounts() {
    refreash_delay_accounts(m_send_tx_delay_accounts);
    refreash_delay_accounts(m_recv_tx_delay_accounts);
    refreash_delay_accounts(m_conf_tx_delay_accounts);
}

void xtx_queue_t::refreash_delay_accounts(std::unordered_map<std::string, uint8_t> & accounts_map) {
    for (auto it = accounts_map.begin(); it != accounts_map.end();) {
        xassert(it->second > 0);
        it->second--;
        if (it->second == 0) {
            it = accounts_map.erase(it);
        } else {
            it++;
        }
    }
}

void xtx_queue_t::clear_old_send_txs(std::vector<xsend_tx_queue::iterator> & continuous_txs, uint32_t from_idx, const std::shared_ptr<xtx_entry> & tx_ent) {
    for (uint32_t i = continuous_txs.size(); i > from_idx; i--) {
        xwarn("xtx_queue_t::push_tx cached tx:%s not continuous with new tx:%s", continuous_txs[i -1]->get()->get_tx()->dump().c_str(), tx_ent->get_tx()->dump().c_str());
        erase_send_tx(continuous_txs[i - 1]);
        continuous_txs.pop_back();
    }
}

}  // namespace xtxpool_v2
}  // namespace top
