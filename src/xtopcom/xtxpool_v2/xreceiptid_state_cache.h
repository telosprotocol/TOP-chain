// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xreceiptid.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

enum enum_min_unconfirm_id_result {
    enum_min_unconfirm_id_ok = 0,
    enum_min_unconfirm_id_all_confirmed = 1,
    enum_min_unconfirm_id_fail = 2,
};

class xreceiptid_state_cache_t {
public:
    void update_table_receiptid_state(const base::xreceiptid_state_ptr_t & receiptid_state);
    uint64_t get_confirmid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const;
    uint64_t get_recvid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const;
    uint64_t get_sendid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const;
    uint64_t get_height(base::xtable_shortid_t table_id) const;
    base::xreceiptid_state_ptr_t get_table_receiptid_state(base::xtable_shortid_t table_id) const;
    // bool is_all_table_state_cached(const std::set<base::xtable_shortid_t> & all_table_sids) const;
    void get_unconfirm_id_section_as_sender(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id, uint64_t & confirm_id, uint64_t & unconfirm_id_max) const;
    void get_unconfirm_id_section_as_receiver(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id, uint64_t & confirm_id, uint64_t & unconfirm_id_max) const;
    bool get_min_unconfirm_id_as_receiver(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id, uint64_t & min_unconfirm_id) const;

private:
    mutable std::mutex m_mutex;
    std::map<base::xtable_shortid_t, base::xreceiptid_state_ptr_t> m_receiptid_state_map;
    mutable bool m_all_cached{false};
};

NS_END2
