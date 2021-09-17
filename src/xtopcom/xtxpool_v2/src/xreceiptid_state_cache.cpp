// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xreceiptid_state_cache.h"

NS_BEG2(top, xtxpool_v2)

void xreceiptid_state_cache_t::update_table_receiptid_state(const base::xreceiptid_state_ptr_t & receiptid_state) {
    auto table_id = receiptid_state->get_self_tableid();
    std::lock_guard<std::mutex> lck(m_mutex);
    auto iter = m_receiptid_state_map.find(table_id);
    if (iter != m_receiptid_state_map.end()) {
        auto & old_receiptid_state = iter->second;
        if (receiptid_state->get_block_height() <= old_receiptid_state->get_block_height()) {
            return;
        }
    }
    xdbg("xreceiptid_state_cache_t::update_table_receiptid_state table:%d,height:%llu,pairs:%s",
         receiptid_state->get_self_tableid(),
         receiptid_state->get_block_height(),
         receiptid_state->get_all_receiptid_pairs()->dump().c_str());
    m_receiptid_state_map[table_id] = receiptid_state;
}

uint64_t xreceiptid_state_cache_t::get_confirmid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    auto iter = m_receiptid_state_map.find(table_id);
    if (iter != m_receiptid_state_map.end()) {
        auto & table_receiptid_state = iter->second;
        base::xreceiptid_pair_t pair;
        table_receiptid_state->find_pair(peer_table_id, pair);
        return pair.get_confirmid_max();
    }
    return 0;
}

uint64_t xreceiptid_state_cache_t::get_recvid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    auto iter = m_receiptid_state_map.find(table_id);
    if (iter != m_receiptid_state_map.end()) {
        auto & table_receiptid_state = iter->second;
        base::xreceiptid_pair_t pair;
        table_receiptid_state->find_pair(peer_table_id, pair);
        return pair.get_recvid_max();
    }
    return 0;
}

uint64_t xreceiptid_state_cache_t::get_sendid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    auto iter = m_receiptid_state_map.find(table_id);
    if (iter != m_receiptid_state_map.end()) {
        auto & table_receiptid_state = iter->second;
        base::xreceiptid_pair_t pair;
        table_receiptid_state->find_pair(peer_table_id, pair);
        return pair.get_sendid_max();
    }
    return 0;
}

uint64_t xreceiptid_state_cache_t::get_height(base::xtable_shortid_t table_id) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    auto iter = m_receiptid_state_map.find(table_id);
    if (iter != m_receiptid_state_map.end()) {
        auto & table_receiptid_state = iter->second;
        return table_receiptid_state->get_block_height();
    }
    return 0;
}

base::xreceiptid_state_ptr_t xreceiptid_state_cache_t::get_table_receiptid_state(base::xtable_shortid_t table_id) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    auto iter = m_receiptid_state_map.find(table_id);
    if (iter != m_receiptid_state_map.end()) {
        return iter->second;
    }
    return nullptr;
}

NS_END2
