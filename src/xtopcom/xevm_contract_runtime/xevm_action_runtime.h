// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_runtime/xaction_runtime.h"
#include "xdata/xconsensus_action.h"
#include "xevm_contract_runtime/xevm_contract_manager.h"
#include "xevm_statestore_helper/xstatestore_helper.h"

NS_BEG2(top, contract_runtime)

template <>
class xtop_action_runtime<data::xevm_consensus_action_t> {
private:
    observer_ptr<evm::xevm_contract_manager_t> evm_contract_manager_;

    observer_ptr<evm_statestore::xevm_statestore_helper_t> evm_statestore_helper_;

public:
    // xtop_action_runtime() = default;
    xtop_action_runtime(xtop_action_runtime const &) = delete;
    xtop_action_runtime & operator=(xtop_action_runtime const &) = delete;
    xtop_action_runtime(xtop_action_runtime &&) = default;
    xtop_action_runtime & operator=(xtop_action_runtime &&) = default;
    ~xtop_action_runtime() = default;

    xtop_action_runtime(observer_ptr<evm::xevm_contract_manager_t> const & evm_contract_manager,
                        observer_ptr<evm_statestore::xevm_statestore_helper_t> const & statestore_helper) noexcept;

    // todo this state? should be user's or contract's. User's account state or none.
    // std::unique_ptr<xaction_session_t<data::xevm_consensus_action_t>> new_session(observer_ptr<contract_common::xcontract_state_t> contract_state);

    std::unique_ptr<xaction_session_t<data::xevm_consensus_action_t>> new_session();

    xtransaction_execution_result_t execute(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx);
};

NS_END2

NS_BEG3(top, contract_runtime, evm)
using xevm_action_runtime_t = xaction_runtime_t<data::xevm_consensus_action_t>;
NS_END3
