// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xreceiptid_state_cache.h"

#include "xtxpool_v2/xtxpool_log.h"

NS_BEG2(top, xtxpool_v2)

void xtable_receiptid_info_t::update_table_receiptid_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const base::xreceiptid_state_ptr_t & receiptid_state) {
    m_property_prove_ptr = property_prove_ptr;
    m_receiptid_state = receiptid_state;
}
const base::xvproperty_prove_ptr_t & xtable_receiptid_info_t::get_property_prove() const {
    return m_property_prove_ptr;
}

const base::xreceiptid_state_ptr_t & xtable_receiptid_info_t::get_receiptid_state() const {
    return m_receiptid_state;
}

xunconfirm_tx_nums_t::xunconfirm_tx_nums_t() {
    std::map<base::enum_xchain_zone_index, uint16_t> const& all_table_indexs = base::xvledger_config_t::get_all_consensus_zone_subaddr_paris();
    for (auto & table_index : all_table_indexs) {
        m_unconfirm_nums[table_index.first].resize(table_index.second);
    }
}

void xunconfirm_tx_nums_t::add_unconfirm_tx_num(uint32_t zoneid, uint32_t subaddr, int32_t num) {
    if (num != 0) {
        // unconfirm num is not accurate, it can be a negative number, because receiptid state is not real-time.
        m_unconfirm_nums[zoneid][subaddr] += num;
        xdbg("xunconfirm_tx_nums_t::add_unconfirm_tx_num zone:%d,table:%d,num:%d,add num:%d", zoneid, subaddr, m_unconfirm_nums[zoneid][subaddr], num);        
    }
}

int32_t xunconfirm_tx_nums_t::get_unconfirm_tx_num(uint32_t zoneid, uint32_t subaddr) const {
    return m_unconfirm_nums[zoneid][subaddr];
}

xreceiptid_state_cache_t::xreceiptid_state_cache_t() {
    std::map<base::enum_xchain_zone_index, uint16_t> const& all_table_indexs = base::xvledger_config_t::get_all_consensus_zone_subaddr_paris();
    for (auto & table_index : all_table_indexs) {
        m_receiptid_infos[table_index.first].resize(table_index.second);
    }
}

uint64_t get_sendid_max_from_state(const base::xreceiptid_state_ptr_t & receiptid_state, base::xtable_shortid_t peer_table_id) {
    if (receiptid_state != nullptr) {
        base::xreceiptid_pair_t pair;
        receiptid_state->find_pair(peer_table_id, pair);
        return pair.get_sendid_max();
    }
    return 0;
}

uint64_t get_recvid_max_from_state(const base::xreceiptid_state_ptr_t & receiptid_state, base::xtable_shortid_t peer_table_id) {
    if (receiptid_state != nullptr) {
        base::xreceiptid_pair_t pair;
        receiptid_state->find_pair(peer_table_id, pair);
        return pair.get_recvid_max();
    }
    return 0;
}

uint64_t get_confirmid_max_from_state(const base::xreceiptid_state_ptr_t & receiptid_state, base::xtable_shortid_t peer_table_id) {
    if (receiptid_state != nullptr) {
        base::xreceiptid_pair_t pair;
        receiptid_state->find_pair(peer_table_id, pair);
        return pair.get_confirmid_max();
    }
    return 0;
}

uint64_t get_recvid_max_sum(const base::xreceiptid_state_ptr_t & receiptid_state) {
    uint64_t recvid_max_sum = 0;
    if (receiptid_state != nullptr) {
        auto & all_pairs = receiptid_state->get_all_receiptid_pairs()->get_all_pairs();
        for (auto & pair : all_pairs) {
            recvid_max_sum += pair.second.get_recvid_max();
        }
    }
    return recvid_max_sum;
}

void xreceiptid_state_cache_t::update_table_receiptid_state(const base::xvproperty_prove_ptr_t & property_prove_ptr, const base::xreceiptid_state_ptr_t & receiptid_state) {
    base::xtable_index_t table_index(receiptid_state->get_self_tableid());
    std::lock_guard<std::mutex> lck(m_mutex);
    auto & table_receiptid_info = m_receiptid_infos[table_index.get_zone_index()][table_index.get_subaddr()];
    auto old_receiptid_state = table_receiptid_info.get_receiptid_state();
    if (old_receiptid_state != nullptr) {
        if (receiptid_state->get_block_height() <= old_receiptid_state->get_block_height()) {
            return;
        }
    }
    xinfo("xreceiptid_state_cache_t::update_table_receiptid_state table:%d,height:%llu,pairs:%s",
          receiptid_state->get_self_tableid(),
          receiptid_state->get_block_height(),
          receiptid_state->get_all_receiptid_pairs()->dump().c_str());
    m_receiptid_infos[table_index.get_zone_index()][table_index.get_subaddr()].update_table_receiptid_state(property_prove_ptr, receiptid_state);

    for (uint32_t zone_idx = 0; zone_idx < xtxpool_zone_type_max; zone_idx++) {
        for (uint32_t subaddr = 0; subaddr < m_receiptid_infos[zone_idx].size(); subaddr++) {
            base::xtable_shortid_t peer_table_id = make_table_shortid(zone_idx, subaddr);
            int32_t add_num = get_sendid_max_from_state(receiptid_state, peer_table_id) - get_sendid_max_from_state(old_receiptid_state, peer_table_id);
            if (receiptid_state->get_self_tableid() == peer_table_id) {
                uint64_t inc_num = get_recvid_max_sum(receiptid_state) - get_recvid_max_sum(old_receiptid_state);
                add_num -= (int32_t)(inc_num);
            }
            m_unconfirm_tx_nums.add_unconfirm_tx_num(zone_idx, subaddr, add_num);
        }
    }
}

