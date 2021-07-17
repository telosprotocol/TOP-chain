// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxpool_table.h"

#include "xbasic/xmodule_type.h"
#include "xdata/xlightunit.h"
#include "xdata/xtable_bstate.h"
#include "xmbus/xevent_behind.h"
#include "xtxpool_v2/xnon_ready_account.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_utl.h"
#include "xverifier/xwhitelist_verifier.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"

namespace top {
namespace xtxpool_v2 {

#define state_update_too_long_time (600)

bool xtxpool_table_t::get_account_basic_info(const std::string & account, xaccount_basic_info_t & account_index_info) const {
    // TODO(jimmy) try sync behind account unit, make a new function
    base::xaccount_index_t account_index;
    bool ret = m_table_state_cache.get_account_index(account, account_index);
    if (!ret) {
        xtxpool_warn("xtxpool_table_t::get_account_basic_info get account index fail account:%s", account.c_str());
        return false;
    }
    base::xvaccount_t _account_vaddress(account);
    base::xauto_ptr<base::xvblock_t> _block_ptr = m_para->get_vblockstore()->get_latest_cert_block(_account_vaddress);
    xblock_ptr_t latest_cert_unit = xblock_t::raw_vblock_to_object_ptr(_block_ptr.get());
    if (latest_cert_unit->get_height() < account_index.get_latest_unit_height()) {
        base::xauto_ptr<base::xvblock_t> _start_block_ptr = m_para->get_vblockstore()->get_latest_connected_block(_account_vaddress);
        if (account_index.get_latest_unit_height() > _start_block_ptr->get_height()) {
            account_index_info.set_sync_height_start(_start_block_ptr->get_height() + 1);
            account_index_info.set_sync_num(account_index.get_latest_unit_height() - _start_block_ptr->get_height());
        }
        return false;
    }

    base::xauto_ptr<base::xvblock_t> _start_block_ptr = m_para->get_vblockstore()->get_latest_committed_block(_account_vaddress);
    base::xauto_ptr<base::xvbstate_t> account_bstate =
        base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_start_block_ptr.get(), metrics::statestore_access_from_txpool_get_accountstate);
    if (account_bstate == nullptr) {
        xwarn("xtxpool_table_t::get_account_basic_info fail-get unitstate. block=%s", _start_block_ptr->dump().c_str());
        return false;
    }

    xaccount_ptr_t account_state = std::make_shared<xunit_bstate_t>(account_bstate.get());
    // account_index_info.set_latest_block(latest_unit);
    account_index_info.set_latest_state(account_state);
    account_index_info.set_account_index(account_index);
    return true;
}

bool xtxpool_table_t::is_reach_limit(const std::shared_ptr<xtx_entry> & tx) const {
    if (tx->get_tx()->is_send_tx()) {
        if (m_unconfirmed_tx_num >= table_unconfirm_txs_num_max) {
            xtxpool_warn("xtxpool_table_t::push_send_tx node unconfirm txs reached upper limit tx:%s", tx->get_tx()->dump().c_str());
            return true;
        }

        auto peer_table_sid = tx->get_tx()->get_peer_tableid();
        if (m_table_state_cache.is_unconfirmed_num_reach_limit(peer_table_sid)) {
            xtxpool_warn("xtxpool_table_t::push_send_tx table-table unconfirm txs reached upper limit tx:%s,peer_sid:%d", tx->get_tx()->dump().c_str(), peer_table_sid);
            return true;
        }
    }
    return false;
}

int32_t xtxpool_table_t::push_send_tx_real(const std::shared_ptr<xtx_entry> & tx) {
    uint64_t latest_nonce;
    // bool is_cached_nonce = false;

    auto & account_addr = tx->get_tx()->get_source_addr();

    // {
    //     std::lock_guard<std::mutex> lck(m_mgr_mutex);
    //     is_cached_nonce = m_txmgr_table.get_account_nonce_cache(account_addr, latest_nonce);
    // }

    // if (!is_cached_nonce) {
        bool result = get_account_latest_nonce(account_addr, latest_nonce);
        if (!result) {
            // todo : push to non_ready_accounts
            // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
            // m_non_ready_accounts.push_tx(tx);
            xtxpool_warn("xtxpool_table_t::push_send_tx account state fall behind tx:%s", tx->get_tx()->dump(true).c_str());
            return xtxpool_error_account_state_fall_behind;
        }
    // }

    if (data::is_sys_contract_address(common::xaccount_address_t{account_addr})) {
        tx->get_para().set_tx_type_score(enum_xtx_type_socre_system);
    } else {
        tx->get_para().set_tx_type_score(enum_xtx_type_socre_normal);
    }

    int32_t ret;
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        // if (!is_cached_nonce) {
            m_txmgr_table.updata_latest_nonce(account_addr, latest_nonce);
        // }
        ret = m_txmgr_table.push_send_tx(tx, latest_nonce);
    }
    if (ret != xsuccess) {
        // XMETRICS_COUNTER_INCREMENT("txpool_push_tx_send_fail", 1);
        m_xtable_info.get_statistic()->inc_push_tx_send_fail_num(1);
    }
    return ret;
}

