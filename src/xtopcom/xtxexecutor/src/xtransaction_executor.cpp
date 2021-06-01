// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#include "xtxexecutor/xtransaction_executor.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xdata/xgenesis_data.h"

NS_BEG2(top, txexecutor)

REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xtxexecutor, xunit_error_to_string, xconsensus_service_error_base+1, xconsensus_service_error_max);

int32_t xtransaction_executor::exec_one_tx(xaccount_context_t * account_context, const xcons_transaction_ptr_t & tx) {
    if (false == account_context->add_transaction(tx)) {
        xerror("xtransaction_executor::exec_one_tx fail-add transaction.tx=%s", tx->dump(true).c_str());
        return -1;
    }

    size_t before_op_records_size = account_context->get_op_records_size();

    xtransaction_context_t tx_context(account_context, tx);
    int32_t action_ret = tx_context.exec();
    if (action_ret) {
        tx->set_current_exec_status(enum_xunit_tx_exec_status_fail);
        xwarn("xtransaction_executor::exec_one_tx tx exec fail, tx=%s,error:%s",
            tx->dump(true).c_str(), chainbase::xmodule_error_to_str(action_ret).c_str());
        return action_ret;
    }

    size_t after_op_records_size = account_context->get_op_records_size();
    if (tx->is_self_tx() || tx->is_send_tx()) {
        if (after_op_records_size == before_op_records_size) {
            tx->set_current_exec_status(enum_xunit_tx_exec_status_fail);
            xassert(data::is_sys_contract_address(common::xaccount_address_t{tx->get_source_addr()}));
            xinfo("xtransaction_executor::exec_one_tx tx exec no property change, tx=%s",
                tx->dump(true).c_str());
            return xunit_contract_exec_no_property_change;
        }
    }

    tx->set_current_exec_status(enum_xunit_tx_exec_status_success);
    xkinfo("xtransaction_executor::exec_one_tx succ, tx=%s,tx_state=%s",
        tx->dump(true).c_str(), tx->dump_execute_state().c_str());
    return xsuccess;
}

int32_t xtransaction_executor::exec_tx(xaccount_context_t * account_context, const xcons_transaction_ptr_t & tx) {
    int32_t ret = exec_one_tx(account_context, tx);
    if (ret != xsuccess) {
        xwarn("xtransaction_executor::exec_tx input tx fail. %s error:%s",
            tx->dump().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
        return ret;
    }
    account_context->update_succ_tx_info();

    auto & create_txs = account_context->get_create_txs();
    // exec txs created by origin tx secondly, this tx must be a run contract transaction
    if (!create_txs.empty()) {
        for (auto & new_tx : create_txs) {
            xinfo("xtransaction_executor::exec_tx contract create tx. account_txnonce=%ld,input_tx:%s new_tx:%s",
                account_context->get_blockchain()->get_latest_send_trans_number(), tx->dump(true).c_str(), new_tx->dump(true).c_str());
            ret = exec_one_tx(account_context, new_tx);
            if (ret != xsuccess && ret != xunit_contract_exec_no_property_change) {  // TODO(jimmy)
                xwarn("xtransaction_executor::exec_tx contract create tx fail. %s error:%s",
                    new_tx->dump().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
                return ret;
            }
            account_context->update_succ_tx_info();
        }
    }
    return xsuccess;
}

int32_t xtransaction_executor::exec_batch_txs(xaccount_context_t * account_context, const std::vector<xcons_transaction_ptr_t> & txs,
    xbatch_txs_result_t & txs_result) {
    int32_t error_code = xsuccess;
    const std::string & address = account_context->get_address();
    for (auto & tx : txs) {
        // if previous send tx fail, other send txs no need execute
        if (tx->is_send_tx() || tx->is_self_tx()) {
            if (!txs_result.m_exec_fail_txs.empty()) {
                txs_result.m_exec_fail_txs.push_back(tx);
                xwarn("xtransaction_executor::exec_batch_txs fail-for previous send tx exec error, tx=%s",
                    tx->dump(true).c_str());
                continue;
            }
        }

        // execute input tx,
        int32_t action_ret = xtransaction_executor::exec_tx(account_context, tx);
        if (action_ret) {
            account_context->revert_to_last_succ_result();  // fail tx should not modify account's propertys

            // record failure send tx which will be pop from txpool
            if (tx->is_send_tx() || tx->is_self_tx()) {
                txs_result.m_exec_fail_txs.push_back(tx);
                xwarn("xtransaction_executor::exec_batch_txs fail-for send tx exec error, tx=%s error:%s",
                    tx->dump(true).c_str(), chainbase::xmodule_error_to_str(action_ret).c_str());
                error_code = action_ret;
                continue;
            } else {
                txs_result.m_exec_succ_txs.push_back(tx);  // fail recv/confirm tx should always be included in block
            }
        } else {
            account_context->save_succ_result();

            txs_result.m_exec_succ_txs.push_back(tx);
        }

        xkinfo("xtransaction_executor::exec_batch_txs tx exec succ, tx=%s",
            tx->dump().c_str());
    }
    if (txs_result.m_exec_succ_txs.empty()) {
        xassert(error_code != xsuccess);
        xassert(!txs_result.m_exec_fail_txs.empty());
        return error_code;
    }

    xtransaction_result_t result;
    account_context->get_transaction_result(result);
    txs_result.succ_txs_result = result;
    return xsuccess;
}

NS_END2
