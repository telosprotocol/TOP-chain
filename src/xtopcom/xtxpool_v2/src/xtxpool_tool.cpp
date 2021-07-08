// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxpool_tool.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"

namespace top {
namespace xtxpool_v2 {

ready_accounts_t xordered_ready_txs_t::ready_txs_to_ready_accounts(std::vector<xcons_transaction_ptr_t> ready_txs) {
    ready_accounts_t ready_accounts;
    std::map<std::string, std::shared_ptr<xready_account_t>> ready_accounts_map;

    for (auto tx : ready_txs) {
        auto account_addr = tx->get_account_addr();
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

xordered_ready_txs_t::xordered_ready_txs_t(ready_accounts_t reday_accounts) {
    for (auto & ready_account : reday_accounts) {
        auto & txs = ready_account->get_txs();
        for (auto & tx : txs) {
            if (tx->is_confirm_tx()) {
                add_tx(tx, m_peer_table_confirm_tx_map);
            } else if (tx->is_recv_tx()) {
                add_tx(tx, m_peer_table_recv_tx_map);
            } else {
                m_send_txs.push_back(tx);
            }
        }
    }
}

void xordered_ready_txs_t::erase_noncontinuous_receipts(const base::xreceiptid_state_ptr_t receiptid_state) {
    for (auto & recv_tx_map_pair : m_peer_table_recv_tx_map) {
        auto peer_table_sid = recv_tx_map_pair.first;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        auto min_receipt_id = receiptid_pair.get_recvid_max();
        auto & recv_tx_map = recv_tx_map_pair.second;
        for (auto it_recv_tx_map = recv_tx_map.begin(); it_recv_tx_map != recv_tx_map.end();) {
            auto & tx = it_recv_tx_map->second;
            if (tx->get_last_action_receipt_id() != (min_receipt_id + 1)) {
                xtxpool_warn("xordered_ready_txs_t::erase_noncontinuous_receipts:peer_table_sid:%d,tx:%s, min_receipt_id:%llu", peer_table_sid, tx->dump().c_str(), min_receipt_id);
                it_recv_tx_map = recv_tx_map.erase(it_recv_tx_map);
            } else {
                it_recv_tx_map++;
                min_receipt_id++;
            }
        }
    }

    for (auto & confirm_tx_map_pair : m_peer_table_confirm_tx_map) {
        auto peer_table_sid = confirm_tx_map_pair.first;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        auto min_receipt_id = receiptid_pair.get_confirmid_max();
        auto & confirm_tx_map = confirm_tx_map_pair.second;
        for (auto it_confirm_tx_map = confirm_tx_map.begin(); it_confirm_tx_map != confirm_tx_map.end();) {
            auto & tx = it_confirm_tx_map->second;
            if (tx->get_last_action_receipt_id() != (min_receipt_id + 1)) {
                xtxpool_warn("xordered_ready_txs_t::erase_noncontinuous_receipts:peer_table_sid:%d,tx:%s, min_receipt_id:%llu", peer_table_sid, tx->dump().c_str(), min_receipt_id);
                it_confirm_tx_map = confirm_tx_map.erase(it_confirm_tx_map);
            } else {
                it_confirm_tx_map++;
                min_receipt_id++;
            }
        }
    }
}

ready_accounts_t xordered_ready_txs_t::get_ready_accounts() const {
    std::vector<xcons_transaction_ptr_t> txs;
    for (auto & tx_map_pair : m_peer_table_confirm_tx_map) {
        auto & tx_map = tx_map_pair.second;
        for (auto & tx_pair : tx_map) {
            txs.push_back(tx_pair.second);
        }
    }
    for (auto & tx_map_pair : m_peer_table_recv_tx_map) {
        auto & tx_map = tx_map_pair.second;
        for (auto & tx_pair : tx_map) {
            txs.push_back(tx_pair.second);
        }
    }

    txs.insert(txs.end(), m_send_txs.begin(), m_send_txs.end());

    return ready_txs_to_ready_accounts(txs);
}

void xordered_ready_txs_t::add_tx(const xcons_transaction_ptr_t & tx, xpeer_table_receipt_map_t & peer_table_map) {
    auto peer_table_sid = tx->get_peer_tableid();
    auto it_peer_table_map = peer_table_map.find(peer_table_sid);
    if (it_peer_table_map == peer_table_map.end()) {
        std::map<uint64_t, xcons_transaction_ptr_t> receipt_id_map;
        receipt_id_map[tx->get_last_action_receipt_id()] = tx;
        peer_table_map[peer_table_sid] = receipt_id_map;
    } else {
        auto & receipt_id_map = it_peer_table_map->second;
        receipt_id_map[tx->get_last_action_receipt_id()] = tx;
    }
}

bool xordered_ready_txs_t::is_receipts_id_state_continuous(const base::xreceiptid_state_ptr_t receiptid_state) {
    for (auto & recv_tx_map_pair : m_peer_table_recv_tx_map) {
        auto peer_table_sid = recv_tx_map_pair.first;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        auto min_receipt_id = receiptid_pair.get_recvid_max();
        auto & recv_tx_map = recv_tx_map_pair.second;
        for (auto & recv_tx_pair : recv_tx_map) {
            auto & tx = recv_tx_pair.second;
            if (tx->get_last_action_receipt_id() != (min_receipt_id + 1)) {
                xtxpool_warn("recv tx receipt if not continuous:peer_table_sid:%d,tx:%s, min_receipt_id:%llu", peer_table_sid, tx->dump().c_str(), min_receipt_id);
                return false;
            }
            min_receipt_id++;
        }
    }

    for (auto & confirm_tx_map_pair : m_peer_table_confirm_tx_map) {
        auto peer_table_sid = confirm_tx_map_pair.first;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_state->find_pair(peer_table_sid, receiptid_pair);
        auto min_receipt_id = receiptid_pair.get_confirmid_max();
        auto & confirm_tx_map = confirm_tx_map_pair.second;
        for (auto & confirm_tx_pair : confirm_tx_map) {
            auto & tx = confirm_tx_pair.second;
            if (tx->get_last_action_receipt_id() != (min_receipt_id + 1)) {
                xtxpool_warn("recv tx receipt if not continuous:peer_table_sid:%d,tx:%s, min_receipt_id:%llu", peer_table_sid, tx->dump().c_str(), min_receipt_id);
                return false;
            }
            min_receipt_id++;
        }
    }
    return true;
}

}  // namespace xtxpool_v2
}  // namespace top