int32_t xtxpool_table_t::push_send_tx(const std::shared_ptr<xtx_entry> & tx) {
    if (is_reach_limit(tx)) {
        return xtxpool_error_account_unconfirm_txs_reached_upper_limit;
    }

    int32_t ret = verify_send_tx(tx->get_tx());
    if (ret != xsuccess) {
        return ret;
    }

    return push_send_tx_real(tx);
}

int32_t xtxpool_table_t::push_receipt_real(const std::shared_ptr<xtx_entry> & tx) {
    // auto & account_addr = tx->get_tx()->get_account_addr();

    // store::xaccount_basic_info_t account_basic_info;
    // bool result = m_table_indexstore->get_account_basic_info(account_addr, account_basic_info);
    // if (!result) {
    //     // todo : push to non_ready_accounts
    //     // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
    //     // m_non_ready_accounts.push_tx(tx);
    //     xtxpool_warn("xtxpool_table_t::push_receipt account state fall behind tx:%s", tx->get_tx()->dump(true).c_str());
    //     return xtxpool_error_account_state_fall_behind;
    // }

    if (data::is_sys_contract_address(common::xaccount_address_t{tx->get_tx()->get_account_addr()})) {
        tx->get_para().set_tx_type_score(enum_xtx_type_socre_system);
    } else {
        tx->get_para().set_tx_type_score(enum_xtx_type_socre_normal);
    }

    int32_t ret;
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        ret = m_txmgr_table.push_receipt(tx);
    }
    if (ret != xsuccess) {
        // XMETRICS_COUNTER_INCREMENT("txpool_push_tx_receipt_fail", 1);
        m_xtable_info.get_statistic()->inc_push_tx_receipt_fail_num(1);
    }
    return ret;
}

int32_t xtxpool_table_t::push_receipt(const std::shared_ptr<xtx_entry> & tx, bool is_self_send) {
    uint64_t latest_receipt_id = m_table_state_cache.get_tx_corresponding_latest_receipt_id(tx);
    uint64_t tx_receipt_id = tx->get_tx()->get_last_action_receipt_id();
    if (tx_receipt_id < latest_receipt_id) {
        xtxpool_warn("xtxpool_table_t::push_receipt duplicate receipt:%s,id:%llu:%llu", tx->get_tx()->dump().c_str(), tx_receipt_id, latest_receipt_id);
        // XMETRICS_COUNTER_INCREMENT("txpool_receipt_duplicate", 1);
        m_xtable_info.get_statistic()->inc_receipt_duplicate_num(1);
        return xtxpool_error_tx_duplicate;
    }

    // check if receipt is repeat before verify it, because receipt verify is a time-consuming job.
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        bool is_repeat = m_txmgr_table.is_repeat_tx(tx);
        if (is_repeat) {
            xtxpool_warn("xtxpool_table_t::push_receipt repeat receipt:%s", tx->get_tx()->dump().c_str());
            // XMETRICS_COUNTER_INCREMENT("txpool_receipt_repeat", 1);
            m_xtable_info.get_statistic()->inc_receipt_repeat_num(1);
            return xtxpool_error_request_tx_repeat;
        }
    }
    if (!is_self_send) {
        int32_t ret = verify_receipt_tx(tx->get_tx());
        if (ret != xsuccess) {
            return ret;
        }
    }

    return push_receipt_real(tx);
}

