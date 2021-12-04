// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xreceipt_state_cache.h"

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xverifier_utl.h"
#include "xvledger/xvledger.h"

#define table_unconfirm_tx_num_max (2048)

namespace top {
namespace xtxpool_v2 {
void xtable_state_cache_t::update(const data::xtablestate_ptr_t & table_state) {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_table_state = table_state;
    m_update_time = xverifier::xtx_utl::get_gmttime_s();
    xdbg("xtable_state_cache_t::update this:%p,table:%d,id state:%s",
         this,
         table_state->get_receiptid_state()->get_self_tableid(),
         table_state->get_receiptid_state()->get_all_receiptid_pairs()->dump().c_str());
}
uint64_t xtable_state_cache_t::get_tx_corresponding_latest_receipt_id(const std::shared_ptr<xtx_entry> & tx) const {
    auto peer_table_sid = tx->get_tx()->get_peer_tableid();

    return tx->get_tx()->is_recv_tx() ? get_recvid_max(peer_table_sid) : get_confirmid_max(peer_table_sid);
}
uint64_t xtable_state_cache_t::get_confirmid_max(base::xtable_shortid_t peer_table_sid) const {
    base::xreceiptid_pair_t receiptid_pair;
    std::lock_guard<std::mutex> lck(m_mutex);
    if (m_table_state == nullptr) {
        if (!init_table_state()) {
            return 0;
        }
    }
    m_table_state->get_receiptid_state()->find_pair(peer_table_sid, receiptid_pair);
    xdbg("xtable_state_cache_t::get_confirmid_max this:%p,table:%d,peer:%d,id:%llu", this, m_table_state->get_receiptid_state()->get_self_tableid(), peer_table_sid, receiptid_pair.get_confirmid_max());
    return receiptid_pair.get_confirmid_max();
}
uint64_t xtable_state_cache_t::get_recvid_max(base::xtable_shortid_t peer_table_sid) const {
    base::xreceiptid_pair_t receiptid_pair;
    std::lock_guard<std::mutex> lck(m_mutex);
    if (m_table_state == nullptr) {
        if (!init_table_state()) {
            return 0;
        }
    }
    m_table_state->get_receiptid_state()->find_pair(peer_table_sid, receiptid_pair);
    xdbg("xtable_state_cache_t::get_recvid_max this:%p,table:%d,peer:%d,id:%llu", this, m_table_state->get_receiptid_state()->get_self_tableid(), peer_table_sid, receiptid_pair.get_recvid_max());
    return receiptid_pair.get_recvid_max();
}

uint64_t xtable_state_cache_t::last_update_time() const {
    return m_update_time;
}

bool xtable_state_cache_t::is_unconfirmed_num_reach_limit(base::xtable_shortid_t peer_table_sid) const {
    base::xreceiptid_pair_t receiptid_pair;
    std::lock_guard<std::mutex> lck(m_mutex);
    if (m_table_state == nullptr) {
        if (!init_table_state()) {
            return false;
        }
    }
    m_table_state->get_receiptid_state()->find_pair(peer_table_sid, receiptid_pair);
    return (receiptid_pair.get_unconfirm_num() >= table_unconfirm_tx_num_max);
}

bool xtable_state_cache_t::get_account_index(const std::string & account, base::xaccount_index_t & account_index) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    if (m_table_state == nullptr) {
        if (!init_table_state()) {
            return false;
        }
    }
    m_table_state->get_account_index(account, account_index);
    return true;
}

uint64_t xtable_state_cache_t::get_state_height() const {
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_table_state == nullptr ? 0 : m_table_state->get_block_height();
}

bool xtable_state_cache_t::init_table_state() const {
    auto latest_table = m_para->get_vblockstore()->get_latest_committed_block(m_table_account);
    xblock_ptr_t committed_block = xblock_t::raw_vblock_to_object_ptr(latest_table.get());
    base::xauto_ptr<base::xvbstate_t> bstate =
        base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(committed_block.get(), metrics::statestore_access_from_txpool_get_accountstate);
    if (bstate == nullptr) {
        xwarn("xtable_state_cache_t::init_table_state fail-get tablestate. this:%p,block=%s", this, committed_block->dump().c_str());
        return false;
    }

    m_table_state = std::make_shared<xtable_bstate_t>(bstate.get());
    return true;
}

}  // namespace xtxpool_v2
}  // namespace top
