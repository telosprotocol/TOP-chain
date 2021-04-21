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


static ready_accounts_t ready_txs_to_ready_accounts(std::vector<xcons_transaction_ptr_t> ready_txs) {
    ready_accounts_t ready_accounts;
    std::map<std::string, std::shared_ptr<xready_account_t>> ready_accounts_map;

    for (auto tx : ready_txs) {
        auto & account_addr = tx->get_account_addr();
        auto it_ready_accounts = ready_accounts_map.find(account_addr);
        if (it_ready_accounts != ready_accounts_map.end()) {
            it_ready_accounts->second->put_tx(tx);
        } else {
            auto ready_account = std::make_shared<xready_account_t>(account_addr);
            ready_account->put_tx(tx);
            ready_accounts_map[account_addr] = ready_account;
        }
    }

    for (auto ready_account : ready_accounts_map) {
        ready_accounts.push_back(ready_account.second);
    }

    xtxpool_dbg("ready_txs_to_ready_accounts ready_account size:%u", ready_accounts.size());
    return ready_accounts;
}

#if 0
using xpeer_table_receipt_map_t = std::unordered_map<base::xtable_shortid_t, std::map<uint64_t, xcons_transaction_ptr_t>>;
using xaccount_send_tx_map_t = std::unordered_map<std::string, std::map<uint64_t, xcons_transaction_ptr_t>>;

class xordered_ready_txs_t {
public:
    void covert_from_ready_accounts(ready_accounts_t reday_accounts);
    bool is_receipts_id_state_continuous(const base::xreceiptid_state_ptr_t receiptid_state);
    // todo:what parameter???
    bool is_send_txs_nonce_continuous();
private:
    xpeer_table_receipt_map_t m_peer_table_recv_tx_map;
    xpeer_table_receipt_map_t m_peer_table_confirm_tx_map;
    xaccount_send_tx_map_t m_account_send_tx_map;
};
#endif

// void xordered_ready_txs_t::covert_from_ready_accounts(ready_accounts_t reday_accounts) {
//     for (auto & ready_account : reday_accounts) {
        
//     }
// }

// bool xordered_ready_txs_t::is_receipts_id_state_continuous(const base::xreceiptid_state_ptr_t receiptid_state) {

// }

// bool xordered_ready_txs_t::is_send_txs_nonce_continuous() {

// }

NS_END2