std::shared_ptr<xtx_entry> xtxpool_table_t::pop_tx(const tx_info_t & txinfo, bool clear_follower) {
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        bool exist = false;
        auto tx_ent = m_txmgr_table.pop_tx(txinfo, clear_follower);
        if (tx_ent != nullptr) {
            return tx_ent;
        }
    }

    // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
    // return m_non_ready_accounts.pop_tx(txinfo);
    return nullptr;
}

void xtxpool_table_t::update_id_state(const tx_info_t & txinfo, base::xtable_shortid_t peer_table_sid, uint64_t receiptid, uint64_t nonce) {
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        m_txmgr_table.update_id_state(txinfo, peer_table_sid, receiptid, nonce);
        return;
    }

    // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
    // m_non_ready_accounts.pop_tx(txinfo);
}

ready_accounts_t xtxpool_table_t::get_ready_accounts(const xtxs_pack_para_t & pack_para) {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.get_ready_accounts(pack_para);
}

std::vector<xcons_transaction_ptr_t> xtxpool_table_t::get_ready_txs(const xtxs_pack_para_t & pack_para) {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.get_ready_txs(pack_para);
}

const std::shared_ptr<xtx_entry> xtxpool_table_t::query_tx(const std::string & account, const uint256_t & hash) {
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        auto tx_ent = m_txmgr_table.query_tx(account, hash);
        if (tx_ent != nullptr) {
            return tx_ent;
        }
    }

    // std::lock_guard<std::mutex> lck(m_non_ready_mutex);
    // return m_non_ready_accounts.find_tx(account, hash);
    return nullptr;
}

void xtxpool_table_t::updata_latest_nonce(const std::string & account_addr, uint64_t latest_nonce) {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.updata_latest_nonce(account_addr, latest_nonce);
}

bool xtxpool_table_t::is_account_need_update(const std::string & account_addr) const {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.is_account_need_update(account_addr);
}

const std::vector<xcons_transaction_ptr_t> xtxpool_table_t::get_resend_txs(uint64_t now) {
    std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
    return m_unconfirmed_tx_queue.get_resend_txs(now);
}

void xtxpool_table_t::on_block_confirmed(xblock_t * table_block) {
    // TODO(jimmy)
    const std::vector<base::xventity_t *> & _table_inentitys = table_block->get_input()->get_entitys();
    uint32_t entitys_count = _table_inentitys.size();
    for (uint32_t index = 1; index < entitys_count; index++) {  // unit entity from index#1
        base::xvinentity_t * _table_unit_inentity = dynamic_cast<base::xvinentity_t *>(_table_inentitys[index]);
        base::xtable_inentity_extend_t extend;
        extend.serialize_from_string(_table_unit_inentity->get_extend_data());
        const xobject_ptr_t<base::xvheader_t> & _unit_header = extend.get_unit_header();

        const std::vector<base::xvaction_t> & input_actions = _table_unit_inentity->get_actions();
        for (auto & action : input_actions) {
            if (action.get_org_tx_hash().empty()) {  // not txaction
                continue;
            }

            xlightunit_action_t txaction(action);
            tx_info_t txinfo(_unit_header->get_account(), txaction.get_tx_hash_256(), txaction.get_tx_subtype());
            uint64_t txnonce = 0;
            xtransaction_ptr_t _rawtx = table_block->query_raw_transaction(txaction.get_tx_hash());
            if (_rawtx != nullptr) {
                txnonce = _rawtx->get_tx_nonce();
            }
            update_id_state(txinfo, txaction.get_receipt_id_peer_tableid(), txaction.get_receipt_id(), txnonce);
        }
    }
}

