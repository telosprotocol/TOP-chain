// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xvledger/xreceiptid.h"
#include "xvledger/xvaccount.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

#undef __MODULE__
#define __MODULE__ "xtxpool_v2"

class xreceipt_state_cache_t {
public:
    xreceipt_state_cache_t() : m_receiptid_state(make_object_ptr<base::xreceiptid_state_t>()){
    }
    void update(const base::xreceiptid_state_ptr_t & receiptid_state);
    uint64_t get_tx_corresponding_latest_receipt_id(const std::shared_ptr<xtx_entry> & tx) const;
    uint64_t get_confirmid_max(base::xtable_shortid_t peer_table_sid) const;
    uint64_t get_recvid_max(base::xtable_shortid_t peer_table_sid) const;
    bool is_unconfirmed_num_reach_limmit(base::xtable_shortid_t peer_table_sid) const;

private:
    base::xreceiptid_state_ptr_t m_receiptid_state;
    mutable std::mutex m_mutex;
};

NS_END2
