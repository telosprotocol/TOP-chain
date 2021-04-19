// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xlocked_txs.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"

namespace top {
namespace xtxpool_v2 {

const std::shared_ptr<xtx_entry> xlocked_txs_t::pop_tx(const tx_info_t & txinfo, bool & exist) {
    std::string hash_str = txinfo.get_hash_str();
    auto it = m_locked_tx_map.find(hash_str);
    if (it != m_locked_tx_map.end()) {
        auto lockec_tx = it->second;
        xdbg("xlocked_txs_t::pop_tx table:%s,account:%s tx:%s subtype:%d",
             m_xtable_info->get_table_addr().c_str(),
             txinfo.get_addr().c_str(),
             hash_str.c_str(),
             txinfo.get_subtype());
        m_locked_tx_map.erase(it);
        // m_xtable_info->conf_tx_dec(1);
        exist = true;
        return lockec_tx->get_tx_ent();
    }
    exist = false;
    return nullptr;
}

const bool xlocked_txs_t::try_push_tx(const std::shared_ptr<xtx_entry> & tx) {
    std::string hash_str = tx->get_tx()->get_transaction()->get_digest_str();
    auto it = m_locked_tx_map.find(hash_str);
    if (it != m_locked_tx_map.end()) {
        auto tx_ent = it->second->get_tx_ent();
        if (tx_ent == nullptr) {
            it->second->set_tx_ent(tx);
            // m_xtable_info->conf_tx_inc(1);
            xdbg("xlocked_txs_t::try_push_tx table:%s,tx:%s", m_xtable_info->get_table_addr().c_str(), tx->get_tx()->dump(true).c_str());
        }
        return true;
    }
    return false;
}

void xlocked_txs_t::update(const locked_tx_map_t & locked_tx_map, std::vector<std::shared_ptr<xtx_entry>> & unlocked_txs) {
    // old locked txs witch can be found from new locked txs, complement the new locked tx with original tx, if not, insert to unlocked_txs witch will be roll back to queue.
    locked_tx_map_t new_locked_tx_map = locked_tx_map;
    for (auto & old_locked_tx : m_locked_tx_map) {
        auto & tx_ent = old_locked_tx.second->get_tx_ent();
        if (tx_ent != nullptr) {
            auto it_new_locked_tx_map = new_locked_tx_map.find(old_locked_tx.first);
            if (it_new_locked_tx_map != new_locked_tx_map.end()) {
                it_new_locked_tx_map->second->set_tx_ent(tx_ent);
                xdbg("xlocked_txs_t::update table:%s,tx:%s still locked", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump(true).c_str());
            } else {
                xinfo("xlocked_txs_t::update table:%s,tx:%s unlocked,will push to pool again", m_xtable_info->get_table_addr().c_str(), tx_ent->get_tx()->dump(true).c_str());
                unlocked_txs.push_back(tx_ent);
            }
        }
    }
    m_locked_tx_map.swap(new_locked_tx_map);
}

}  // namespace xtxpool_v2
}  // namespace top
