// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xunconfirm_id_height.h"

#include "xbase/xns_macro.h"
#include "xvledger/xvaccount.h"

#include <sstream>

NS_BEG2(top, xtxpool_v2)

void xunconfirm_id_height_list_t::update_confirm_id(uint64_t confirm_id) {
    if (confirm_id > m_confirm_id) {
        xdbg("xunconfirm_id_height_list_t::update_confirm_id confirmid selfid:%d,peerid:%d,as sender:%d,old:%llu,new:%llu",
             m_self_table_sid,
             m_peer_table_sid,
             m_as_sender,
             m_confirm_id,
             confirm_id);
        m_confirm_id = confirm_id;
        for (auto iter = m_id_height_map.begin(); iter != m_id_height_map.end();) {
            if (iter->first <= m_confirm_id) {
                if (iter->second > m_confirmed_height_max) {
                    m_confirmed_height_max = iter->second;
                }
                iter = m_id_height_map.erase(iter);
            } else {
                break;
            }
        }
    }
}

void xunconfirm_id_height_list_t::add_id_height(uint64_t receipt_id, uint64_t height, uint64_t time) {
    if (receipt_id > m_confirm_id) {
        xdbg("xunconfirm_id_height_list_t::add_id_height confirmid selfid:%d,peerid:%d,as sender:%d,receipt_id:%llu,height:%llu,m_confirm_id:%llu",
             m_self_table_sid,
             m_peer_table_sid,
             m_as_sender,
             receipt_id,
             height,
             m_confirm_id);
        m_id_height_map[receipt_id] = height;
        if (m_id_height_map.rbegin()->first == receipt_id) {
            m_update_time = time;
        }
    }
}

bool xunconfirm_id_height_list_t::is_lacking() const {
    if (m_id_height_map.empty()) {
        xdbg("xunconfirm_id_height_list_t::is_lacking m_id_height_map empty");
        return true;
    } else {
        xdbg("xunconfirm_id_height_list_t::is_lacking m_id_height_map size:%u,last receipt id:%llu,m_confirm_id:%llu",
             m_id_height_map.size(),
             m_id_height_map.rbegin()->first,
             m_confirm_id);
        return !(m_id_height_map.size() == (m_id_height_map.rbegin()->first - m_confirm_id));
    }
}

bool xunconfirm_id_height_list_t::get_min_height(uint64_t & min_height) const {
    auto min_unconfirmid = m_confirm_id + 1;
    auto it = m_id_height_map.find(min_unconfirmid);
    if (it != m_id_height_map.end()) {
        min_height = it->second;

        xdbg("xunconfirm_id_height_list_t::get_min_height succ selfid:%d,peerid:%d,as sender:%d,min_unconfirmid:%llu,height:%llu",
             m_self_table_sid,
             m_peer_table_sid,
             m_as_sender,
             min_unconfirmid,
             min_height);
        return true;
    } else {
        if (m_confirmed_height_max != 0) {
            min_height = m_confirmed_height_max + 1;
            xdbg("xunconfirm_id_height_list_t::get_min_height succ use latest confirmed height selfid:%d,peerid:%d,as sender:%d,min_unconfirmid:%llu,height:%llu",
                 m_self_table_sid,
                 m_peer_table_sid,
                 m_as_sender,
                 min_unconfirmid,
                 min_height);
            return true;
        }
    }

    xdbg("xunconfirm_id_height_list_t::get_min_height fail selfid:%d,peerid:%d,as sender:%d,m_confirm_id:%llu", m_self_table_sid, m_peer_table_sid, m_as_sender, m_confirm_id);
    return false;
}

bool xunconfirm_id_height_list_t::get_height_by_id(uint64_t receipt_id, uint64_t & height) const {
    auto iter = m_id_height_map.find(receipt_id);
    if (iter == m_id_height_map.end()) {
        return false;
    }
    height = iter->second;

    return true;
}

bool xunconfirm_id_height_list_t::get_resend_id_height(uint64_t & receipt_id, uint64_t & height, uint64_t cur_time) const {
    if (m_update_time == 0 || (cur_time < m_update_time + resend_interval_min)) {
        xdbg("xunconfirm_id_height_list_t::get_resend_id_height m_update_time:%llu,cur_time:%llu", m_update_time, cur_time);
        return false;
    }

    auto iter = m_id_height_map.rbegin();
    if (iter != m_id_height_map.rend()) {
        receipt_id = iter->first;
        height = iter->second;
        return true;
    }
    return false;
}

