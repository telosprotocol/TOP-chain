// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xvledger/xvaccount.h"

NS_BEG2(top, data)

enum enum_min_height_result {
    enum_min_height_result_ok = 0,
    enum_min_height_result_fail = 1,
    enum_min_height_result_no_unconfirm_id = 2,
};

#define resend_interval_min (60)

class xunconfirm_id_height_list_t {
public:
    void update_confirm_id(uint64_t confirm_id, uint32_t unconfirm_num) {
        if (confirm_id > m_confirm_id || m_confirm_id == 0xFFFFFFFFFFFFFFFF) {
            m_confirm_id = confirm_id;
            m_unconfirm_num = unconfirm_num;
            for (auto iter = m_id_height_map.begin(); iter != m_id_height_map.end();) {
                if (iter->first <= m_confirm_id) {
                    iter = m_id_height_map.erase(iter);
                } else {
                    break;
                }
            }
        }
    }
    void add_id_height(uint64_t receipt_id, uint64_t height, uint64_t time) {
        if (receipt_id > m_confirm_id) {
            m_id_height_map[receipt_id] = height;
            if (m_id_height_map.rbegin()->first == receipt_id) {
                m_update_time = time;
            }
        }
    }
    enum_min_height_result get_min_height(uint64_t & min_height) const {
        if (m_confirm_id == 0xFFFFFFFFFFFFFFFF) {
            return enum_min_height_result_fail;
        }
        if (m_unconfirm_num == 0) {
            return enum_min_height_result_no_unconfirm_id;
        }
        if (m_id_height_map.empty()) {
            return enum_min_height_result_fail;
        }
        if (m_id_height_map.begin()->first != m_confirm_id + 1) {
            return enum_min_height_result_fail;
        }
        min_height = m_id_height_map.begin()->second;
        return enum_min_height_result_ok;
    }
    bool get_height_by_id(uint64_t receipt_id, uint64_t & height) const {
        auto iter = m_id_height_map.find(receipt_id);
        if (iter == m_id_height_map.end()) {
            return false;
        }
        height = iter->second;

        return true;
    }
    bool is_reached_confirm_id() const {
        if (m_confirm_id == 0xFFFFFFFFFFFFFFFF) {
            return false;
        }
        if (m_unconfirm_num == 0) {
            return true;
        }
        if (m_id_height_map.empty()) {
            return false;
        }
        if (m_id_height_map.begin()->first != m_confirm_id + 1) {
            return false;
        }
        return true;
    }

    bool get_resend_id_height(uint64_t & receipt_id, uint64_t & height, uint64_t cur_time) const {
        if (m_id_height_map.empty() || cur_time < m_update_time + resend_interval_min) {
            return false;
        }

        receipt_id = m_id_height_map.rbegin()->first;
        height = m_id_height_map.rbegin()->second;
        return true;
    }

private:
    uint64_t m_confirm_id{0xFFFFFFFFFFFFFFFF};
    uint32_t m_unconfirm_num{0};
    std::map<uint64_t, uint64_t> m_id_height_map;
    uint64_t m_update_time{0};
};

struct xresend_id_height_t {
    base::xtable_shortid_t table_sid;
    uint64_t receipt_id;
    uint64_t height;
};

class xtable_unconfirm_id_height_t {
public:
    void update_confirm_id(base::xtable_shortid_t table_sid, uint64_t confirm_id, uint32_t unconfirm_num) {
        auto iter = m_table_sid_unconfirm_list_map.find(table_sid);
        if (iter == m_table_sid_unconfirm_list_map.end()) {
            xunconfirm_id_height_list_t unconfirm_list;
            unconfirm_list.update_confirm_id(confirm_id, unconfirm_num);
            m_table_sid_unconfirm_list_map[table_sid] = unconfirm_list;
            return;
        }
        iter->second.update_confirm_id(confirm_id, unconfirm_num);
    }
    void add_id_height(base::xtable_shortid_t table_sid, uint64_t receipt_id, uint64_t height, uint64_t time) {
        auto iter = m_table_sid_unconfirm_list_map.find(table_sid);
        if (iter == m_table_sid_unconfirm_list_map.end()) {
            xunconfirm_id_height_list_t unconfirm_list;
            unconfirm_list.add_id_height(receipt_id, height, time);
            m_table_sid_unconfirm_list_map[table_sid] = unconfirm_list;
            return;
        }
        iter->second.add_id_height(receipt_id, height, time);
    }
    bool get_min_height(uint64_t & min_height) const {
        uint64_t min_tmp = 0xFFFFFFFFFFFFFFFF;
        if (m_table_sid_unconfirm_list_map.size() < 67) {
            return false;
        }
        for (auto & iter : m_table_sid_unconfirm_list_map) {
            uint64_t height;
            auto ret = iter.second.get_min_height(height);
            if (ret == enum_min_height_result_fail) {
                return false;
            }

            if (ret == enum_min_height_result_ok && min_tmp > height) {
                min_tmp = height;
                return true;
            }
        }
    }
    bool get_height_by_id(base::xtable_shortid_t table_sid, uint64_t receipt_id, uint64_t & height) const {
        auto iter = m_table_sid_unconfirm_list_map.find(table_sid);
        if (iter != m_table_sid_unconfirm_list_map.end()) {
            return iter->second.get_height_by_id(receipt_id, height);
        }
        return false;
    }
    bool is_all_unconfirm_id_recovered() const {
        // todo:table到所有其他table的未确认数据都加到map里，且都找到了左边界，才是真的完全恢复了。
        if (m_table_sid_unconfirm_list_map.size() < 67) {
            return false;
        }
        for (auto & iter : m_table_sid_unconfirm_list_map) {
            if (!iter.second.is_reached_confirm_id()) {
                return false;
            }
        }
        return true;
    }

    std::vector<xresend_id_height_t> get_resend_id_height_list(uint64_t cur_time) const {
        std::vector<xresend_id_height_t> resend_vec;
        for (auto & table_unconfirm_list : m_table_sid_unconfirm_list_map) {
            uint64_t receipt_id;
            uint64_t height;
            if (table_unconfirm_list.second.get_resend_id_height(receipt_id, height, cur_time)) {
                xresend_id_height_t resend_id_height;
                resend_id_height.table_sid = table_unconfirm_list.first;
                resend_id_height.receipt_id = receipt_id;
                resend_id_height.height = height;
                resend_vec.push_back(resend_id_height);
            }
        }
        return resend_vec;
    }

private:
    std::map<base::xtable_shortid_t, xunconfirm_id_height_list_t> m_table_sid_unconfirm_list_map;
};

NS_END2