int32_t xtxpool_table_t::verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) {
    for (auto & tx : txs) {
        {
            std::lock_guard<std::mutex> lck(m_mgr_mutex);
            auto tx_inside = m_txmgr_table.query_tx(tx->get_account_addr(), tx->get_tx_hash_256());
            if (tx_inside != nullptr) {
                if (tx_inside->get_tx()->get_tx_subtype() == tx->get_tx_subtype()) {
                    continue;
                } else if(tx_inside->get_tx()->get_tx_subtype() > tx->get_tx_subtype()) {
                    return xtxpool_error_request_tx_repeat;
                }
            }
        }
        int32_t ret = verify_cons_tx(tx);
        if (ret != xsuccess) {
            xtxpool_warn("xtxpool_table_t::verify_txs verify fail,tx:%s,err:%u", tx->dump(true).c_str(), ret);
            return ret;
        }

        xtxpool_v2::xtx_para_t para;
        std::shared_ptr<xtxpool_v2::xtx_entry> tx_ent = std::make_shared<xtxpool_v2::xtx_entry>(tx, para);
        if (tx->is_send_tx() || tx->is_self_tx()) {
            if (!is_reach_limit(tx_ent)) {
                xtxpool_info("xtxpool_table_t::verify_txs push tx from proposal tx:%s", tx->dump().c_str());
                XMETRICS_GAUGE(metrics::txpool_push_tx_from_proposal, 1);
                push_send_tx_real(tx_ent);
            }
        } else {
            uint64_t latest_receipt_id = m_table_state_cache.get_tx_corresponding_latest_receipt_id(tx_ent);
            uint64_t tx_receipt_id = tx->get_last_action_receipt_id();
            if (tx_receipt_id < latest_receipt_id) {
                xtxpool_warn("xtxpool_table_t::push_receipt duplicate receipt:%s,id:%llu:%llu", tx->dump().c_str(), tx_receipt_id, latest_receipt_id);
                return xtxpool_error_tx_duplicate;
            } else {
                xtxpool_info("xtxpool_table_t::verify_txs push tx from proposal tx:%s", tx->dump().c_str());
                XMETRICS_GAUGE(metrics::txpool_push_tx_from_proposal, 1);
                push_receipt_real(tx_ent);
            }
        }
    }
    return xsuccess;
}

void xtxpool_table_t::refresh_table(bool refresh_unconfirm_txs) {
    base::xvaccount_t _vaddr(m_xtable_info.get_table_addr());
    auto _block = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block(_vaddr, metrics::blockstore_access_from_txpool_refresh_table);
    base::xauto_ptr<base::xvbstate_t> bstate =
        base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block.get(), metrics::statestore_access_from_txpool_refreshtable);
    if (bstate == nullptr) {
        xwarn("xtxpool_table_t::refresh_table fail-get bstate.table=%s,block=%s", m_xtable_info.get_table_addr().c_str(), _block->dump().c_str());
        return;
    }

    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(bstate.get());
    xtxpool_info("xtxpool_table_t::refresh_table table:%s height:%llu", tablestate->get_account().c_str(), tablestate->get_block_height());
    update_table_state(tablestate);

    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        m_txmgr_table.clear_expired_txs();
    }

    if (refresh_unconfirm_txs) {
        std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
        uint32_t tx_num_before = m_unconfirmed_tx_queue.size();
        m_unconfirmed_tx_queue.recover(m_table_state_cache, tablestate);
        uint32_t tx_num_after = m_unconfirmed_tx_queue.size();
        m_unconfirmed_tx_num = m_unconfirmed_tx_queue.size();
        // XMETRICS_COUNTER_SET("table_unconfirm_txs_num" + m_xtable_info.get_table_addr(), m_unconfirmed_tx_num);
        if (tx_num_after > tx_num_before) {
            // XMETRICS_COUNTER_INCREMENT("txpool_unconfirm_txs_cache_num", tx_num_after - tx_num_before);
            m_xtable_info.get_statistic()->inc_unconfirm_tx_cache_num(tx_num_after - tx_num_before);
        } else if (tx_num_after < tx_num_before) {
            // XMETRICS_COUNTER_DECREMENT("txpool_unconfirm_txs_cache_num", tx_num_before - tx_num_after);
            m_xtable_info.get_statistic()->dec_unconfirm_tx_cache_num(tx_num_before - tx_num_after);
        }
    }
}

// void xtxpool_table_t::update_non_ready_accounts() {
// std::vector<std::shared_ptr<xtx_entry>> repush_txs;
// {
//     std::lock_guard<std::mutex> lck(m_non_ready_mutex);
//     auto account_addrs = m_non_ready_accounts.get_accounts();
//     for (auto & account_addr : account_addrs) {
//         store::xaccount_basic_info_t account_basic_info;
//         bool result = m_table_indexstore->get_account_basic_info(account_addr, account_basic_info);
//         if (result) {
//             auto tx_ents = m_non_ready_accounts.pop_account_txs(account_addr);
//             repush_txs.insert(repush_txs.end(), tx_ents.begin(), tx_ents.end());
//         }
//     }
// }

