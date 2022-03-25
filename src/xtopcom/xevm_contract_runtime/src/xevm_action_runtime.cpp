// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action_runtime.h"

#include "xevm_contract_runtime/xevm_action_session.h"

NS_BEG2(top, contract_runtime)

// todo state store.?
xtop_action_runtime<data::xevm_consensus_action_t>::xtop_action_runtime(observer_ptr<evm::xevm_contract_manager_t> const & evm_contract_manager,
                                                                        observer_ptr<evm_statestore::xevm_statestore_helper_t> const & statestore_helper) noexcept
  : evm_contract_manager_{evm_contract_manager}, evm_statestore_helper_{statestore_helper} {
}

// todo this state? should be user's or contract's. User's account state
std::unique_ptr<xaction_session_t<data::xevm_consensus_action_t>> xtop_action_runtime<data::xevm_consensus_action_t>::new_session() {
    return top::make_unique<xaction_session_t<data::xevm_consensus_action_t>>(top::make_observer(this), evm_statestore_helper_);
}

xtransaction_execution_result_t xtop_action_runtime<data::xevm_consensus_action_t>::execute(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx) {
    return {};
}

NS_END2