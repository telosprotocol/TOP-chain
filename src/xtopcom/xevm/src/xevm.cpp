// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm/xevm.h"

#include "assert.h"
#include "xdata/xtop_action.h"
#include "xevm_contract_runtime/xevm_action_session.h"

NS_BEG2(top, evm)

xtop_evm::xtop_evm(observer_ptr<contract_runtime::evm::xevm_contract_manager_t> const & evm_contract_manager,
                   observer_ptr<evm_statestore::xevm_statestore_helper_t> const & evm_statestore_helper)
  : evm_statestore_helper_{evm_statestore_helper}
  , evm_action_runtime_{top::make_unique<contract_runtime::evm::xevm_action_runtime_t>(evm_contract_manager, evm_statestore_helper)} {
}

xevm_output_t xtop_evm::execute(std::vector<data::xcons_transaction_ptr_t> const & txs, data::xblock_consensus_para_t const & cs_para) {
    return {};
}

contract_runtime::xtransaction_execution_result_t xtop_evm::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action,
                                                                           contract_common::xcontract_execution_param_t const & param
                                                                           //  , state_accessor::xstate_accessor_t & ac
) {
    assert(action->type() == data::xtop_action_type_t::evm);
    return evm_action_runtime_->new_session()->execute_action(std::move(action));
    // return {};
}

NS_END2