// for (auto &tx_ent : repush_txs) {
//     if (tx_ent->get_tx()->is_self_tx() || tx_ent->get_tx()->is_send_tx()) {
//         push_send_tx(tx_ent);
//     } else {
//         push_receipt(tx_ent);
//     }
// }
// }

void xtxpool_table_t::update_table_state(const data::xtablestate_ptr_t & table_state) {
    m_table_state_cache.update(table_state);

    int32_t unconfirm_tx_num = (int32_t)table_state->get_receiptid_state()->get_unconfirm_tx_num();
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    m_txmgr_table.update_receiptid_state(table_state->get_receiptid_state());
    m_xtable_info.set_unconfirm_tx_num(unconfirm_tx_num);
}

void xtxpool_table_t::add_shard(xtxpool_shard_info_t * shard) {
    m_xtable_info.add_shard(shard);
}

void xtxpool_table_t::remove_shard(xtxpool_shard_info_t * shard) {
    m_xtable_info.remove_shard(shard);
}

bool xtxpool_table_t::no_shard() const {
    return m_xtable_info.no_shard();
}

xcons_transaction_ptr_t xtxpool_table_t::get_unconfirmed_tx(const std::string & to_table_addr, uint64_t receipt_id) const {
    std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
    return m_unconfirmed_tx_queue.get_unconfirmed_tx(to_table_addr, receipt_id);
}

bool xtxpool_table_t::is_consensused_recv_receiptid(const std::string & from_addr, uint64_t receipt_id) const {
    base::xvaccount_t vaccount(from_addr);
    auto peer_table_sid = vaccount.get_short_table_id();
    uint64_t state_cache_receipt_id_max = m_table_state_cache.get_recvid_max(peer_table_sid);
    uint64_t tx_cache_receipt_id_max = m_txmgr_table.get_latest_recv_receipt_id(peer_table_sid);
    if (receipt_id > state_cache_receipt_id_max && receipt_id > tx_cache_receipt_id_max) {
        return false;
    }
    return true;
}

bool xtxpool_table_t::is_consensused_confirm_receiptid(const std::string & to_addr, uint64_t receipt_id) const {
    base::xvaccount_t vaccount(to_addr);
    auto peer_table_sid = vaccount.get_short_table_id();
    uint64_t state_cache_receipt_id_max = m_table_state_cache.get_confirmid_max(peer_table_sid);
    uint64_t tx_cache_receipt_id_max = m_txmgr_table.get_latest_confirm_receipt_id(peer_table_sid);
    if (receipt_id > state_cache_receipt_id_max && receipt_id > tx_cache_receipt_id_max) {
        return false;
    }
    return true;
}

bool xtxpool_table_t::need_sync_lacking_receipts() const {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    if (m_table_state_cache.last_update_time() + state_update_too_long_time < now) {
        xwarn("xtxpool_table_t::need_sync_lacking_receipts too long time not update receipt state, not sync lacking receipts. table:%s,update time:%llu,now:%llu,threshold:%d",
              m_xtable_info.get_table_addr().c_str(),
              m_table_state_cache.last_update_time(),
              now,
              state_update_too_long_time);
        return false;
    }
    return true;
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxpool_table_t::get_lacking_recv_tx_ids(uint32_t max_num) const {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.get_lacking_recv_tx_ids(max_num);
}

const std::vector<xtxpool_table_lacking_confirm_tx_hashs_t> xtxpool_table_t::get_lacking_confirm_tx_hashs(uint32_t max_num) const {
    std::vector<xtxpool_table_lacking_receipt_ids_t> lacking_ids_vec;
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        lacking_ids_vec = m_txmgr_table.get_lacking_confirm_tx_ids(max_num);
    }

    std::vector<xtxpool_table_lacking_confirm_tx_hashs_t> lacking_hashs;

    std::lock_guard<std::mutex> lck(m_unconfirm_mutex);
    for (auto & table_lacking_ids : lacking_ids_vec) {
        auto peer_sid = table_lacking_ids.get_peer_sid();
        xtxpool_table_lacking_confirm_tx_hashs_t table_lacking_hashs(peer_sid);
        std::vector<uint256_t> hash_vec;
        auto & receipt_ids = table_lacking_ids.get_receipt_ids();
        for (auto & receipt_id : receipt_ids) {
            auto cons_tx = m_unconfirmed_tx_queue.get_unconfirmed_tx(peer_sid, receipt_id);
            if (cons_tx == nullptr) {
                xtxpool_warn("xtxpool_table_t::get_lacking_confirm_tx_hashs unconfirm tx(peersid:%d,rid:%llu) not found", peer_sid, receipt_id);
            } else {
                table_lacking_hashs.add_receipt_id_hash(receipt_id, cons_tx->get_tx_hash_256());
            }
        }
        if (!table_lacking_hashs.empty()) {
            lacking_hashs.push_back(table_lacking_hashs);
        }
    }
    return lacking_hashs;
}