uint32_t xunconfirm_id_height_list_t::size() const {
    return m_id_height_map.size();
}

void xtable_unconfirm_id_height_t::update_confirm_id(base::xtable_shortid_t peer_table_sid, uint64_t confirm_id) {
    auto iter = m_table_sid_unconfirm_list_map.find(peer_table_sid);
    if (iter == m_table_sid_unconfirm_list_map.end()) {
        auto unconfirm_list = std::make_shared<xunconfirm_id_height_list_t>(m_self_table_sid, peer_table_sid, is_sender());
        unconfirm_list->update_confirm_id(confirm_id);
        m_table_sid_unconfirm_list_map[peer_table_sid] = unconfirm_list;
        return;
    }
    iter->second->update_confirm_id(confirm_id);
}

void xtable_unconfirm_id_height_t::add_id_height(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, uint64_t height, uint64_t time) {
    auto iter = m_table_sid_unconfirm_list_map.find(peer_table_sid);
    if (iter == m_table_sid_unconfirm_list_map.end()) {
        auto unconfirm_list = std::make_shared<xunconfirm_id_height_list_t>(m_self_table_sid, peer_table_sid, is_sender());
        unconfirm_list->add_id_height(receipt_id, height, time);
        m_table_sid_unconfirm_list_map[peer_table_sid] = unconfirm_list;
        return;
    }
    iter->second->add_id_height(receipt_id, height, time);
}

bool xtable_unconfirm_id_height_t::get_min_height(const xreceiptid_state_cache_t & receiptid_state_cache,
                                                  const std::set<base::xtable_shortid_t> & all_table_sids,
                                                  uint64_t self_height,
                                                  uint64_t & min_height,
                                                  bool check_lacking,
                                                  bool & is_lacking) const {
    bool is_lacking_tmp = false;
    uint64_t min_tmp = self_height;
    for (auto & peer_table_sid : all_table_sids) {
        uint64_t confirm_id;
        uint64_t unconfirm_id_max;
        get_unconfirm_id_section(receiptid_state_cache, m_self_table_sid, peer_table_sid, confirm_id, unconfirm_id_max);
        xdbg("xtable_unconfirm_id_height_t::get_min_height_as_sender selfid:%d,peerid:%d,as sender:%d,confirm_id:%llu,unconfirm_id_max:%llu,self_height:%llu",
                     m_self_table_sid,
                     peer_table_sid,
                     is_sender(),
                     confirm_id,
                     unconfirm_id_max,
                     self_height);
        if (confirm_id < unconfirm_id_max) {
            auto it = m_table_sid_unconfirm_list_map.find(peer_table_sid);
            if (it == m_table_sid_unconfirm_list_map.end()) {
                xdbg("xtable_unconfirm_id_height_t::get_min_height_as_sender fail,selfid:%d,peerid:%d,as sender:%d,confirm_id:%llu,unconfirm_id_max:%llu,self_height:%llu",
                     m_self_table_sid,
                     peer_table_sid,
                     is_sender(),
                     confirm_id,
                     unconfirm_id_max,
                     self_height);
                if (check_lacking) {
                    is_lacking = true;
                }
                return false;
            }
            uint64_t min_height_tmp;
            it->second->update_confirm_id(confirm_id);
            if (check_lacking && !is_lacking_tmp) {
                is_lacking_tmp = it->second->is_lacking();
            }
            auto ret_min_height = it->second->get_min_height(min_height_tmp);
            if (!ret_min_height) {
                xdbg("xtable_unconfirm_id_height_t::get_min_height_as_sender fail,selfid:%d,peerid:%d,as sender:%d,confirm_id:%llu,unconfirm_id_max:%llu,self_height:%llu",
                     m_self_table_sid,
                     peer_table_sid,
                     is_sender(),
                     confirm_id,
                     unconfirm_id_max,
                     self_height);
                if (check_lacking) {
                    is_lacking = true;
                }
                return false;
            }
            xdbg("xtable_unconfirm_id_height_t::get_min_height_as_sender succ,selfid:%d,peerid:%d,as sender:%d,confirm_id:%llu,unconfirm_id_max:%llu,height:%llu,self_height:%llu",
                 m_self_table_sid,
                 peer_table_sid,
                 is_sender(),
                 confirm_id,
                 unconfirm_id_max,
                 min_height_tmp,
                 self_height);
            min_tmp = (min_tmp < min_height_tmp) ? min_tmp : min_height_tmp;
        } else {
            continue;
        }
    }

    min_height = min_tmp;
    if (check_lacking) {
        is_lacking = is_lacking_tmp;
    }
    return true;
}

