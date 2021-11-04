// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xaction_execution_param.h"
#include "xcontract_common/xcontract_fwd.h"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_runtime/xaction_runtime_fwd.h"
#include "xcontract_runtime/xaction_session_fwd.h"
#include "xcontract_runtime/xtransaction_execution_result.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xconsensus_action_fwd.h"

#include <cinttypes>

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

#include "xbasic/xscope_executer.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xaction_runtime.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xdata/xconsensus_action.h"

NS_BEG2(top, contract_runtime)

template <typename ActionT>
xtop_action_session<ActionT>::xtop_action_session(observer_ptr<xaction_runtime_t<ActionT>> associated_runtime,
                                                  observer_ptr<contract_common::xcontract_state_t> contract_state) noexcept
  : m_associated_runtime{std::move(associated_runtime)}, m_contract_state{std::move(contract_state)} {
}

template <typename ActionT>
xtransaction_execution_result_t xtop_action_session<ActionT>::execute_action(std::unique_ptr<data::xbasic_top_action_t const> action) {
    assert(m_associated_runtime != nullptr);
    assert(action != nullptr);

    auto const * cons_action = static_cast<data::xsystem_consensus_action_t const *>(action.get());
    auto const receipt_data = cons_action->receipt_data();

    xtransaction_execution_result_t result;
    std::unique_ptr<contract_common::xcontract_execution_context_t> execution_context{top::make_unique<contract_common::xcontract_execution_context_t>(std::move(action), m_contract_state)};

    xscope_executer_t reset_action{ [&execution_context] {
        execution_context->consensus_action_stage(data::xconsensus_action_stage_t::invalid);
    } };

    execution_context->consensus_action_stage(execution_context->action_stage());
    switch (execution_context->consensus_action_stage()) {
    case data::xenum_consensus_action_stage::send:
    case data::xenum_consensus_action_stage::confirm:
    case data::xenum_consensus_action_stage::self:
        xdbg("[xtop_action_session::xtop_action_session] stage %d, set state address sender: %s", execution_context->consensus_action_stage(), execution_context->sender().value().c_str());
        execution_context->contract_state(execution_context->sender());
        break;

    case data::xenum_consensus_action_stage::recv:
        xdbg("[xtop_action_session::xtop_action_session] stage %d, set state address recver: %s", execution_context->consensus_action_stage(), execution_context->recver().value().c_str());
        execution_context->contract_state(execution_context->recver());
        break;

    default:
        assert(false);
        break;
    }

    auto observed_exectx = top::make_observer(execution_context.get());
    std::error_code ec;

    result.output.fee_change = observed_exectx->action_preprocess(ec);
    if (ec) {
        assert(ec);
        xwarn("[xtop_action_session::xtop_action_session] action_preprocess failed, category: %s, msg: %s", ec.category().name(), ec.message().c_str());
        result.status.ec = ec;
        return result;
    }

    if (observed_exectx->consensus_action_stage() == data::xconsensus_action_stage_t::recv) {
        if (!receipt_data.empty()) {
            observed_exectx->input_receipt_data(cons_action->receipt_data());
        }
    }

    if (observed_exectx->consensus_action_stage() == data::xconsensus_action_stage_t::confirm) {
        if (!receipt_data.empty()) {
            execution_context->input_receipt_data(cons_action->receipt_data());
        }

        if (data::xaction_consensus_exec_status::enum_xunit_tx_exec_status_success == observed_exectx->action_consensus_result()) {
            return result;
        }
    }

    result = m_associated_runtime->execute(observed_exectx);
    if (result.status.ec) {
        return result;
    }

    return result;
}

NS_END2
