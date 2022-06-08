// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm/xevm.h"

#include "assert.h"
#include "xcontract_common/xerror/xerror.h"
#include "xdata/xconsensus_action.h"
#include "xdata/xtop_action.h"
#include "xdata/xtop_action_generator.h"
#include "xevm_contract_runtime/xevm_action_session.h"

NS_BEG2(top, evm)

xtop_evm::xtop_evm(statectx::xstatectx_face_ptr_t const evm_statectx)
  : m_evm_statectx{evm_statectx}
  , evm_action_runtime_{top::make_unique<contract_runtime::evm::xevm_action_runtime_t>(
        top::make_observer<contract_runtime::evm::xevm_contract_manager_t>(contract_runtime::evm::xevm_contract_manager_t::instance()),
        evm_statectx)} {
}

xtop_evm::xtop_evm(observer_ptr<contract_runtime::evm::xevm_contract_manager_t> const evm_contract_manager, statectx::xstatectx_face_ptr_t const evm_statectx)
  : m_evm_statectx{evm_statectx}, evm_action_runtime_{top::make_unique<contract_runtime::evm::xevm_action_runtime_t>(evm_contract_manager, evm_statectx)} {
}

txexecutor::enum_execute_result_type xtop_evm::execute(txexecutor::xvm_input_t const & input, txexecutor::xvm_output_t & output) {
    m_evm_statectx = input.get_statectx();

    contract_runtime::evm::xevm_output_t evm_output = execute(input.get_tx(), input.get_para());

    // output.used_gas = evm_output.used_gas;
    if (!evm_output.status.ec) {
        output.m_tx_result = evm_output.tx_result;
        return txexecutor::enum_exec_success;
    } else {
        output.m_ec = evm_output.status.ec;
        return txexecutor::enum_exec_error_vm_execute;
    }
}

contract_runtime::evm::xevm_output_t xtop_evm::execute(data::xcons_transaction_ptr_t const & tx, txexecutor::xvm_para_t const & vm_para) {
    auto action = contract_runtime::xaction_generator_t::generate(tx);

    contract_runtime::evm::xevm_output_t evm_output;

    try {
        auto action_result = execute_action(std::move(action), vm_para);
        evm_output.used_gas = action_result.used_gas;
        evm_output.tx_result = action_result;
        // result.transaction_results.emplace_back(action_result);
        // if (!action_result.status.ec) {
        //     xwarn(
        //         "[xtop_evm::execute] tx failed, category: %s, msg: %s, abort all txs after!", action_result.status.ec.category().name(),
        //         action_result.status.ec.message().c_str());
        //     result.status.ec = action_result.status.ec;
        // }
    } catch (top::error::xtop_error_t & eh) {
        evm_output.status.ec = eh.code();  // this should be implementation bug(or cases we don't charge). tx won't be made into blcok and so no gas cost.
        // xerror("xtop_evm: caught chain error exception: category: %s msg: %s", eh.code().category().name(), eh.what());
    } catch (std::exception const & eh) {
        xerror("xtop_evm: caught unknown exception: %s", eh.what());
    }

    return evm_output;
}

evm_common::xevm_transaction_result_t xtop_evm::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action, txexecutor::xvm_para_t const & vm_para) {
    assert(action->type() == data::xtop_action_type_t::evm);

    return evm_action_runtime_->new_session()->execute_action(std::move(action), vm_para);
}

NS_END2
