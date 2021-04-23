// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xtxpool_v2/xtxpool_log.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

#undef __MODULE__
#define __MODULE__ "xtxpool_v2"

using xpeer_table_receipt_map_t = std::unordered_map<base::xtable_shortid_t, std::map<uint64_t, xcons_transaction_ptr_t>>;

class xordered_ready_txs_t {
public:
    xordered_ready_txs_t(ready_accounts_t reday_accounts);
    void erase_noncontinuous_receipts(const base::xreceiptid_state_ptr_t receiptid_state);
    ready_accounts_t get_ready_accounts() const;
    bool is_receipts_id_state_continuous(const base::xreceiptid_state_ptr_t receiptid_state);
    static ready_accounts_t ready_txs_to_ready_accounts(std::vector<xcons_transaction_ptr_t> ready_txs);

private:
    void add_tx(const xcons_transaction_ptr_t & tx, xpeer_table_receipt_map_t & peer_table_map);
    xpeer_table_receipt_map_t m_peer_table_recv_tx_map;
    xpeer_table_receipt_map_t m_peer_table_confirm_tx_map;
    std::vector<xcons_transaction_ptr_t> m_send_txs;
};


NS_END2