int32_t xtxpool_table_t::verify_cons_tx(const xcons_transaction_ptr_t & tx) const {
    int32_t ret;
    if (tx->is_send_tx() || tx->is_self_tx()) {
        ret = verify_send_tx(tx);
    } else if (tx->is_recv_tx()|| tx->is_confirm_tx()) {
        ret = verify_receipt_tx(tx);
    } else {
        ret = xtxpool_error_tx_invalid_type;
    }
    return ret;
}

int32_t xtxpool_table_t::verify_send_tx(const xcons_transaction_ptr_t & tx) const {
    // 1. validation check
    int32_t ret = xverifier::xtx_verifier::verify_send_tx_validation(tx->get_transaction());
    if (ret) {
        return ret;
    }
    // 2. legal check, include hash/signature check and white/black check
    ret = xverifier::xtx_verifier::verify_send_tx_legitimacy(tx->get_transaction(), make_observer(m_para->get_store()));
    if (ret) {
        return ret;
    }
    // 3. tx duration expire check
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    ret = xverifier::xtx_verifier::verify_tx_duration_expiration(tx->get_transaction(), now);
    if (ret) {
        return ret;
    }
    return xsuccess;
}

int32_t xtxpool_table_t::verify_receipt_tx(const xcons_transaction_ptr_t & tx) const {
    XMETRICS_TIME_RECORD("txpool_message_unit_receipt_push_receipt_verify_receipt_tx");
    if (!tx->verify_cons_transaction()) {
        return xtxpool_error_receipt_invalid;
    }

    std::string prove_account;
    xobject_ptr_t<base::xvqcert_t> prove_cert = tx->get_receipt_prove_cert_and_account(prove_account);
    if (nullptr == prove_cert) {
        xtxpool_error("xtxpool_table_t::verify_receipt_tx fail get prove cert, tx:%s", tx->dump(true).c_str());
        return xtxpool_error_tx_multi_sign_error;
    }

    base::enum_vcert_auth_result auth_result = m_para->get_certauth()->verify_muti_sign(prove_cert.get(), prove_account);
    if (auth_result != base::enum_vcert_auth_result::enum_successful) {
        int32_t ret = xtxpool_error_tx_multi_sign_error;
        xtxpool_warn("xtxpool_table_t::verify_receipt_tx fail. account=%s,tx=%s,auth_result:%d,fail-%u", prove_account.c_str(), tx->dump(true).c_str(), auth_result, ret);
        return ret;
    }
    return xsuccess;
}

bool xtxpool_table_t::get_account_latest_nonce(const std::string account_addr, uint64_t & latest_nonce) const {
    xaccount_basic_info_t account_basic_info;
    bool result = get_account_basic_info(account_addr, account_basic_info);
    if (!result) {
        if (account_basic_info.get_sync_num() > 0) {
            mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(
                account_addr, account_basic_info.get_sync_height_start(), account_basic_info.get_sync_num(), true, "account_state_fall_behind");
            m_para->get_bus()->push_event(ev);
            xtxpool_info("xtxpool_table_t::get_account_latest_nonce account:%s state fall behind,try sync unit from:%llu,count:%u",
                         account_addr.c_str(),
                         account_basic_info.get_sync_height_start(),
                         account_basic_info.get_sync_num());
        }
        return false;
    }

    latest_nonce = account_basic_info.get_latest_state()->account_send_trans_number();
    return true;
}

}  // namespace xtxpool_v2
}  // namespace top
