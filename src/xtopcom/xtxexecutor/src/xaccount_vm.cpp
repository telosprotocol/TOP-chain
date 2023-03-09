// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>

#include "xchain_fork/xutility.h"
#include "xtxexecutor/xaccount_vm.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xtxexecutor/xaccount_vm.h"
#include "xtxexecutor/xtransaction_context.h"

NS_BEG2(top, txexecutor)

xaccount_vm_t::xaccount_vm_t(const data::xaccountstate_ptr_t & accountstate, const statectx::xstatectx_face_ptr_t & statectx) {
    m_account_context = std::make_shared<store::xaccount_context_t>(accountstate->get_unitstate(), statectx, accountstate->get_tx_nonce());
}

int32_t xaccount_vm_t::exec_one_tx(store::xaccount_context_t * account_context, const xcons_transaction_ptr_t & tx) {
    if (false == account_context->add_transaction(tx)) {
        xerror("xaccount_vm_t::exec_one_tx fail-add transaction, tx=%s",
            tx->dump().c_str());
        return enum_xtxexecutor_error_tx_nonce_not_match;  // may happen when failure tx is deleted
    }

    xtransaction_context_t tx_context(account_context, tx);
    int32_t action_ret = tx_context.exec();

    bool const sys_contract = base::xvaccount_t::get_addrtype_from_account(tx->get_account_addr()) == base::enum_vaccount_addr_type_native_contract;
    XMETRICS_GAUGE(metrics::txexecutor_total_system_contract_count, static_cast<int64_t>(sys_contract));

    if (sys_contract) {
        XMETRICS_GAUGE(metrics::txexecutor_system_contract_failed_count, static_cast<int64_t>(static_cast<bool>(action_ret)));
    }    

    return action_ret;
}

int32_t xaccount_vm_t::exec_tx(store::xaccount_context_t * account_context, const xcons_transaction_ptr_t & tx, std::vector<xcons_transaction_ptr_t> & contract_create_txs) {
    // uint64_t now = cs_para.get_gettimeofday_s();
    // uint64_t delay_time_s = tx->get_transaction()->get_delay_from_fire_timestamp(now);
    // if (tx->is_self_tx() || tx->is_send_tx()) {
    //     XMETRICS_GAUGE(metrics::txdelay_from_client_to_sendtx_exec, delay_time_s);
    // } else if (tx->is_recv_tx()) {
    //     XMETRICS_GAUGE(metrics::txdelay_from_client_to_recvtx_exec, delay_time_s);
    // } else if (tx->is_confirm_tx()) {
    //     XMETRICS_GAUGE(metrics::txdelay_from_client_to_confirmtx_exec, delay_time_s);
    // }

    int32_t ret = exec_one_tx(account_context, tx);
    if (ret != xsuccess) {
        xwarn("xaccount_vm_t::exec_tx input tx fail. %s error:%s",
            tx->dump().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
        return ret;
    }
    if (false == account_context->get_blockchain()->is_state_dirty()) {
        if (tx->is_send_or_self_tx()) {
            // XTODO the send or self tx will not be packed if no property changed
            xwarn("xaccount_vm_t::exec_tx input send tx no property change. %s", tx->dump().c_str());
            return xunit_contract_exec_no_property_change;
        }
    }

    // copy create txs from account context
    std::vector<xcons_transaction_ptr_t> create_txs = account_context->get_create_txs();
    // exec txs created by origin tx secondly, this tx must be a run contract transaction
    if (!create_txs.empty()) {
        for (auto & new_tx : create_txs) {            
            // TODO(jimmy) cross tx need not confirm now
            new_tx->set_not_need_confirm();
            ret = exec_one_tx(account_context, new_tx);
            if (ret != xsuccess) {  // contract create tx send action may not change property, it's ok
                xwarn("xaccount_vm_t::exec_tx contract create tx fail. input_tx:%s new_tx:%s error:%s",
                    new_tx->dump().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
                return ret;
            } else {
                xinfo("xaccount_vm_t::exec_tx contract create tx succ. input_tx:%s new_tx:%s",
                    tx->dump().c_str(), new_tx->dump(true).c_str());
            }
            // new_tx->set_push_pool_timestamp(now);
            contract_create_txs.push_back(new_tx);  // return create tx for unit pack
        }
    }
    xdbg("xaccount_vm_t::exec_tx succ. tx=%s,tx_state=%s", tx->dump().c_str(), tx->dump_execute_state().c_str());
    return xsuccess;
}

int32_t xaccount_vm_t::execute(const xvm_input_t & input, xvm_output_t & output) {
    m_account_context->set_context_para(input.get_para().get_clock(), input.get_para().get_random_seed(), input.get_para().get_timestamp(), input.get_para().get_lock_tgas_token());
    const std::string & table_address = input.get_statectx()->get_table_address();
    xassert(!table_address.empty());
    uint64_t table_proposal_height = input.get_statectx()->get_table_state()->height();
    uint64_t table_committed_height = (table_proposal_height <= 3) ? 0 : (table_proposal_height - 3);
    m_account_context->set_context_pare_current_table(table_address, table_committed_height);

    std::vector<xcons_transaction_ptr_t> contract_create_txs;
    int32_t ret = exec_tx(m_account_context.get(), input.get_tx(), contract_create_txs);
    if (ret != xsuccess) {
        return ret;
    }
    output.logs = m_account_context->logs();
    output.m_contract_create_txs = contract_create_txs;
    output.m_tgas_balance_change = m_account_context->get_tgas_balance_change();
    return xsuccess;
}


uint64_t xaccount_vm_t::get_total_gas_burn() const
{
    if (m_account_context) {
        return m_account_context->get_total_gas_burn();
    }
    return 0;
}

NS_END2
