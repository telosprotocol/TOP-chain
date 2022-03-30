// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action_session.h"

#include "xevm_contract_runtime/xevm_action_runtime.h"
#include "xevm_runner/evm_context.h"

NS_BEG2(top, contract_runtime)

xtop_action_session<data::xevm_consensus_action_t>::xtop_action_session(observer_ptr<xaction_runtime_t<data::xevm_consensus_action_t>> associated_runtime,
                                                                        observer_ptr<evm_statestore::xevm_statestore_helper_t> const & statestore_helper) noexcept
  : m_associated_runtime{associated_runtime}, m_statestore_helper{statestore_helper} {
}

xtransaction_execution_result_t xtop_action_session<data::xevm_consensus_action_t>::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action, contract_common::xcontract_execution_param_t const param) {
    assert(m_associated_runtime != nullptr);
    assert(m_statestore_helper != nullptr);
    assert(action != nullptr);

    std::unique_ptr<top::evm::xevm_context_t> ctx{top::make_unique<top::evm::xevm_context_t>(std::move(action), param)};
    auto observed_exectx = top::make_observer(ctx.get());

    return m_associated_runtime->execute(observed_exectx);
}

NS_END2