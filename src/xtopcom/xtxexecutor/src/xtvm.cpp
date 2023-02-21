// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>

#include "xdata/xcons_transaction.h"
#include "xstore/xaccount_context.h"
#include "xtxexecutor/xtvm.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xtxexecutor/xvm_face.h"
#include "xtxexecutor/xaccount_vm.h"
#include "xdata/xblockbuild.h"

NS_BEG2(top, txexecutor)

enum_execute_result_type xtvm_t::execute(const xvm_input_t & input, xvm_output_t & output) {
    const xcons_transaction_ptr_t & tx = input.get_tx();
    // execute the first tx
    {
        common::xaccount_address_t address(tx->get_account_addr());
        data::xaccountstate_ptr_t accountstate = input.get_statectx()->load_account_state(address);
        if (nullptr == accountstate) {
            xerror("xtvm_t::execute fail-load unit state.tx=%s", tx->dump().c_str());
            return enum_exec_error_load_state;
        }

        if (accountstate->get_unitstate()->is_state_readonly()) {
            xerror("xtvm_t::execute fail-not same table unit state.tx=%s", tx->dump().c_str());
            return enum_exec_error_load_state;
        }
        xaccount_vm_t accountvm(accountstate, input.get_statectx());  // TODO(jimmy)
        int32_t ret = accountvm.execute(input, output);
        if (ret != xsuccess) {
            xwarn("xtvm_t::execute fail-vm execute.tx=%s,ret=%s", tx->dump().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
            return enum_exec_error_vm_execute;
        }
        output.m_total_gas_burn = accountvm.get_total_gas_burn();

        xdbg("xtvm_t::execute succ vm execute.tx=%s burn_gas= %ld", tx->dump().c_str(), output.m_total_gas_burn);
    }
    // execute the second inner table tx
    if (tx->is_send_tx() && tx->get_inner_table_flag()) {  // TODO(jimmy) only transfer now
        common::xaccount_address_t address(tx->get_target_addr());

        data::xaccountstate_ptr_t accountstate = input.get_statectx()->load_account_state(address);
        if (nullptr == accountstate) {
            xerror("xtvm_t::execute fail-load unit state.tx=%s", tx->dump().c_str());
            return enum_exec_error_load_state;
        }

        if (accountstate->get_unitstate()->is_state_readonly()) {
            xerror("xtvm_t::execute fail-not same table unit state.tx=%s", tx->dump().c_str());
            return enum_exec_error_load_state;
        }

        // TODO(jimmy) make a mock recvtx  performance
        base::xvaction_t srctx_action = data::xblockaction_build_t::make_tx_action(tx);
        base::xtx_receipt_ptr_t txreceipt = make_object_ptr<base::xtx_receipt_t>(srctx_action);
        data::xcons_transaction_ptr_t recvtx = make_object_ptr<data::xcons_transaction_t>(tx->get_transaction(), txreceipt);
        xassert(recvtx->is_recv_tx());
        xvm_input_t recv_input(input.get_statectx(), input.get_para(), recvtx);
        xvm_output_t recv_output;

        xaccount_vm_t accountvm(accountstate, input.get_statectx());
        int32_t ret = accountvm.execute(recv_input, recv_output);  // TODO(jimmy) ignore recvtx output
        if (ret != xsuccess) {
            xerror("xtvm_t::execute fail-vm execute.tx=%s", tx->dump().c_str());
            return enum_exec_error_vm_execute;
        }
        xdbg("xtvm_t::execute succ vm execute inner tx second phase.tx=%s", recvtx->dump().c_str());
    }

    return enum_exec_success;
}

NS_END2
