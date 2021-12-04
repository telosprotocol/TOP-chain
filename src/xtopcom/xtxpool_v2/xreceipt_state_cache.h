// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxpool_v2/xtxpool_resources_face.h"
#include "xvledger/xreceiptid.h"
#include "xvledger/xvaccount.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

#undef __MODULE__
#define __MODULE__ "xtxpool_v2"

class xtable_state_cache_t {
public:
    xtable_state_cache_t(xtxpool_resources_face * para, const std::string & table_addr) : m_para(para), m_table_account(table_addr) {
    }
    void update(const data::xtablestate_ptr_t & table_state);
    uint64_t get_tx_corresponding_latest_receipt_id(const std::shared_ptr<xtx_entry> & tx) const;
    uint64_t get_confirmid_max(base::xtable_shortid_t peer_table_sid) const;
    uint64_t get_recvid_max(base::xtable_shortid_t peer_table_sid) const;
    uint64_t last_update_time() const;
    bool is_unconfirmed_num_reach_limit(base::xtable_shortid_t peer_table_sid) const;
    bool get_account_index(const std::string & account, base::xaccount_index_t & account_index) const;
    uint64_t get_state_height() const;

private:
    bool init_table_state() const;
    xtxpool_resources_face * m_para;
    base::xvaccount_t m_table_account;
    mutable data::xtablestate_ptr_t m_table_state{nullptr};
    uint64_t m_update_time{0};
    mutable std::mutex m_mutex;
};

NS_END2
