// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtvm_runtime/xtvm.h"

#include "xdata/xtop_action.h"
#include "xdata/xtop_action_generator.h"
#include "xtvm_runtime/xtvm_action_runner.h"
#include "xtvm_runtime/xtvm_context.h"

NS_BEG2(top, tvm)

xtop_vm::xtop_vm(statectx::xstatectx_face_ptr_t const statectx) : m_statectx{statectx} {
}

txexecutor::enum_execute_result_type xtop_vm::execute(txexecutor::xvm_input_t const & input, txexecutor::xvm_output_t & output) {
    m_statectx = input.get_statectx();
    auto action = contract_runtime::xaction_generator_t::generate(input.get_tx());

    evm_common::xevm_transaction_result_t action_result;

    try {
        action_result = execute_action(std::move(action), input.get_para());
    } catch (top::error::xtop_error_t & eh) {
        // should be implment bug here:
        xerror("tvm: caught error: %s %s", eh.category().name(), eh.what());
    } catch (std::exception const & eh) {
        xerror("tvm: caught unknown exception: %s", eh.what());
    }

    output.m_tx_result = action_result;
    if (action_result.status == evm_common::xevm_transaction_status_t::FAILED) {
        return txexecutor::enum_exec_error_vm_execute;
    }
    return txexecutor::enum_exec_success;
}

evm_common::xevm_transaction_result_t xtop_vm::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action, txexecutor::xvm_para_t const & vm_para) {
    // 1. build action runtime context with action and vm_para
    auto vm_context = top::make_unique<xtvm_context_t>(std::move(action), vm_para);

    // 2. run action runtime on this context, return `evm_common::xevm_transaction_result_t`
    auto vm_runner = xtvm_action_runner_t{m_statectx};
    return vm_runner.execute_action(std::move(vm_context));
}

NS_END2