// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action_session.h"

#include "xevm_runner/evm_engine_interface.h"
#include "xevm_runner/evm_import_instance.h"
#include "xevm_runner/evm_logic.h"
#include "xevm_runner/evm_storage_face.h"

NS_BEG2(top, contract_runtime)

xtop_action_session<data::xevm_consensus_action_t>::xtop_action_session(observer_ptr<xaction_runtime_t<data::xevm_consensus_action_t>> associated_runtime,
                                                                        observer_ptr<evm_statestore::xevm_statestore_helper_t> const & statestore_helper) noexcept
  : m_associated_runtime{associated_runtime}, m_statestore_helper{statestore_helper} {
}

xtransaction_execution_result_t xtop_action_session<data::xevm_consensus_action_t>::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action) {
    // mock:
    top::evm::xtop_evm_context context{{}, {}, {}};
    top::evm::xtop_evm_logic evm_logic{nullptr, context};

    top::evm::evm_import_instance::instance()->set_evm_logic(evm_logic);
    call_contract();

    return {};
}

NS_END2