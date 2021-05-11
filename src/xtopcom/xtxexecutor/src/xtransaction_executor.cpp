// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>

#include "xtxexecutor/xtransaction_executor.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xdata/xgenesis_data.h"

NS_BEG2(top, txexecutor)

REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xtxexecutor, xunit_error_to_string, xconsensus_service_error_base+1, xconsensus_service_error_max);

int32_t xtransaction_executor::exec_one_tx(xaccount_context_t * account_context, const xcons_transaction_ptr_t & tx, xtransaction_result_t & result) {
    account_context->add_transaction(tx);
    xtransaction_context_t tx_context(account_context, tx);

    int32_t action_ret = tx_context.exec(result);
    if (action_ret) {
        xwarn("xtransaction_executor::exec_one_tx tx exec fail, tx=%s,error:%s",
            tx->dump().c_str(), chainbase::xmodule_error_to_str(action_ret).c_str());
        return action_ret;
    }

    // some native contract is called by chain timer transaction trigger, should check if property changed
    if (tx->is_self_tx() && data::is_sys_contract_address(common::xaccount_address_t{tx->get_source_addr()})) {
        if (!result.m_native_property.is_dirty() && result.m_prop_log == nullptr) {
            xinfo("xtransaction_executor::exec_one_tx tx exec no property change, tx=%s",
                tx->dump().c_str());
            return xunit_contract_exec_no_property_change;
        }
    }

    xkinfo("xtransaction_executor::exec_one_tx succ, tx=%s,tx_state=%s",
        tx->dump(true).c_str(), tx->dump_execute_state().c_str());
    return xsuccess;
}

int32_t xtransaction_executor::exec_tx(xaccount_context_t * account_context, const xcons_transaction_ptr_t & tx, xtransaction_result_t & result) {
    // exec the origin tx firstly
    if (tx->is_self_tx() || tx->is_send_tx()) {
        if (account_context->get_blockchain()->get_account_mstate().get_latest_send_trans_number() != tx->get_transaction()->get_last_nonce()
            || false == tx->get_transaction()->check_last_trans_hash(account_context->get_blockchain()->get_account_mstate().get_latest_send_trans_hash())) {
            xerror("xtransaction_executor::exec_tx tx not match. account=%s,account_tx_nonce=%ld,tx_last_nonce=%ld",
                account_context->get_address().c_str(), account_context->get_blockchain()->get_account_mstate().get_latest_send_trans_number(),
                tx->get_transaction()->get_last_nonce());
            return enum_xtxexecutor_error_tx_nonce_not_match;
        }
        // update account the latest send tx hash and nonce firstly, because it may create send tx to call other account
        account_context->get_blockchain()->set_account_send_trans_hash(tx->get_transaction()->digest());
        account_context->get_blockchain()->set_account_send_trans_number(tx->get_transaction()->get_tx_nonce());
    }

    uint64_t last_tx_nonce = account_context->get_blockchain()->get_account_mstate().get_latest_send_trans_number();
    int32_t ret = exec_one_tx(account_context, tx, result);
    if (ret != xsuccess) {
        xwarn("xtransaction_executor::exec_tx input tx fail. %s error:%s",
            tx->dump().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
        return ret;
    }

    // exec txs created by origin tx secondly, this tx must be a run contract transaction
    if (!result.m_contract_txs.empty()) {
        for (auto & new_tx : result.m_contract_txs) {
            xinfo("xtransaction_executor::exec_tx contract create tx. account_txnonce=%ld,input_tx:%s new_tx:%s",
                account_context->get_blockchain()->get_account_mstate().get_latest_send_trans_number(), tx->dump().c_str(), new_tx->dump().c_str());
            xassert(new_tx->get_transaction()->get_last_nonce() == last_tx_nonce);
            last_tx_nonce = new_tx->get_transaction()->get_tx_nonce();
            xassert(new_tx->get_source_addr() == account_context->get_address());
            ret = exec_one_tx(account_context, new_tx, result);
            if (ret != xsuccess) {
                xwarn("xtransaction_executor::exec_tx contract create tx fail. %s error:%s",
                    new_tx->dump().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
                return ret;
            }
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

        xtransaction_result_t result;
        int32_t action_ret = xtransaction_executor::exec_tx(account_context, tx, result);
        if (action_ret) {
            // record failure send tx which will be pop from txpool
            if (tx->is_send_tx() || tx->is_self_tx()) {
                txs_result.m_exec_fail_txs.push_back(tx);
                xwarn("xtransaction_executor::exec_batch_txs fail-for send tx exec error, tx=%s error:%s",
                    tx->dump(true).c_str(), chainbase::xmodule_error_to_str(action_ret).c_str());
                error_code = action_ret;
                continue;
            }

            // receive tx should always consensus success, contract only can exec one tx once time, TODO(jimmy) need record fail/success
            tx->set_current_exec_status(enum_xunit_tx_exec_status_fail);
            xassert(txs.size() == 1);
        } else {
            tx->set_current_exec_status(enum_xunit_tx_exec_status_success);
            txs_result.succ_txs_result = result;
        }

        txs_result.m_exec_succ_txs.push_back(tx);
        xkinfo("xtransaction_executor::exec_batch_txs tx exec succ, tx=%s,total_result:%s",
            tx->dump().c_str(), result.dump().c_str());
    }
    if (txs_result.m_exec_succ_txs.empty()) {
        xassert(error_code != xsuccess);
        xassert(!txs_result.m_exec_fail_txs.empty());
        return error_code;
    }
    return xsuccess;
}

NS_END2