bool xtable_unconfirm_id_height_t::get_height_by_id(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, uint64_t & height) const {
    auto iter = m_table_sid_unconfirm_list_map.find(peer_table_sid);
    if (iter != m_table_sid_unconfirm_list_map.end()) {
        return iter->second->get_height_by_id(receipt_id, height);
    }
    return false;
}

std::vector<xresend_id_height_t> xtable_unconfirm_id_height_t::get_resend_id_height_list(uint64_t cur_time) const {
    std::vector<xresend_id_height_t> resend_vec;
    for (auto & table_unconfirm_list : m_table_sid_unconfirm_list_map) {
        uint64_t receipt_id;
        uint64_t height;
        if (table_unconfirm_list.second->get_resend_id_height(receipt_id, height, cur_time)) {
            xresend_id_height_t resend_id_height;
            resend_id_height.table_sid = table_unconfirm_list.first;
            resend_id_height.receipt_id = receipt_id;
            resend_id_height.height = height;
            resend_vec.push_back(resend_id_height);
        }
    }
    return resend_vec;
}

uint32_t xtable_unconfirm_id_height_t::size() const {
    uint32_t count = 0;
    for (auto & id_height_list : m_table_sid_unconfirm_list_map) {
        count += id_height_list.second->size();
    }
    return count;
}

void xtable_unconfirm_id_height_as_sender_t::get_unconfirm_id_section(const xreceiptid_state_cache_t & receiptid_state_cache,
                                                                      base::xtable_shortid_t self_table_id,
                                                                      base::xtable_shortid_t peer_table_id,
                                                                      uint64_t & confirm_id,
                                                                      uint64_t & unconfirm_id_max) const {
    return receiptid_state_cache.get_unconfirm_id_section_as_sender(self_table_id, peer_table_id, confirm_id, unconfirm_id_max);
}

bool xtable_unconfirm_id_height_as_sender_t::is_sender() const {
    return true;
}

void xtable_unconfirm_id_height_as_receiver_t::get_unconfirm_id_section(const xreceiptid_state_cache_t & receiptid_state_cache,
                                                                        base::xtable_shortid_t self_table_id,
                                                                        base::xtable_shortid_t peer_table_id,
                                                                        uint64_t & confirm_id,
                                                                        uint64_t & unconfirm_id_max) const {
    return receiptid_state_cache.get_unconfirm_id_section_as_receiver(self_table_id, peer_table_id, confirm_id, unconfirm_id_max);
}

bool xtable_unconfirm_id_height_as_receiver_t::is_sender() const {
    return false;
}

