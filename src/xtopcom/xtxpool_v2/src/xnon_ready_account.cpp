// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xnon_ready_account.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"

namespace top {
namespace xtxpool_v2 {

int32_t xnon_ready_accounts_t::push_tx(const std::shared_ptr<xtx_entry> & tx_ent) {
    auto account_addr = tx_ent->get_tx()->get_account_addr();
    auto it = m_account_map.find(account_addr);
    if (it == m_account_map.end()) {
        std::map<std::string, std::shared_ptr<xtx_entry>> txs_map;
        txs_map[tx_ent->get_tx()->get_tx_hash()] = tx_ent;
        m_account_map[account_addr] = txs_map;
    } else {
        auto & txs_map = it->second;
        txs_map[tx_ent->get_tx()->get_tx_hash()] = tx_ent;
    }
    m_xtable_info->tx_inc(tx_ent->get_tx()->get_tx_subtype(), 1);
    return xsuccess;
}

const std::shared_ptr<xtx_entry> xnon_ready_accounts_t::pop_tx(const tx_info_t & txinfo) {
    auto it_account = m_account_map.find(txinfo.get_addr());
    if (it_account != m_account_map.end()) {
        std::string hash_str = txinfo.get_hash_str();
        auto & txs_map = it_account->second;
        auto it_tx = txs_map.find(hash_str);
        if (it_tx != txs_map.end()) {
            auto tx_ent = it_tx->second;
            if (txinfo.get_subtype() !=tx_ent->get_tx()->get_tx_subtype()) {
                xtxpool_dbg("xnon_ready_accounts_t::pop_tx subtype not match tx:%s, subtype:%d", tx_ent->get_tx()->dump().c_str(), txinfo.get_subtype());
                return nullptr;
            }
            txs_map.erase(it_tx);
            m_xtable_info->tx_dec(tx_ent->get_tx()->get_tx_subtype(), 1);
            return tx_ent;
        }
    }
    return nullptr;
}

const std::shared_ptr<xtx_entry> xnon_ready_accounts_t::find_tx(const std::string & account_addr, const uint256_t & hash) const {
    auto it_account = m_account_map.find(account_addr);
    if (it_account != m_account_map.end()) {
        std::string hash_str = std::string(reinterpret_cast<char *>(hash.data()), hash.size());
        auto & txs_map = it_account->second;
        auto it_tx = txs_map.find(hash_str);
        if (it_tx != txs_map.end()) {
            return it_tx->second;
        }
    }
    return nullptr;
}

const std::vector<std::string> xnon_ready_accounts_t::get_accounts() const {
    std::vector<std::string> accounts;
    for (auto account_txs : m_account_map) {
        accounts.push_back(account_txs.first);
    }
    return accounts;
}

const std::vector<std::shared_ptr<xtx_entry>> xnon_ready_accounts_t::pop_account_txs(const std::string & account_addr) {
    std::vector<std::shared_ptr<xtx_entry>> txs;
    auto it_account = m_account_map.find(account_addr);
    if (it_account != m_account_map.end()) {
        auto & txs_map = it_account->second;
        for (auto it_tx : txs_map) {
            txs.push_back(it_tx.second);
            m_xtable_info->tx_dec(it_tx.second->get_tx()->get_tx_subtype(), 1);
        }
        m_account_map.erase(account_addr);
    }
    return txs;
}

}  // namespace xtxpool_v2
}  // namespace top
