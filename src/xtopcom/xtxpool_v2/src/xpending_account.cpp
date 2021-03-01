// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xpending_account.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_base.h"
#include "xtxpool_v2/xtxpool_error.h"

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

#define xpending_accounts_num_max 80

int32_t xpending_accounts_t::push_tx(std::shared_ptr<xtx_entry> tx_ent) {
    std::string addr;
    int32_t ret = xsuccess;
    if (tx_ent->get_tx()->is_recv_tx()) {
        addr = tx_ent->get_tx()->get_target_addr();
    } else {
        addr = tx_ent->get_tx()->get_source_addr();
    }
    auto iter = m_account_map.find(addr);
    if (iter == m_account_map.end()) {
        if (m_account_map.size() > xpending_accounts_num_max) {
            return xtxpool_error_pending_reached_upper_limit;
        }
        std::shared_ptr<xcandidate_account_entry> account_ent = std::make_shared<xcandidate_account_entry>(addr);
        account_ent->add_tx(tx_ent);
        m_xtable_info->send_tx_inc(1);
        uint16_t selected_count = 0;
        m_account_selected_lru.get(addr, selected_count);
        account_ent->set_select_count(selected_count);
        auto iter = m_accounts_set.insert(account_ent);
        m_account_map[addr] = iter;
        xdbg("xpending_accounts_t::push_tx success tx:%s", tx_ent->get_tx()->dump().c_str());
    } else {
        ret = iter->second->get()->add_tx(tx_ent);
    }
    if (ret == xsuccess) {
        tx_count_inc(tx_ent->get_tx()->get_tx_subtype(), 1);
    }
    return ret;
}

std::shared_ptr<xtx_entry> xpending_accounts_t::pop_tx_by_hash(const std::string & account_addr, const uint256_t & hash, uint8_t subtype, int32_t err) {
    auto iter = m_account_map.find(account_addr);
    if (iter == m_account_map.end()) {
        return nullptr;
    } else {
        std::shared_ptr<xtx_entry> tx_ent = iter->second->get()->pop_tx_by_hash(hash, subtype, err);
        if (tx_ent != nullptr && iter->second->get()->empty()) {
            m_accounts_set.erase(iter->second);
            m_account_map.erase(iter);
            tx_count_inc(tx_ent->get_tx()->get_tx_subtype(), 1);
        }
        return tx_ent;
    }
}

candidate_accounts xpending_accounts_t::pop_accounts(uint32_t count) {
    candidate_accounts accounts;
    auto iter = m_accounts_set.begin();
    while (iter != m_accounts_set.end() && count > 0) {
        accounts.push_back(*iter);
        m_xtable_info->recv_tx_inc(-(*iter)->get_txs(enum_transaction_subtype_recv).size());
        m_xtable_info->conf_tx_inc(-(*iter)->get_txs(enum_transaction_subtype_confirm).size());
        m_xtable_info->send_tx_inc(-(*iter)->get_txs(enum_transaction_subtype_send).size());
        m_account_selected_lru.put((*iter)->get_addr(), (*iter)->get_select_count() + 1);
        m_account_map.erase((*iter)->get_addr());
        iter = m_accounts_set.erase(iter);
        count--;
    }
    return accounts;
}

const xcons_transaction_ptr_t xpending_accounts_t::query_tx(const std::string & account, const uint256_t & hash) const {
    auto iter = m_account_map.find(account);
    if (iter != m_account_map.end()) {
        xdbg("xpending_accounts_t::query_tx found account:%s", account.c_str());
        return iter->second->get()->query_tx(hash);
    }
    return nullptr;
}

void xpending_accounts_t::tx_count_inc(uint8_t subtype, int32_t count) {
    if (subtype == enum_transaction_subtype_recv) {
        m_xtable_info->recv_tx_inc(count);
    } else if (subtype == enum_transaction_subtype_recv) {
        m_xtable_info->conf_tx_inc(count);
    } else {
        m_xtable_info->send_tx_inc(count);
    }
}

}  // namespace xtxpool_v2
}  // namespace top