uint64_t xreceiptid_state_cache_t::get_confirmid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const {
    base::xtable_index_t table_index(table_id);
    std::lock_guard<std::mutex> lck(m_mutex);
    return get_confirmid_max_from_state(m_receiptid_infos[table_index.get_zone_index()][table_index.get_subaddr()].get_receiptid_state(), peer_table_id);
}

uint64_t xreceiptid_state_cache_t::get_recvid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const {
    base::xtable_index_t table_index(table_id);
    std::lock_guard<std::mutex> lck(m_mutex);
    return get_recvid_max_from_state(m_receiptid_infos[table_index.get_zone_index()][table_index.get_subaddr()].get_receiptid_state(), peer_table_id);
}

uint64_t xreceiptid_state_cache_t::get_sendid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const {
    base::xtable_index_t table_index(table_id);
    std::lock_guard<std::mutex> lck(m_mutex);
    return get_sendid_max_from_state(m_receiptid_infos[table_index.get_zone_index()][table_index.get_subaddr()].get_receiptid_state(), peer_table_id);
}

uint64_t xreceiptid_state_cache_t::get_height(base::xtable_shortid_t table_id) const {
    base::xtable_index_t table_index(table_id);
    std::lock_guard<std::mutex> lck(m_mutex);
    auto & table_receiptid_info = m_receiptid_infos[table_index.get_zone_index()][table_index.get_subaddr()];
    if (table_receiptid_info.get_receiptid_state() != nullptr) {
        return table_receiptid_info.get_receiptid_state()->get_block_height();
    }
    return 0;
}

base::xreceiptid_state_ptr_t xreceiptid_state_cache_t::get_table_receiptid_state(base::xtable_shortid_t table_id) const {
    base::xtable_index_t table_index(table_id);
    std::lock_guard<std::mutex> lck(m_mutex);
    auto & table_receiptid_info = m_receiptid_infos[table_index.get_zone_index()][table_index.get_subaddr()];
    if (table_receiptid_info.get_receiptid_state() != nullptr) {
        return table_receiptid_info.get_receiptid_state();
    }
    return nullptr;
}

// normal case for table A and table B
// section of A as sender pull from B：         (A confirm id, B recv id]
// section of A as sender for B pull from A：   (B recv id, A send id]
// left boundary for A as sender: if A have no unconfirm rsp id    ----> B recv id
//                                else                             ----> A confirm id
// section of A as receiver pull from B：       (A recv id, B send id]
// section of A as receiver for B pull from A： (B confirm id, A recv id]
// left boundary for A as receiver: if B have no unconfirm rsp id  ----> A recv id
//                                  else                           ----> B confirm id
void xreceiptid_state_cache_t::get_unconfirm_id_section_as_sender(base::xtable_shortid_t table_id,
                                                                  base::xtable_shortid_t peer_table_id,
                                                                  uint64_t & confirm_id,
                                                                  uint64_t & unconfirm_id_max,
                                                                  bool for_pull_lacking) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    uint64_t sendid_max = 0;
    uint64_t confirmid_max = 0;
    uint64_t recvid_max = 0;
    base::xtable_index_t table_index(table_id);
    base::xtable_index_t peer_table_index(peer_table_id);

    auto & peer_table_receiptid_info = m_receiptid_infos[peer_table_index.get_zone_index()][peer_table_index.get_subaddr()];
    if (peer_table_receiptid_info.get_receiptid_state() != nullptr) {
        base::xreceiptid_pair_t peer_pair;
        peer_table_receiptid_info.get_receiptid_state()->find_pair(table_id, peer_pair);
        recvid_max = peer_pair.get_recvid_max();
    }

    auto & table_receiptid_info = m_receiptid_infos[table_index.get_zone_index()][table_index.get_subaddr()];
    if (table_receiptid_info.get_receiptid_state() != nullptr) {
        base::xreceiptid_pair_t self_pair;
        table_receiptid_info.get_receiptid_state()->find_pair(peer_table_id, self_pair);
        sendid_max = self_pair.get_sendid_max();
        confirmid_max = self_pair.get_confirmid_max();

        if (self_pair.all_confirmed_as_sender()) {
            if (recvid_max > sendid_max) {
                // self state is fall behind
                confirm_id = sendid_max;
                unconfirm_id_max = sendid_max;
                return;
            } else if (recvid_max >= confirmid_max) {
                // normal
                confirm_id = recvid_max;
            } else {
                // peer state is fall behind
                confirm_id = confirmid_max;
            }
        } else {
            confirm_id = confirmid_max;
        }
    }

    if (for_pull_lacking) {
        unconfirm_id_max = (confirm_id > recvid_max) ? confirm_id : recvid_max;
    } else {
        unconfirm_id_max = sendid_max;
    }
}

