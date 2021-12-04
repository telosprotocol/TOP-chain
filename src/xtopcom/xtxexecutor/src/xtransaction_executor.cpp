// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#include "xdata/xgenesis_data.h"
#include "xmetrics/xmetrics.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xtxexecutor/xtransaction_executor.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvblockbuild.h"
#include "xchain_fork/xchain_upgrade_center.h"

NS_BEG2(top, txexecutor)

REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xtxexecutor, xunit_error_to_string, xconsensus_service_error_base+1, xconsensus_service_error_max);

int32_t xtransaction_executor::exec_one_tx(xaccount_context_t * account_context, const xcons_transaction_ptr_t & tx) {
    if (false == account_context->add_transaction(tx)) {
        xerror("xtransaction_executor::exec_one_tx fail-add transaction, tx=%s",
            tx->dump().c_str());
        return enum_xtxexecutor_error_tx_nonce_not_match;  // may happen when failure tx is deleted
    }

    size_t before_op_records_size = account_context->get_op_records_size();

    xtransaction_context_t tx_context(account_context, tx);
    int32_t action_ret = tx_context.exec();

    bool const sys_contract = base::xvaccount_t::get_addrtype_from_account(tx->get_account_addr()) == base::enum_vaccount_addr_type_native_contract;
    XMETRICS_GAUGE(metrics::txexecutor_total_system_contract_count, static_cast<int64_t>(sys_contract));

    if (sys_contract) {
        XMETRICS_GAUGE(metrics::txexecutor_system_contract_failed_count, static_cast<int64_t>(static_cast<bool>(action_ret)));
    }

    if (action_ret) {
        return action_ret;
    }

    size_t after_op_records_size = account_context->get_op_records_size();
    if (tx->is_self_tx() || tx->is_send_tx()) {
        if (after_op_records_size == before_op_records_size) {
            return xunit_contract_exec_no_property_change;
        }
    }

    const auto & fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    auto clock = account_context->get_timer_height();
    xdbg("xtransaction_executor::exec_one_tx timer:%llu, timestamp(second):%llu, forkclock:%llu", clock, tx->get_transaction()->get_fire_timestamp(), fork_config.block_fork_point.value().point);
    if (chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, clock)) {
        if (tx->is_confirm_tx()) {
            if (after_op_records_size == before_op_records_size) {
                xdbg("xtransaction_executor::exec_one_tx confirm state no change, tx_hash:%s, type:%s", tx->get_transaction()->get_digest_hex_str().c_str(), tx->get_tx_subtype_str().c_str());
                return xtransaction_confirm_state_unchange;
            }
        }
    }

    return xsuccess;
}

