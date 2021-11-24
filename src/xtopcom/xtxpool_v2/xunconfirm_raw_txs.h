// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xdata/xtransaction.h"
#include "xtxpool_v2/xreceiptid_state_cache.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvtxindex.h"

#include <inttypes.h>

NS_BEG2(top, xtxpool_v2)

struct xraw_tx_info {
    xraw_tx_info(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, data::xtransaction_ptr_t raw_tx)
      : m_peer_table_sid(peer_table_sid), m_receipt_id(receipt_id), m_raw_tx(raw_tx) {
    }
    base::xtable_shortid_t m_peer_table_sid;
    uint64_t m_receipt_id;
    data::xtransaction_ptr_t m_raw_tx;
};

class xunconfirm_raw_txs {
public:
    xunconfirm_raw_txs(base::xtable_shortid_t self_table_sid) : m_self_table_sid(self_table_sid) {
    }
    void add_raw_txs(std::vector<xraw_tx_info> raw_txs);
    data::xtransaction_ptr_t get_raw_tx(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id) const;
    void refresh(base::xreceiptid_state_ptr_t table_receiptid_state);

private:
    base::xtable_shortid_t m_self_table_sid;
    std::map<base::xtable_shortid_t, std::map<uint64_t, data::xtransaction_ptr_t>> m_raw_tx_cache;
    mutable std::mutex m_mutex;
};

NS_END2
