// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xtxpool_v2/xtxmgr_table.h"

#include "xtxpool_v2/xtxpool_error.h"

namespace top {
namespace xtxpool_v2 {

using data::xcons_transaction_ptr_t;

#define txs_num_pop_from_queue_max (100)

int32_t xtxmgr_table_t::push_tx(const xcons_transaction_ptr_t & tx, const xtx_para_t & tx_para) {
    std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, tx_para);
    int32_t ret = m_tx_queue.push_tx(tx_ent);
    if (ret != xsuccess) {
        xwarn("xtxmgr_table_t::push_tx fail.table %s, tx:%s", m_xtable_info->get_table_addr().c_str(), tx->dump().c_str());
    } else {
        xinfo("xtxmgr_table_t::push_tx success.table %s, tx:%s", m_xtable_info->get_table_addr().c_str(), tx->dump().c_str());
    }
    // pop tx from queue and push to pending here would bring about that queue is empty frequently,
    // thus the tx usually directly goes to pending without ordered in queue.
    // therefore, we should not do that without any conditions
    // queue_to_pending();
    return ret;
}

std::shared_ptr<xtx_entry> xtxmgr_table_t::pop_tx_by_hash(const std::string & account, const uint256_t & hash, uint8_t subtype, int32_t err) {
    // maybe m_tx_queue m_pending_accounts both contains the tx
    std::shared_ptr<xtx_entry> tx_ent_q = m_tx_queue.pop_tx_by_hash(account, hash, subtype);
    std::shared_ptr<xtx_entry> tx_ent_p = m_pending_accounts.pop_tx_by_hash(account, hash, subtype, err);
    return (tx_ent_q != nullptr) ? tx_ent_q : tx_ent_p;
}

candidate_accounts xtxmgr_table_t::get_accounts_txs(uint32_t count) {
    queue_to_pending();
    candidate_accounts accounts = m_pending_accounts.pop_accounts(count);
    m_tx_queue.refreash_delay_accounts();
    return accounts;
}

int32_t xtxmgr_table_t::push_back_tx(std::shared_ptr<xtx_entry> tx_ent) {
    int32_t ret = m_tx_queue.push_tx(tx_ent);
    return ret;
}

const xcons_transaction_ptr_t xtxmgr_table_t::query_tx(const std::string & account, const uint256_t & hash) const {
    auto tx = m_tx_queue.query_tx(account, hash);
    if (tx == nullptr) {
        tx = m_pending_accounts.query_tx(account, hash);
    }
    return tx;
}

int32_t xtxmgr_table_t::init() {
    return xsuccess;
}
int32_t xtxmgr_table_t::deinit() {
    return xsuccess;
}

void xtxmgr_table_t::queue_to_pending() {
    uint32_t tx_count = 0;
    bool stop_pop_receipt = false;
    bool stop_pop_send_tx = false;
    while (tx_count < txs_num_pop_from_queue_max) {
        std::vector<std::shared_ptr<xtx_entry>> txs;

        if (!stop_pop_receipt) {
            txs = m_tx_queue.pop_receipts();
            if (txs.empty()) {
                stop_pop_receipt = true;
            }
            xdbg("xtxmgr_table_t::queue_to_pending pop %d receipts from queue", txs.size());
        }

        if (!stop_pop_send_tx) {
            auto txs_send = m_tx_queue.pop_send_txs();
            if (txs_send.empty()) {
                stop_pop_send_tx = true;
            } else {
                txs.insert(txs.end(), txs_send.begin(), txs_send.begin() + txs_send.size());
            }
            xdbg("xtxmgr_table_t::queue_to_pending pop %d send txs from queue txs size:%d", txs_send.size(), txs.size());
        }

        if (txs.empty()) {
            break;
        }
        if (!push_to_pending(txs, tx_count)) {
            return;
        }
    }
}

bool xtxmgr_table_t::push_to_pending(std::vector<std::shared_ptr<xtx_entry>> & txs, uint32_t & tx_count) {
    int32_t ret = xsuccess;
    std::vector<std::shared_ptr<xtx_entry>> backward_txs;
    auto it = txs.begin();
    for (; it != txs.end(); it++) {
        ret = m_pending_accounts.push_tx(*it);
        xdbg("xtxmgr_table_t::push_to_pending push tx to pending tx:%s, ret=%d", (*it)->get_tx()->dump().c_str(), ret);
        if (ret == xtxpool_error_pending_reached_upper_limit) {
            backward_txs.insert(backward_txs.end(), it, txs.end());
            break;
        } else if (ret == xtxpool_error_pending_account_reached_upper_limit) {
            std::string account = (*it)->get_tx()->is_recv_tx() ? ((*it)->get_tx()->get_target_addr()) : ((*it)->get_tx()->get_source_addr());
            m_tx_queue.set_delay_accout(account, (*it)->get_tx()->get_tx_subtype());
            backward_txs.push_back(*it);
        } else if (ret == xsuccess) {
            tx_count++;
        }
    }

    for (auto it_b = backward_txs.rbegin(); it_b != backward_txs.rend(); it_b++) {
        m_tx_queue.push_tx(*it_b);
    }

    if (ret == xtxpool_error_pending_reached_upper_limit) {
        return false;
    }
    return true;
}

}  // namespace xtxpool_v2
}  // namespace top
