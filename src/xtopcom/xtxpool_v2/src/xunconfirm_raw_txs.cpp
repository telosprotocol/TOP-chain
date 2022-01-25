// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xunconfirm_raw_txs.h"

#include "xtxpool_v2/xtxpool_log.h"

NS_BEG2(top, xtxpool_v2)

void xunconfirm_raw_txs::add_raw_txs(std::vector<xraw_tx_info> raw_txs) {
    std::lock_guard<std::mutex> lck(m_mutex);
    for (auto & raw_tx_info : raw_txs) {
        xtxpool_dbg("xunconfirm_raw_txs::add_raw_txs add raw tx succ:%s,self table:%d,peer table:%d,receipt id:%llu",
                    raw_tx_info.m_raw_tx->dump().c_str(),
                    m_self_table_sid,
                    raw_tx_info.m_peer_table_sid,
                    raw_tx_info.m_receipt_id);
        auto it = m_raw_tx_cache.find(raw_tx_info.m_peer_table_sid);
        if (it == m_raw_tx_cache.end()) {
            std::map<uint64_t, data::xtransaction_ptr_t> id_tx_map;
            id_tx_map[raw_tx_info.m_receipt_id] = raw_tx_info.m_raw_tx;
            m_raw_tx_cache[raw_tx_info.m_peer_table_sid] = id_tx_map;
        } else {
            it->second[raw_tx_info.m_receipt_id] = raw_tx_info.m_raw_tx;
        }
    }
}

data::xtransaction_ptr_t xunconfirm_raw_txs::get_raw_tx(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    auto it = m_raw_tx_cache.find(peer_table_sid);
    if (it != m_raw_tx_cache.end()) {
        auto & id_tx_map = it->second;
        auto it_id_tx = id_tx_map.find(receipt_id);
        if (it_id_tx != id_tx_map.end()) {
            xtxpool_dbg("xunconfirm_raw_txs::get_raw_tx get raw tx succ:%s,self table:%d,peer table:%d,receipt id:%llu",
                        it_id_tx->second->dump().c_str(),
                        m_self_table_sid,
                        peer_table_sid,
                        receipt_id);
            return it_id_tx->second;
        }
    }

    xtxpool_dbg("xunconfirm_raw_txs::get_raw_tx get raw tx fail,self table:%d,peer table:%d:receipt id:%llu", m_self_table_sid, peer_table_sid, receipt_id);
    return nullptr;
}

void xunconfirm_raw_txs::refresh(base::xreceiptid_state_ptr_t table_receiptid_state) {
    std::lock_guard<std::mutex> lck(m_mutex);
    for (auto & raw_tx_cache_pair : m_raw_tx_cache) {
        auto & peer_table_sid = raw_tx_cache_pair.first;
        auto & id_tx_map = raw_tx_cache_pair.second;
        base::xreceiptid_pair_t pair;
        table_receiptid_state->find_pair(peer_table_sid, pair);
        auto max_confirm_id = pair.get_confirmid_max();
        for (auto it = id_tx_map.begin(); it != id_tx_map.end();) {
            auto & receipt_id = it->first;
            if (receipt_id <= max_confirm_id) {
                xtxpool_dbg("xunconfirm_raw_txs::refresh erase raw tx:%s,self table:%d,peer table:%d,max_confirm_id:%llu",
                            it->second->dump().c_str(),
                            m_self_table_sid,
                            peer_table_sid,
                            max_confirm_id);
                it = id_tx_map.erase(it);
            } else {
                break;
            }
        }
    }
}

uint32_t xunconfirm_raw_txs::size() const {
    uint32_t num = 0;
    std::lock_guard<std::mutex> lck(m_mutex);
    for (auto & cache_pair : m_raw_tx_cache) {
        num += cache_pair.second.size();
    }
    return num;
}

NS_END2
