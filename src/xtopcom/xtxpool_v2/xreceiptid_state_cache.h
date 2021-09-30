// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xreceiptid.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

class xreceiptid_state_cache_t {
public:
    void update_table_receiptid_state(const base::xreceiptid_state_ptr_t & receiptid_state);
    uint64_t get_confirmid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const;
    uint64_t get_recvid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const;
    uint64_t get_sendid_max(base::xtable_shortid_t table_id, base::xtable_shortid_t peer_table_id) const;
    uint64_t get_height(base::xtable_shortid_t table_id) const;
    base::xreceiptid_state_ptr_t get_table_receiptid_state(base::xtable_shortid_t table_id) const;
private:
    mutable std::mutex m_mutex;
    std::map<base::xtable_shortid_t, base::xreceiptid_state_ptr_t> m_receiptid_state_map;
};

NS_END2
