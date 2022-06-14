// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action_session.h"

#include "xevm_contract_runtime/xevm_action_runtime.h"

NS_BEG2(top, contract_runtime)

xtop_action_session<data::xevm_consensus_action_t>::xtop_action_session(observer_ptr<xaction_runtime_t<data::xevm_consensus_action_t>> associated_runtime) noexcept
  : m_associated_runtime{associated_runtime} {
}

evm_common::xevm_transaction_result_t xtop_action_session<data::xevm_consensus_action_t>::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action, txexecutor::xvm_para_t const & vm_para) {
    assert(m_associated_runtime != nullptr);
    assert(action != nullptr);

    std::unique_ptr<evm_runtime::xevm_context_t> exectx{top::make_unique<evm_runtime::xevm_context_t>(std::move(action), vm_para)};
    return m_associated_runtime->execute(top::make_observer(exectx.get()));
}

NS_END2