void xreceiptid_state_cache_t::get_unconfirm_id_section_as_receiver(base::xtable_shortid_t table_id,
                                                                    base::xtable_shortid_t peer_table_id,
                                                                    uint64_t & confirm_id,
                                                                    uint64_t & unconfirm_id_max) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    uint64_t recvid_max = 0;
    uint64_t confirmid_max = 0;
    base::xtable_index_t table_index(table_id);
    base::xtable_index_t peer_table_index(peer_table_id);

    auto & table_receiptid_info = m_receiptid_infos[table_index.get_zone_index()][table_index.get_subaddr()];
    if (table_receiptid_info.get_receiptid_state() != nullptr) {
        base::xreceiptid_pair_t self_pair;
        table_receiptid_info.get_receiptid_state()->find_pair(peer_table_id, self_pair);
        recvid_max = self_pair.get_recvid_max();
    }

    auto & peer_table_receiptid_info = m_receiptid_infos[peer_table_index.get_zone_index()][peer_table_index.get_subaddr()];
    if (peer_table_receiptid_info.get_receiptid_state() != nullptr) {
        base::xreceiptid_pair_t peer_pair;
        peer_table_receiptid_info.get_receiptid_state()->find_pair(table_id, peer_pair);
        if (peer_pair.all_confirmed_as_sender()) {
            if (recvid_max < peer_pair.get_confirmid_max()) {
                // self state is fall behind
                confirm_id = peer_pair.get_confirmid_max();
                unconfirm_id_max = peer_pair.get_confirmid_max();
                return;
            } else if (peer_pair.get_sendid_max() >= recvid_max) {
                // normal
                confirm_id = recvid_max;
                unconfirm_id_max = recvid_max;
                return;                
            } else {
                // peer state is fall behind.
                confirm_id = peer_pair.get_sendid_max();
                unconfirm_id_max = recvid_max;
                return;
            }
        }
        confirmid_max = peer_pair.get_confirmid_max();
    }

    confirm_id = confirmid_max;
    unconfirm_id_max = recvid_max;
}

const xreceiptid_state_and_prove xreceiptid_state_cache_t::get_receiptid_state_and_prove(base::xtable_shortid_t self_table_id,
                                                                                         base::xtable_shortid_t peer_table_id,
                                                                                         uint64_t min_not_need_confirm_receiptid,
                                                                                         uint64_t max_not_need_confirm_receiptid) const {
    base::xtable_index_t peer_table_index(peer_table_id);
    std::lock_guard<std::mutex> lck(m_mutex);

    auto & peer_table_receiptid_info = m_receiptid_infos[peer_table_index.get_zone_index()][peer_table_index.get_subaddr()];
    if (peer_table_receiptid_info.get_receiptid_state() == nullptr || peer_table_receiptid_info.get_property_prove() == nullptr) {
        return {};
    }
    base::xreceiptid_pair_t peer_pair;
    peer_table_receiptid_info.get_receiptid_state()->find_pair(self_table_id, peer_pair);
    auto recvid_max = peer_pair.get_recvid_max();

    if (recvid_max >= min_not_need_confirm_receiptid && recvid_max <= max_not_need_confirm_receiptid) {
        xtxpool_info("xreceiptid_state_cache_t::get_receiptid_state_and_prove self:%d,peer:%d,recvid_max:%llu,min:%llu,max:%llu",
                     self_table_id,
                     peer_table_id,
                     recvid_max,
                     min_not_need_confirm_receiptid,
                     max_not_need_confirm_receiptid);
        return xreceiptid_state_and_prove(peer_table_receiptid_info.get_property_prove(), peer_table_receiptid_info.get_receiptid_state());
    }
    xtxpool_dbg("xreceiptid_state_cache_t::get_receiptid_state_and_prove fail self:%d,peer:%d,recvid_max:%llu,min:%llu,max:%llu",
                self_table_id,
                peer_table_id,
                recvid_max,
                min_not_need_confirm_receiptid,
                max_not_need_confirm_receiptid);
    return {};
}

xunconfirm_tx_nums_t xreceiptid_state_cache_t::get_unconfirm_tx_nums() const {
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_unconfirm_tx_nums;
}

NS_END2
