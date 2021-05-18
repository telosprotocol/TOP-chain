// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xreceipt_state_cache.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_log.h"

namespace top {
namespace xtxpool_v2 {
void xreceipt_state_cache_t::update(const base::xreceiptid_state_ptr_t & receiptid_state) {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_receiptid_state = receiptid_state;
}
uint64_t xreceipt_state_cache_t::get_tx_corresponding_latest_receipt_id(const std::shared_ptr<xtx_entry> & tx) const {
    base::xreceiptid_pair_t receiptid_pair;
    auto & account_addr = (tx->get_tx()->is_recv_tx()) ? tx->get_tx()->get_source_addr() : tx->get_tx()->get_target_addr();
    base::xvaccount_t vaccount(account_addr);
    auto peer_table_sid = vaccount.get_short_table_id();

    return tx->get_tx()->is_recv_tx() ? get_recvid_max(peer_table_sid) : get_confirmid_max(peer_table_sid);
}
uint64_t xreceipt_state_cache_t::get_confirmid_max(base::xtable_shortid_t peer_table_sid) const {
    base::xreceiptid_pair_t receiptid_pair;
    std::lock_guard<std::mutex> lck(m_mutex);
    m_receiptid_state->find_pair(peer_table_sid, receiptid_pair);
    return receiptid_pair.get_confirmid_max();
}
uint64_t xreceipt_state_cache_t::get_recvid_max(base::xtable_shortid_t peer_table_sid) const {
    base::xreceiptid_pair_t receiptid_pair;
    std::lock_guard<std::mutex> lck(m_mutex);
    m_receiptid_state->find_pair(peer_table_sid, receiptid_pair);
    return receiptid_pair.get_recvid_max();
}


}  // namespace xtxpool_v2
}  // namespace top