void xprocessed_height_record_t::update_min_height(uint64_t height) {
    uint64_t new_min_height = (height & 0xFFFFFFFFFFFFFFC0UL);
    if (new_min_height > m_min_height) {
        uint32_t delete_record_num = ((new_min_height - m_min_height) >> 6);
        if (m_bit_record.size() > delete_record_num) {
            m_bit_record.erase(m_bit_record.begin(), m_bit_record.begin() + delete_record_num);
        } else {
            m_bit_record.clear();
        }

        m_min_height = new_min_height;
    }
    if (m_min_record_height < height) {
        m_min_record_height = height;
    }
    if (m_max_record_height < height) {
        m_max_record_height = height;
    }

    if (m_max_height < new_min_height) {
        m_max_height = new_min_height;
    }
    print();
}
void xprocessed_height_record_t::record_height(uint64_t height) {
    uint64_t new_min_height = (height & 0xFFFFFFFFFFFFFFC0UL);
    if (new_min_height < m_min_height) {
        uint32_t add_record_num = (m_min_height - new_min_height) >> 6;
        for (uint32_t i = 0; i < add_record_num; i++) {
            m_bit_record.push_front(0);
        }
        m_min_height = new_min_height;
    }

    uint64_t new_max_height = (height & 0xFFFFFFFFFFFFFFC0UL) + 64;
    if (new_max_height > m_max_height) {
        uint32_t add_record_num = (new_max_height - m_max_height) >> 6;
        for (uint32_t i = 0; i < add_record_num; i++) {
            m_bit_record.push_back(0);
        }
        m_max_height = new_max_height;
    }

    uint32_t recordidx = ((new_min_height - m_min_height) >> 6);
    uint64_t add_bit = (1UL << (height - new_min_height));
    xdbg("before add height:%llu,new_min_height:%llu,m_bit_record[%u]:0x%" PRIx64 " ,add num:0x%" PRIx64 " ", height, new_min_height, recordidx, m_bit_record[recordidx], add_bit);
    m_bit_record[recordidx] |= add_bit;

    if (height > m_max_record_height) {
        m_max_record_height = height;
    }

    if (height < m_min_record_height) {
        m_min_record_height = height;
    }
    xdbg("add height:%llu,new_min_height:%llu,m_bit_record[%u]:0x%" PRIx64 " ", height, new_min_height, recordidx, m_bit_record[recordidx]);
    print();
}
bool xprocessed_height_record_t::is_record_height(uint64_t height) const {
    if (height > m_max_record_height || height < m_min_record_height) {
        return false;
    }

    uint64_t min_height = (height & 0xFFFFFFFFFFFFFFC0UL);
    uint32_t recordidx = ((min_height - m_min_height) >> 6);

    uint64_t bit_num = (1UL << (height - min_height));
    xdbg("m_bit_record size:%u,height:%llu,m_bit_record[%u]=0x%" PRIx64 " ,judge bit:0x%" PRIx64 " ", m_bit_record.size(), height, recordidx, m_bit_record[recordidx], bit_num);
    return m_bit_record[recordidx] & bit_num;
}

bool xprocessed_height_record_t::get_latest_lacking_saction(uint64_t & left_end, uint64_t & right_end, uint16_t max_lacking_num) const {
    if (m_max_record_height == 0 || m_max_record_height == m_min_record_height) {
        return false;
    }
    uint64_t min_check_height = (m_min_record_height > 1) ? m_min_record_height : 1;
    bool right_end_found = false;

    // todo:to be optimize, check one u64 have 0 or not first, then calculate high bit and low bit.
    for (uint64_t height = m_max_record_height - 1; height >= min_check_height; height--) {
        uint64_t min_height = (height & 0xFFFFFFFFFFFFFFC0UL);
        uint32_t recordidx = ((min_height - m_min_height) >> 6);
        if (!right_end_found) {
            if (!(m_bit_record[recordidx] & (1UL << (height - min_height)))) {
                right_end = height;
                right_end_found = true;
            }
        } else {
            if ((height + max_lacking_num <= right_end) || (m_bit_record[recordidx] & (1UL << (height - min_height)))) {
                left_end = height + 1;
                return true;
            }
        }
    }

    if (right_end_found) {
        left_end = min_check_height;
        return true;
    }
    return false;
}

uint32_t xprocessed_height_record_t::size() const {
    return m_bit_record.size();
}

void xprocessed_height_record_t::print() const {
    xdbg("m_min_height:%llu,m_max_height:%llu,m_min_record_height:%llu,m_max_record_height:%llu,m_bit_record size:%u",
         m_min_height,
         m_max_height,
         m_min_record_height,
         m_max_record_height,
         m_bit_record.size());
}

bool xunconfirm_id_height::get_lacking_section(const xreceiptid_state_cache_t & receiptid_state_cache,
                                               const std::set<base::xtable_shortid_t> & all_table_sids,
                                               uint64_t & left_end,
                                               uint64_t & right_end,
                                               uint16_t max_lacking_num) const {
    // if (!receiptid_state_cache.is_all_table_state_cached(all_table_sids)) {
    //     left_end = 0;
    //     right_end = 0;
    //     return false;
    // }
    uint64_t min_height;
    bool is_lacking = false;
    bool ret = get_min_height(receiptid_state_cache, all_table_sids, min_height, true, is_lacking);
    xdbg("xunconfirm_id_height::get_lacking_section ret:%d,min_height:%llu,is_lacking:%d", ret, min_height, is_lacking);

    std::lock_guard<std::mutex> lck(m_mutex);
    if (ret) {
        m_processed_height_record.update_min_height(min_height);
    }

    if (is_lacking) {
        bool found_lacking_saction = m_processed_height_record.get_latest_lacking_saction(left_end, right_end, max_lacking_num);
        if (!found_lacking_saction) {
            left_end = 0;
            right_end = 0;
        }
        return true;
    }
    return false;
}

