// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_v2/xtxpool_table.h"

#include "xbasic/xmodule_type.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xtable_bstate.h"
#include "xmbus/xevent_behind.h"
#include "xtxpool_v2/xnon_ready_account.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_errors.h"
#include "xverifier/xverifier_utl.h"
#include "xverifier/xwhitelist_verifier.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvcontract.h"

namespace top {
namespace xtxpool_v2 {

#define state_update_too_long_time (600)
#define load_table_block_num_max (20)
#define table_fail_behind_height_diff_max (5)
#define table_sync_on_demand_num_max (10)

bool xtxpool_table_t::is_reach_limit(const std::shared_ptr<xtx_entry> & tx) const {
    if (tx->get_tx()->is_send_tx()) {
        // if (m_unconfirmed_tx_num >= table_unconfirm_txs_num_max) {
        //     xtxpool_warn("xtxpool_table_t::push_send_tx node unconfirm txs reached upper limit tx:%s", tx->get_tx()->dump().c_str());
        //     return true;
        // }

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
    if (tx->get_tx()->is_recv_tx()) {
        m_unconfirm_id_height.update_peer_confirm_id(tx->get_tx()->get_peer_tableid(), tx->get_tx()->get_last_action_sender_confirmed_receipt_id());
    }

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

void xtxpool_table_t::update_id_state(const std::vector<update_id_state_para> & para_vec) {
    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        for (auto & para : para_vec) {
            m_txmgr_table.update_id_state(para.m_txinfo, para.m_peer_table_sid, para.m_receiptid, para.m_nonce);
        }
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

void xtxpool_table_t::deal_commit_table_block(xblock_t * table_block, bool update_txmgr) {
    std::vector<xtx_id_height_info> tx_id_height_infos;
    std::vector<update_id_state_para> update_id_state_para_vec;
    auto tx_actions = table_block->get_tx_actions();
    
    for (auto & action : tx_actions) {
        if (action.get_org_tx_hash().empty()) {  // not txaction
            continue;
        }

        xlightunit_action_t txaction(action);
        if (update_txmgr) {
            const std::string uri = txaction.get_contract_uri();
            const std::string & account = base::xvcontract_t::get_contract_address(uri);
            tx_info_t txinfo(account, txaction.get_tx_hash_256(), txaction.get_tx_subtype());
            uint64_t txnonce = 0;
            xtransaction_ptr_t _rawtx = table_block->query_raw_transaction(txaction.get_tx_hash());
            if (_rawtx != nullptr) {
                txnonce = _rawtx->get_tx_nonce();
            }

            update_id_state_para_vec.push_back(update_id_state_para(txinfo, txaction.get_receipt_id_peer_tableid(), txaction.get_receipt_id(), txnonce));
        }

        if (txaction.get_tx_subtype() == base::enum_transaction_subtype_send || txaction.get_tx_subtype() == base::enum_transaction_subtype_recv) {
            tx_id_height_infos.push_back(xtx_id_height_info(txaction.get_tx_subtype(), txaction.get_receipt_id_peer_tableid(), txaction.get_receipt_id()));

            xtxpool_info(
                "xtxpool_table_t::deal_commit_table_block update unconfirm id height table:%s,peer table:%d,tx type:%d,receipt_id::%llu,confirm id:%llu,table_height:%llu",
                m_xtable_info.get_account().c_str(),
                txaction.get_receipt_id_peer_tableid(),
                txaction.get_tx_subtype(),
                txaction.get_receipt_id(),
                txaction.get_sender_confirmed_receipt_id(),
                table_block->get_height());
        }
    }

    if (update_txmgr) {
        update_id_state(update_id_state_para_vec);
    }
    m_unconfirm_id_height.update_unconfirm_id_height(table_block->get_height(), table_block->get_cert()->get_gmtime(), tx_id_height_infos);
}

void xtxpool_table_t::on_block_confirmed(xblock_t * table_block) {
    deal_commit_table_block(table_block, true);
}

int32_t xtxpool_table_t::verify_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) {
    for (auto & tx : txs) {
        {
            std::lock_guard<std::mutex> lck(m_mgr_mutex);
            auto tx_inside = m_txmgr_table.query_tx(tx->get_account_addr(), tx->get_tx_hash_256());
            if (tx_inside != nullptr) {
                if (tx_inside->get_tx()->get_tx_subtype() == tx->get_tx_subtype()) {
                    continue;
                } else if (tx_inside->get_tx()->get_tx_subtype() > tx->get_tx_subtype()) {
                    return xtxpool_error_request_tx_repeat;
                }
            }
        }
        int32_t ret = verify_cons_tx(tx);
        if (ret != xsuccess) {
            xtxpool_warn("xtxpool_table_t::verify_txs verify fail,tx:%s,err:%u", tx->dump(true).c_str(), ret);
            if (ret == xverifier::xverifier_error::xverifier_error_tx_duration_expired) {
                return xsuccess;
            }
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

void xtxpool_table_t::refresh_table() {
    auto latest_committed_block = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block(m_xtable_info, metrics::blockstore_access_from_txpool_refresh_table);
    xtxpool_dbg("xtxpool_table_t::refresh_table begin table:%s,commit_height:%llu", m_xtable_info.get_account().c_str(), latest_committed_block->get_height());

    uint64_t old_state_height = m_table_state_cache.get_state_height();
    if (old_state_height < latest_committed_block()->get_height()) {
        base::xauto_ptr<base::xvbstate_t> bstate =
            base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_committed_block.get(), metrics::statestore_access_from_txpool_refreshtable);
        if (bstate == nullptr) {
            xtxpool_warn("xtxpool_table_t::refresh_table fail-get bstate.table=%s,block=%s", m_xtable_info.get_table_addr().c_str(), latest_committed_block->dump().c_str());
            return;
        }
        xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(bstate.get());
        update_table_state(tablestate);
    }

    {
        std::lock_guard<std::mutex> lck(m_mgr_mutex);
        m_txmgr_table.clear_expired_txs();
    }

    uint64_t left_end = 0;
    uint64_t right_end = 0;
    bool ret = m_unconfirm_id_height.get_lacking_section(m_para->get_receiptid_state_cache(), m_xtable_info.get_all_table_sids(), left_end, right_end, load_table_block_num_max);
    if (ret) {
        uint64_t load_height_max = right_end;
        uint64_t load_height_min = left_end;
        if (left_end == 0 && right_end == 0) {
            if (latest_committed_block->get_height() <= 1) {
                xtxpool_warn("xtxpool_table_t::refresh_table load commit block fail,table:%s", m_xtable_info.get_account().c_str());
                return;
            }
            deal_commit_table_block(dynamic_cast<xblock_t *>(latest_committed_block.get()), false);
            load_height_max = latest_committed_block->get_height() - 1;
            load_height_min = (load_height_max >= load_table_block_num_max) ? (load_height_max + 1 - load_table_block_num_max) : 1;
        }

        for (uint64_t height = load_height_max; height >= load_height_min; height--) {
            base::xauto_ptr<base::xvblock_t> commit_block =
                m_para->get_vblockstore()->load_block_object(m_xtable_info, height, base::enum_xvblock_flag_committed, false, metrics::blockstore_access_from_txpool_refresh_table);

            if (commit_block != nullptr) {
                deal_commit_table_block(dynamic_cast<xblock_t *>(commit_block.get()), false);
            } else {
                uint64_t sync_from_height = (height > load_height_min + table_sync_on_demand_num_max - 1) ? (height - table_sync_on_demand_num_max + 1) : load_height_min;
                // try sync table block
                mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(
                    m_xtable_info.get_account(), sync_from_height, (uint32_t)(height - sync_from_height + 1), false, "lack_of_table_block", false);
                m_para->get_bus()->push_event(ev);
                xtxpool_warn("xtxpool_table_t::refresh_table load table block fail:table:%s,try sync %llu-%llu", m_xtable_info.get_account().c_str(), sync_from_height, height);
                XMETRICS_GAUGE(metrics::txpool_try_sync_table_block, 1);
                break;
            }
        }
    }
    xtxpool_info("xtxpool_table_t::refresh_table finish table:%s,old_state_height:%llu,new_state_height=%llu,get_lacking_ret=%d,left_end=%ld,right_end=%ld",
                m_xtable_info.get_account().c_str(),
                old_state_height,
                m_table_state_cache.get_state_height(),
                ret,
                left_end,
                right_end);
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
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    m_xtable_info.set_unconfirm_tx_count((int32_t)table_state->get_receiptid_state()->get_unconfirm_tx_num());
}

void xtxpool_table_t::add_role(xtxpool_role_info_t * role) {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    m_xtable_info.add_role(role);
}

void xtxpool_table_t::remove_role(xtxpool_role_info_t * role) {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    m_xtable_info.remove_role(role);
}

bool xtxpool_table_t::no_role() const {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_xtable_info.no_role();
}

bool xtxpool_table_t::need_sync_lacking_receipts() const {
    auto cur_height = m_para->get_vblockstore()->get_latest_committed_block_height(m_xtable_info, metrics::blockstore_access_from_txpool_pull_lacking_receipts);
    auto id_state_cache_height = m_para->get_receiptid_state_cache().get_height(m_xtable_info.get_short_table_id());
    xtxpool_dbg("xtxpool_table_t::need_sync_lacking_receipts table:%s,cur height:%llu,bc height:%llu", m_xtable_info.get_account().c_str(), cur_height, id_state_cache_height);
    if (cur_height + table_fail_behind_height_diff_max < id_state_cache_height) {
        xtxpool_warn("xtxpool_table_t::need_sync_lacking_receipts table:%s fail behind,cur height:%llu,bc height:%llu",
                     m_xtable_info.get_account().c_str(),
                     cur_height,
                     id_state_cache_height);
        return false;
    }

    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    if (m_table_state_cache.last_update_time() + state_update_too_long_time < now) {
        xtxpool_warn(
            "xtxpool_table_t::need_sync_lacking_receipts too long time not update receipt state, not sync lacking receipts. table:%s,update time:%llu,now:%llu,threshold:%d",
            m_xtable_info.get_table_addr().c_str(),
            m_table_state_cache.last_update_time(),
            now,
            state_update_too_long_time);
        return false;
    }
    return true;
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxpool_table_t::get_lacking_recv_tx_ids(uint32_t & total_num) const {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.get_lacking_recv_tx_ids(total_num);
}

int32_t xtxpool_table_t::verify_cons_tx(const xcons_transaction_ptr_t & tx) const {
    int32_t ret;
    if (tx->is_send_tx() || tx->is_self_tx()) {
        ret = verify_send_tx(tx);
    } else if (tx->is_recv_tx() || tx->is_confirm_tx()) {
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

    XMETRICS_GAUGE(metrics::cpu_ca_verify_multi_sign_txreceipt, 1);
    base::enum_vcert_auth_result auth_result = m_para->get_certauth()->verify_muti_sign(prove_cert.get(), prove_account);
    if (auth_result != base::enum_vcert_auth_result::enum_successful) {
        int32_t ret = xtxpool_error_tx_multi_sign_error;
        xtxpool_warn("xtxpool_table_t::verify_receipt_tx fail. account=%s,tx=%s,auth_result:%d,fail-%u", prove_account.c_str(), tx->dump(true).c_str(), auth_result, ret);
        return ret;
    }
    return xsuccess;
}

bool xtxpool_table_t::get_account_latest_nonce(const std::string account_addr, uint64_t & latest_nonce) const {
    base::xaccount_index_t account_index;
    bool ret = m_table_state_cache.get_account_index(account_addr, account_index);
    if (!ret) {
        xtxpool_warn("xtxpool_table_t::get_account_latest_nonce get account index fail account:%s", account_addr.c_str());
        return false;
    }
    base::xvaccount_t _account_vaddress(account_addr);

    xblocktool_t::check_lacking_unit_and_try_sync(_account_vaddress, account_index, m_para->get_vblockstore(), "txpool");

    base::xauto_ptr<base::xvblock_t> _start_block_ptr = m_para->get_vblockstore()->get_latest_committed_block(_account_vaddress);
    base::xauto_ptr<base::xvbstate_t> account_bstate =
        base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_start_block_ptr.get(), metrics::statestore_access_from_txpool_get_accountstate);
    if (account_bstate == nullptr) {
        xtxpool_warn("xtxpool_table_t::get_account_latest_nonce fail-get unitstate. block=%s", _start_block_ptr->dump().c_str());
        return false;
    }

    xaccount_ptr_t account_state = std::make_shared<xunit_bstate_t>(account_bstate.get());
    latest_nonce = account_state->account_send_trans_number();
    return true;
}

const std::vector<xtxpool_table_lacking_receipt_ids_t> xtxpool_table_t::get_lacking_confirm_tx_ids(uint32_t & total_num) const {
    std::lock_guard<std::mutex> lck(m_mgr_mutex);
    return m_txmgr_table.get_lacking_confirm_tx_ids(total_num);
}

xcons_transaction_ptr_t xtxpool_table_t::build_receipt(base::xtable_shortid_t peer_table_sid, uint64_t receipt_id, uint64_t commit_height, enum_transaction_subtype subtype) {
    base::xauto_ptr<base::xvblock_t> commit_block =
        m_para->get_vblockstore()->load_block_object(m_xtable_info, commit_height, base::enum_xvblock_flag_committed, false, metrics::blockstore_access_from_txpool_create_receipt);
    if (commit_block == nullptr) {
        xerror("xtxpool_table_t::build_receipt fail-commit table block not exist table=%s,peer table:%d,receipt id:%llu,table_height:%llu,subtype:%d",
               m_xtable_info.get_account().c_str(),
               peer_table_sid,
               receipt_id,
               commit_height,
               subtype);
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> cert_block =
        m_para->get_vblockstore()->load_block_object(m_xtable_info, commit_height + 2, 0, false, metrics::blockstore_access_from_txpool_create_receipt);
    if (cert_block == nullptr) {
        xerror("xtxpool_table_t::build_receipt fail-cert table block not exist table=%s,peer table:%d,receipt id:%llu,table_height:%llu,subtype:%d",
               m_xtable_info.get_account().c_str(),
               peer_table_sid,
               receipt_id,
               commit_height + 2,
               subtype);
        return nullptr;
    }

    m_para->get_vblockstore()->load_block_input(commit_block->get_account(), commit_block.get());

    auto tx = xblocktool_t::create_one_txreceipt(dynamic_cast<xblock_t *>(commit_block.get()), dynamic_cast<xblock_t *>(cert_block.get()), peer_table_sid, receipt_id, subtype);
    xassert(tx != nullptr);
    return tx;
}

void xtxpool_table_t::build_recv_tx(base::xtable_shortid_t peer_table_sid, std::vector<uint64_t> receiptids, std::vector<xcons_transaction_ptr_t> & receipts) {
    auto self_table_sid = m_xtable_info.get_short_table_id();
    auto self_confirmid = m_para->get_receiptid_state_cache().get_confirmid_max(self_table_sid, peer_table_sid);
    auto peer_recvid = m_para->get_receiptid_state_cache().get_recvid_max(peer_table_sid, self_table_sid);
    uint64_t id_lower_bound = (self_confirmid > peer_recvid) ? self_confirmid : peer_recvid;
    for (auto & receiptid : receiptids) {
        if (receiptid <= id_lower_bound) {
            continue;
        }
        uint64_t commit_height;
        bool ret = m_unconfirm_id_height.get_sender_table_height_by_id(peer_table_sid, receiptid, commit_height);
        if (!ret) {
            xtxpool_info("xtxpool_table_t::build_recv_tx fail-receipt id not found self:%d,peer:%d,receipt id:%lu", self_table_sid, peer_table_sid, receiptid);
            continue;
        }

        auto receipt = build_receipt(peer_table_sid, receiptid, commit_height, enum_transaction_subtype_send);
        if (receipt != nullptr) {
            receipts.push_back(receipt);
        }
    }
}

void xtxpool_table_t::build_confirm_tx(base::xtable_shortid_t peer_table_sid, std::vector<uint64_t> receiptids, std::vector<xcons_transaction_ptr_t> & receipts) {
    auto self_table_sid = m_xtable_info.get_short_table_id();
    auto peer_confirmid = m_para->get_receiptid_state_cache().get_confirmid_max(peer_table_sid, self_table_sid);
    for (auto & receiptid : receiptids) {
        if (receiptid <= peer_confirmid) {
            continue;
        }
        uint64_t commit_height;
        bool ret = m_unconfirm_id_height.get_receiver_table_height_by_id(peer_table_sid, receiptid, commit_height);
        if (!ret) {
            xtxpool_info("xtxpool_table_t::build_confirm_tx fail-receipt id not found self:%d,peer:%d,receipt id:%lu", self_table_sid, peer_table_sid, receiptid);
            continue;
        }

        auto receipt = build_receipt(peer_table_sid, receiptid, commit_height, enum_transaction_subtype_recv);
        if (receipt != nullptr) {
            receipts.push_back(receipt);
        }
    }
}

void xtxpool_table_t::unconfirm_cache_status(uint32_t & sender_cache_size, uint32_t & receiver_cache_size, uint32_t & height_record_size) const {
    m_unconfirm_id_height.cache_status(sender_cache_size, receiver_cache_size, height_record_size);
}

}  // namespace xtxpool_v2
}  // namespace top
