// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xreceipt_state_cache.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xverifier_utl.h"
#include "xvledger/xvledger.h"
#include "xstatestore/xstatestore_face.h"

#define table_unconfirm_tx_num_max (2048)

namespace top {
namespace xtxpool_v2 {

uint64_t xtable_state_cache_t::get_tx_corresponding_latest_receipt_id(const std::shared_ptr<xtx_entry> & tx) const {
    auto peer_table_sid = tx->get_tx()->get_peer_tableid();

    return tx->get_tx()->is_recv_tx() ? get_recvid_max(peer_table_sid) : get_confirmid_max(peer_table_sid);
}
uint64_t xtable_state_cache_t::get_confirmid_max(base::xtable_shortid_t peer_table_sid) const {
    base::xreceiptid_pair_t receiptid_pair;
    auto tablestate = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(m_table_address);
    tablestate->get_receiptid_state()->find_pair(peer_table_sid, receiptid_pair);
    xdbg("xtable_state_cache_t::get_confirmid_max this:%p,table:%d,peer:%d,id:%llu", this, tablestate->get_receiptid_state()->get_self_tableid(), peer_table_sid, receiptid_pair.get_confirmid_max());
    return receiptid_pair.get_confirmid_max();
}
uint64_t xtable_state_cache_t::get_recvid_max(base::xtable_shortid_t peer_table_sid) const {
    base::xreceiptid_pair_t receiptid_pair;
    auto tablestate = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(m_table_address);
    tablestate->get_receiptid_state()->find_pair(peer_table_sid, receiptid_pair);
    xdbg("xtable_state_cache_t::get_recvid_max this:%p,table:%d,peer:%d,id:%llu", this, tablestate->get_receiptid_state()->get_self_tableid(), peer_table_sid, receiptid_pair.get_recvid_max());
    return receiptid_pair.get_recvid_max();
}

bool xtable_state_cache_t::is_unconfirmed_num_reach_limit(base::xtable_shortid_t peer_table_sid) const {
    base::xreceiptid_pair_t receiptid_pair;
    auto tablestate = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(m_table_address);
    xassert(nullptr != tablestate);
    tablestate->get_receiptid_state()->find_pair(peer_table_sid, receiptid_pair);
    return (receiptid_pair.get_unconfirm_num() >= table_unconfirm_tx_num_max);
}

bool xtable_state_cache_t::get_account_index(const std::string & account, base::xaccount_index_t & account_index) const {
    common::xaccount_address_t account_address(account);
    return statestore::xstatestore_hub_t::instance()->get_accountindex_from_latest_connected_table(account_address, account_index);
}

uint64_t xtable_state_cache_t::get_state_height() const {
    auto tablestate = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(m_table_address);
    return tablestate == nullptr ? 0 : tablestate->height();
}


}  // namespace xtxpool_v2
}  // namespace top