void xunconfirm_id_height::update_unconfirm_id_height(uint64_t table_height, uint64_t time, const std::vector<xtx_id_height_info> & tx_id_height_infos) {
    std::lock_guard<std::mutex> lck(m_mutex);
    for (auto & tx_info : tx_id_height_infos) {
        if (tx_info.m_subtype == base::enum_transaction_subtype_send) {
            m_sender_unconfirm_id_height.add_id_height(tx_info.m_peer_table_sid, tx_info.m_receipt_id, table_height, time);

        } else if (tx_info.m_subtype == base::enum_transaction_subtype_recv) {
            m_receiver_unconfirm_id_height.add_id_height(tx_info.m_peer_table_sid, tx_info.m_receipt_id, table_height, time);
        }
    }
    m_processed_height_record.record_height(table_height);
}

void xunconfirm_id_height::update_peer_confirm_id(base::xtable_shortid_t peer_table_sid, uint64_t confirm_id) {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_receiver_unconfirm_id_height.update_confirm_id(peer_table_sid, confirm_id);
}

// void xunconfirm_id_height::update_this_confirm_id(base::xtable_shortid_t peer_table_sid, uint64_t confirm_id) {
//     std::lock_guard<std::mutex> lck(m_mutex);
//     m_sender_unconfirm_id_height.update_confirm_id(peer_table_sid, confirm_id);
// }

bool xunconfirm_id_height::get_sender_table_height_by_id(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, uint64_t & height) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_sender_unconfirm_id_height.get_height_by_id(peer_table_sid, receipt_id, height);
}

bool xunconfirm_id_height::get_receiver_table_height_by_id(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, uint64_t & height) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_receiver_unconfirm_id_height.get_height_by_id(peer_table_sid, receipt_id, height);
}

std::vector<xresend_id_height_t> xunconfirm_id_height::get_sender_resend_id_height_list(uint64_t cur_time) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_sender_unconfirm_id_height.get_resend_id_height_list(cur_time);
}

std::vector<xresend_id_height_t> xunconfirm_id_height::get_receiver_resend_id_height_list(uint64_t cur_time) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_receiver_unconfirm_id_height.get_resend_id_height_list(cur_time);
}

void xunconfirm_id_height::cache_status(uint32_t & sender_cache_size, uint32_t & receiver_cache_size, uint32_t & height_record_size) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    sender_cache_size = m_sender_unconfirm_id_height.size();
    receiver_cache_size = m_receiver_unconfirm_id_height.size();
    height_record_size = m_processed_height_record.size();
}

// get_min_height will update commit id
bool xunconfirm_id_height::get_min_height(const xreceiptid_state_cache_t & receiptid_state_cache,
                                          const std::set<base::xtable_shortid_t> & all_table_sids,
                                          uint64_t & min_height,
                                          bool check_lacking,
                                          bool & is_lacking) const {
    auto table_receiptid_state = receiptid_state_cache.get_table_receiptid_state(m_self_table_sid);
    if (table_receiptid_state == nullptr) {
        return false;
    }
    auto self_table_height = table_receiptid_state->get_block_height();
    std::lock_guard<std::mutex> lck(m_mutex);
    uint64_t sender_min_height = 0;
    bool ret = m_sender_unconfirm_id_height.get_min_height(receiptid_state_cache, all_table_sids, self_table_height, sender_min_height, check_lacking, is_lacking);
    if (!ret) {
        xdbg("xunconfirm_id_height::get_min_height sender get min height fail selfid:%d", m_self_table_sid);
        return false;
    }

    uint64_t receiver_min_height = 0;
    ret = m_receiver_unconfirm_id_height.get_min_height(receiptid_state_cache, all_table_sids, self_table_height, receiver_min_height, check_lacking, is_lacking);
    if (!ret) {
        xdbg("xunconfirm_id_height::get_min_height receiver get min height fail selfid:%d", m_self_table_sid);
        return false;
    }
    min_height = (sender_min_height < receiver_min_height) ? sender_min_height : receiver_min_height;
    xdbg("xunconfirm_id_height::get_min_height selfid:%d,sender_min_height:%llu,receiver_min_height:%llu,min_height:%llu",
         m_self_table_sid,
         sender_min_height,
         receiver_min_height,
         min_height);
    return true;
}

NS_END2
