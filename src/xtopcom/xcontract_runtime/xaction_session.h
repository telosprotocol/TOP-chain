// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xaction_execution_param.h"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_runtime/xaction_runtime_fwd.h"
#include "xcontract_runtime/xaction_session_fwd.h"
#include "xcontract_runtime/xtransaction_execution_result.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xconsensus_action_fwd.h"

NS_BEG2(top, contract_runtime)

template <typename ActionT>
class xtop_action_session {
private:
    observer_ptr<xaction_runtime_t<ActionT>> m_associated_runtime;
    observer_ptr<contract_common::xcontract_state_t> m_contract_state;

public:
    xtop_action_session(xtop_action_session const &) = delete;
    xtop_action_session & operator=(xtop_action_session const &) = delete;
    xtop_action_session(xtop_action_session &&) = default;
    xtop_action_session & operator=(xtop_action_session &&) = default;
    ~xtop_action_session() = default;

    xtop_action_session(observer_ptr<xaction_runtime_t<ActionT>> associated_runtime, observer_ptr<contract_common::xcontract_state_t> contract_state) noexcept;

    xtransaction_execution_result_t execute_action(std::unique_ptr<data::xbasic_top_action_t const> action);
};

template <typename ActionT>
using xaction_session_t = xtop_action_session<ActionT>;

NS_END2

#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xaction_runtime.h"
#include "xbasic/xscope_executer.h"
#include "xdata/xconsensus_action.h"

NS_BEG2(top, contract_runtime)

template <typename ActionT>
xtop_action_session<ActionT>::xtop_action_session(observer_ptr<xaction_runtime_t<ActionT>> associated_runtime,
                                                  observer_ptr<contract_common::xcontract_state_t> contract_state) noexcept
  : m_associated_runtime{std::move(associated_runtime)}, m_contract_state{std::move(contract_state)} {
}

template <typename ActionT>
xtransaction_execution_result_t xtop_action_session<ActionT>::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action) {
    xtransaction_execution_result_t result;
    std::unique_ptr<contract_common::xcontract_execution_context_t> execution_context{top::make_unique<contract_common::xcontract_execution_context_t>(std::move(action), m_contract_state)};

    std::error_code ec;
    if (false == execution_context->verify_action(ec)) {
        assert(ec);
        result.status.ec = ec;
    }

    assert(m_associated_runtime != nullptr);
    auto observed_exectx = top::make_observer(execution_context.get());

    xscope_executer_t reset_action{ [&execution_context] {
        execution_context->consensus_action_stage(data::xconsensus_action_stage_t::invalid);
    } };
    auto stage = execution_context->action_stage();
    execution_context->consensus_action_stage(stage);
    if (stage == data::xconsensus_action_stage_t::send) {
        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::source_action);
    } else if (stage == data::xconsensus_action_stage_t::recv || stage == data::xconsensus_action_stage_t::self) {
        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::target_action);
    } else if (stage == data::xconsensus_action_stage_t::confirm) {
        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::confirm_action);
    } else {
        assert(false);
    }
    result = m_associated_runtime->execute(observed_exectx);
    if (result.status.ec) {
        return result;
    }

    return result;
}

NS_END2