int32_t xtransaction_executor::exec_tx(xaccount_context_t * account_context, const data::xblock_consensus_para_t & cs_para, const xcons_transaction_ptr_t & tx, std::vector<xcons_transaction_ptr_t> & contract_create_txs) {
    uint64_t now = cs_para.get_gettimeofday_s();
    uint64_t delay_time_s = tx->get_transaction()->get_delay_from_fire_timestamp(now);
    if (tx->is_self_tx() || tx->is_send_tx()) {
        XMETRICS_GAUGE(metrics::txdelay_from_client_to_sendtx_exec, delay_time_s);
    } else if (tx->is_recv_tx()) {
        XMETRICS_GAUGE(metrics::txdelay_from_client_to_recvtx_exec, delay_time_s);
    } else if (tx->is_confirm_tx()) {
        XMETRICS_GAUGE(metrics::txdelay_from_client_to_confirmtx_exec, delay_time_s);
    }

    int32_t ret = exec_one_tx(account_context, tx);
    if (ret != xsuccess) {
        xwarn("xtransaction_executor::exec_tx input tx fail. %s %s error:%s",
            cs_para.dump().c_str(), tx->dump().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
        return ret;
    }

    // copy create txs from account context
    std::vector<xcons_transaction_ptr_t> create_txs = account_context->get_create_txs();
    // exec txs created by origin tx secondly, this tx must be a run contract transaction
    if (!create_txs.empty()) {
        for (auto & new_tx : create_txs) {
            ret = exec_one_tx(account_context, new_tx);
            if (ret != xsuccess && ret != xunit_contract_exec_no_property_change) {  // contract create tx send action may not change property, it's ok
                xwarn("xtransaction_executor::exec_tx fail contract create tx. %s,%s error:%s",
                    cs_para.dump().c_str(), new_tx->dump().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
                return ret;
            } else {
                xinfo("xtransaction_executor::exec_tx succ contract create tx. %s,input_tx:%s new_tx:%s",
                    cs_para.dump().c_str(), tx->dump().c_str(), new_tx->dump(true).c_str());
            }
            new_tx->set_push_pool_timestamp(now);
            contract_create_txs.push_back(new_tx);  // return create tx for unit pack
        }
    }
    xinfo("xtransaction_executor::exec_tx succ. %s,tx=%s,tx_state=%s",
        cs_para.dump().c_str(), tx->dump().c_str(), tx->dump_execute_state().c_str());
    return xsuccess;
}

int32_t xtransaction_executor::exec_batch_txs(base::xvblock_t* prev_block,
                                              const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                              const data::xblock_consensus_para_t & cs_para,
                                              const std::vector<xcons_transaction_ptr_t> & txs,
                                              xbatch_txs_result_t & txs_result) {

    std::vector<xcons_transaction_ptr_t> exec_txs = txs;
    std::vector<xcons_transaction_ptr_t> failure_send_txs;
    std::vector<xcons_transaction_ptr_t> failure_receipt_txs;
    std::vector<xcons_transaction_ptr_t> unchange_confirm_txs;
    std::vector<xcons_transaction_ptr_t> contract_create_txs;
    int32_t error_code = xsuccess;  // record the last failure tx error code
    std::shared_ptr<store::xaccount_context_t> _account_context = nullptr;
    xaccount_ptr_t proposal_state = nullptr;
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = nullptr;
    base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(prev_block, cs_para.get_clock());
    do {
        // clone new bstate firstly
        proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_header.get(), *prev_bstate.get());
        proposal_state = std::make_shared<xunit_bstate_t>(proposal_bstate.get());

        // create tx execute context
        _account_context = std::make_shared<store::xaccount_context_t>(proposal_state);
        _account_context->set_context_para(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_timestamp(), cs_para.get_total_lock_tgas_token());
        xassert(!cs_para.get_table_account().empty());
        xassert(cs_para.get_table_proposal_height() > 0);
        uint64_t table_committed_height = cs_para.get_table_proposal_height() >= 3 ? cs_para.get_table_proposal_height() - 3 : 0;
        _account_context->set_context_pare_current_table(cs_para.get_table_account(), table_committed_height);

        // clear old contract create txs
        contract_create_txs.clear();

        // try to execute all txs
        bool all_txs_succ = true;
        for (auto iter = exec_txs.begin(); iter != exec_txs.end();) {
            auto cur_tx = *iter;  // copy cur tx
            // execute input tx,
            int32_t action_ret = xtransaction_executor::exec_tx(_account_context.get(), cs_para, cur_tx, contract_create_txs);
            if (action_ret == xtransaction_confirm_state_unchange) {
                xdbg("xtransaction_executor::exec_batch_txs tx_hash:%s, type:%s", cur_tx->get_transaction()->get_digest_hex_str().c_str(), cur_tx->get_tx_subtype_str().c_str());
                unchange_confirm_txs.push_back(cur_tx);
                iter = exec_txs.erase(iter);
            } else if (action_ret) {
                error_code = action_ret;
                all_txs_succ = false;
                auto next_iter = exec_txs.erase(iter);  // erase fail iter and get next iter

                if (cur_tx->is_send_tx() || cur_tx->is_self_tx()) {  // erase next other send txs for nonce unmatching
                    failure_send_txs.push_back(cur_tx);
                    for (;next_iter != exec_txs.end();) {
                        auto next_tx = *next_iter;  // copy next tx
                        if ( (next_tx->is_send_tx() || next_tx->is_self_tx()) && (next_tx->get_tx_nonce() >= cur_tx->get_tx_nonce()) ) {
                            failure_send_txs.push_back(next_tx);
                            next_iter = exec_txs.erase(next_iter);  // get next next tx to compare
                            xwarn("xtransaction_executor::exec_batch_txs %s drop other sendtx=%s", cs_para.dump().c_str(), next_tx->dump().c_str());
                        } else {
                            break;  // break when no other send txs
                        }
                    }
                } else {
                    failure_receipt_txs.push_back(cur_tx);
                }
                break;  // break exec when one tx exec fail
            } else {
                 iter++;
            }
        }
        if (all_txs_succ == true) {
            break;  // break when all txs succ
        } else {
            if (exec_txs.size() == 0) {
                break; // break when all txs fail
            } else {
                xwarn("xtransaction_executor::exec_batch_txs retry exec again. %s,account=%s,height=%ld,origin_txs=%zu",
                    cs_para.dump().c_str(), _temp_header->get_account().c_str(), _temp_header->get_height(), txs.size());
                continue;  // retry execute left txs again
            }
        }
    } while(1);

    txs_result.m_exec_fail_txs = failure_send_txs;  // failure send txs need delete from txpool

    // merge all pack txs and set exec status
    std::vector<xcons_transaction_ptr_t> all_pack_txs;
    for (auto & tx : exec_txs) {
        tx->set_current_exec_status(enum_xunit_tx_exec_status_success);
        all_pack_txs.push_back(tx);
    }
    for (auto & tx : contract_create_txs) {
        tx->set_current_exec_status(enum_xunit_tx_exec_status_success);
        all_pack_txs.push_back(tx);
    }
    for (auto & tx : failure_receipt_txs) {
        tx->set_current_exec_status(enum_xunit_tx_exec_status_fail);
        xassert(tx->is_recv_tx());  // confirm tx should not fail
        all_pack_txs.push_back(tx);
    }
    for (auto & tx : unchange_confirm_txs) {
        tx->set_current_exec_status(enum_xunit_tx_exec_status_success);
        xassert(tx->is_confirm_tx());
        xdbg("xtransaction_executor::exec_batch_txs tx_hash:%s, type:%s", tx->get_transaction()->get_digest_hex_str().c_str(), tx->get_tx_subtype_str().c_str());
        // all_pack_txs.push_back(tx);
    }

    if (!txs.empty() && all_pack_txs.empty() && unchange_confirm_txs.empty()) {
        xassert(error_code != xsuccess);
        return error_code;
    }

    // update tx related propertys and other default propertys
    std::vector<xcons_transaction_ptr_t> succ_txs{all_pack_txs};
    succ_txs.insert(succ_txs.end(), unchange_confirm_txs.begin(), unchange_confirm_txs.end());
    if (!succ_txs.empty() && false == _account_context->finish_exec_all_txs(succ_txs)) {
        xerror("xtransaction_executor::exec_batch_txs fail-update tx info. %s,account=%s,height=%ld,origin_txs=%zu,all_txs=%zu",
            cs_para.dump().c_str(), _temp_header->get_account().c_str(), _temp_header->get_height(), txs.size(), all_pack_txs.size());
        return -1;
    }

    xtransaction_result_t result;
    _account_context->get_transaction_result(result);

    txs_result.m_exec_succ_txs = all_pack_txs;
    txs_result.m_exec_unchange_txs = unchange_confirm_txs;
    txs_result.m_unconfirm_tx_num = proposal_state->get_unconfirm_sendtx_num();
    txs_result.m_full_state = result.m_full_state;
    txs_result.m_property_binlog = result.m_property_binlog;
    txs_result.m_tgas_balance_change = _account_context->get_tgas_balance_change();
    xdbg_info("xtransaction_executor::exec_batch_txs %s,account=%s,height=%ld,origin=%zu,succ=%zu,fail_send=%zu,fail_recv=%zu,contract_txs=%zu,all_pack=%zu,binlog=%zu,state=%zu,tgas_balance_change=%lld",
        cs_para.dump().c_str(), _temp_header->get_account().c_str(), _temp_header->get_height(),
        txs.size(), exec_txs.size(), failure_send_txs.size(), failure_receipt_txs.size(), contract_create_txs.size(), all_pack_txs.size(),
        result.m_property_binlog.size(), result.m_full_state.size(), txs_result.m_tgas_balance_change);
    return xsuccess;
}

NS_END2
