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
    xdbg("wens_test, receipt data, size : %zu\n", receipt_data.size());

    xtransaction_execution_result_t result;
    std::unique_ptr<contract_common::xcontract_execution_context_t> execution_context{top::make_unique<contract_common::xcontract_execution_context_t>(std::move(action), m_contract_state)};

    xscope_executer_t reset_action{ [&execution_context] {
        execution_context->consensus_action_stage(data::xconsensus_action_stage_t::invalid);
    } };

    auto const stage = execution_context->action_stage();
    execution_context->consensus_action_stage(stage);
    if (stage == data::xenum_consensus_action_stage::send || stage == data::xenum_consensus_action_stage::confirm || stage == data::xenum_consensus_action_stage::self) {
        execution_context->contract_state(execution_context->sender());
    } else if (stage == data::xenum_consensus_action_stage::recv) {
        execution_context->contract_state(execution_context->recver());
    } else {
        assert(false);
    }
    auto observed_exectx = top::make_observer(execution_context.get());

    std::error_code ec;
    if (false == observed_exectx->verify_action(ec)) {
        assert(ec);
        result.status.ec = ec;
        return result;
    }

    assert(m_associated_runtime != nullptr);
    auto observed_exectx = top::make_observer(execution_context.get());

    xscope_executer_t reset_action{ [&execution_context] {
        execution_context->consensus_action_stage(data::xconsensus_action_stage_t::invalid);
    } };
    execution_context->consensus_action_stage(execution_context->action_stage());
    switch (execution_context->consensus_action_stage()) {
    case data::xconsensus_action_stage_t::send: {
        uint64_t old_unconfirm_tx_num = execution_context->contract_state()->unconfirm_sendtx_num();
        execution_context->contract_state()->unconfirm_sendtx_num(old_unconfirm_tx_num + 1);

        break;
    }

    case data::xconsensus_action_stage_t::recv: {
        if (!receipt_data.empty()) {
            xdbg("wens_test, recv stage set receipt data");
            execution_context->input_receipt_data(cons_action->receipt_data());
        }
        uint64_t old_recv_tx_num = execution_context->contract_state()->recvtx_num();
        execution_context->contract_state()->recvtx_num(old_recv_tx_num + 1);

        break;
    }

    case data::xconsensus_action_stage_t::confirm: {
        uint64_t old_unconfirm_tx_num = execution_context->contract_state()->unconfirm_sendtx_num();
        assert(old_unconfirm_tx_num > 0);
        execution_context->contract_state()->unconfirm_sendtx_num(old_unconfirm_tx_num - 1);

        break;
    }

    case data::xconsensus_action_stage_t::self: {
        assert(false);
        break;
    }

    default: {
        assert(false);
        break;
    }

    }

    xdbg("sender: %s, rever: %s, state addr: %s", execution_context->sender().c_str(), execution_context->recver().c_str(), execution_context->contract_state()->state_account_address().c_str());
    auto const& src_name = execution_context->action_name();
    auto const& src_data = execution_context->action_data();
    xdbg("source action name: %s, src action data size: %zu", src_name.c_str(), src_data.size());
    if (!src_data.empty()) {
        xdbg("wens_test, src_data not empty");
        data::xproperty_asset asset_out{data::XPROPERTY_ASSET_TOP, uint64_t{0}};
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)src_data.data(), src_data.size());
        stream >> asset_out.m_token_name;
        stream >> asset_out.m_amount;
        xdbg("source action name: %s, token_name: %s, token amount: %d", src_name.c_str(), asset_out.m_token_name.c_str(), (int32_t)asset_out.m_amount);
    }

    auto start_bin_size = observed_exectx->contract_state()->binlog_size();
    result = m_associated_runtime->execute(observed_exectx);
    xdbg("wens_test, xtop_action_session<ActionT>::execute_action, receipt data, size : %zu\n", result.output.receipt_data.size());
    if (result.status.ec) {
        return result;
    }
    auto end_bin_size = observed_exectx->contract_state()->binlog_size();

    if (execution_context->consensus_action_stage() == data::xconsensus_action_stage_t::send ||
        execution_context->consensus_action_stage() == data::xconsensus_action_stage_t::self) {
        if (start_bin_size == end_bin_size) {
            // not a fatal error
            // result.status.ec = error::xenum_errc::enum_bin_code_not_changed;
        }
    }

    return result;
}

NS_END2
