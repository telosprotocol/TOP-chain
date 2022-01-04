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

// bool xreceiptid_state_cache_t::is_all_table_state_cached(const std::set<base::xtable_shortid_t> & all_table_sids) const {
//     std::lock_guard<std::mutex> lck(m_mutex);
//     if (m_all_cached) {
//         return true;
//     }
//     if (all_table_sids.size() != m_receiptid_state_map.size()) {
//         xdbg("xreceiptid_state_cache_t::is_all_table_state_cached all_table_sids size:%u,m_receiptid_state_map size:%u", all_table_sids.size(), m_receiptid_state_map.size());
//         return false;
//     }

//     for (auto & receiptid_state : m_receiptid_state_map) {
//         auto & table_sid = receiptid_state.first;
//         auto it = all_table_sids.find(table_sid);
//         if (it == all_table_sids.end()) {
//             xerror("xreceiptid_state_cache_t::is_all_table_state_cached a illegal table sid(%d) state founded!!!", table_sid);
//             return false;
//         }
//     }
//     xdbg("xreceiptid_state_cache_t::is_all_table_state_cached succ");
//     m_all_cached = true;
//     return true;
// }

void xreceiptid_state_cache_t::get_unconfirm_id_section_as_sender(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id, uint64_t & confirm_id, uint64_t & unconfirm_id_max) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    uint64_t sendid_max = 0;
    uint64_t confirmid_max = 0;
    auto iter_self = m_receiptid_state_map.find(table_id);
    if (iter_self != m_receiptid_state_map.end()) {
        auto & table_receiptid_state = iter_self->second;
        base::xreceiptid_pair_t pair;
        table_receiptid_state->find_pair(peer_table_id, pair);
        sendid_max = pair.get_sendid_max();
        confirmid_max = pair.get_confirmid_max();
    }

    confirm_id = confirmid_max;
    unconfirm_id_max = sendid_max;
}

void xreceiptid_state_cache_t::get_unconfirm_id_section_as_receiver(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id, uint64_t & confirm_id, uint64_t & unconfirm_id_max) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    uint64_t recvid_max = 0;
    uint64_t confirmid_max = 0;
    auto iter_self = m_receiptid_state_map.find(table_id);
    if (iter_self != m_receiptid_state_map.end()) {
        auto & table_receiptid_state = iter_self->second;
        base::xreceiptid_pair_t pair;
        table_receiptid_state->find_pair(peer_table_id, pair);
        recvid_max = pair.get_recvid_max();
    }

    auto iter_peer = m_receiptid_state_map.find(peer_table_id);
    if (iter_peer != m_receiptid_state_map.end()) {
        auto & table_receiptid_state = iter_peer->second;
        base::xreceiptid_pair_t pair;
        table_receiptid_state->find_pair(table_id, pair);
        confirmid_max = pair.get_confirmid_max();
    }

    confirm_id = confirmid_max;
    unconfirm_id_max = recvid_max;
}

NS_END2
