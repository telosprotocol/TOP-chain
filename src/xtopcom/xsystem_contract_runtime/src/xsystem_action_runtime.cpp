// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsystem_contract_runtime/xsystem_action_runtime.h"

#include "xbasic/xerror/xerror.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xaction_session.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xsystem_contract_runtime/xsystem_contract_runtime_helper.h"
#include "xsystem_contract_runtime/xsystem_contract_manager.h"
#include "xsystem_contracts/xtransfer_contract.h"

NS_BEG2(top, contract_runtime)

xtop_action_runtime<data::xsystem_consensus_action_t>::xtop_action_runtime(observer_ptr<system::xsystem_contract_manager_t> const & system_contract_manager) noexcept
  : system_contract_manager_{system_contract_manager} {
}

std::unique_ptr<xaction_session_t<data::xsystem_consensus_action_t>> xtop_action_runtime<data::xsystem_consensus_action_t>::new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) {
    return top::make_unique<xaction_session_t<data::xsystem_consensus_action_t>>(top::make_observer(this), contract_state);
}

xtransaction_execution_result_t xtop_action_runtime<data::xsystem_consensus_action_t>::execute(observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    xtransaction_execution_result_t result;

    try {
        assert(system_contract_manager_ != nullptr);

        auto system_contract = system_contract_manager_->system_contract(exe_ctx->contract_address());
        assert(system_contract != nullptr);

        result = system_contract->execute(exe_ctx);
    } catch (top::error::xtop_error_t const & eh) {
        result.status.ec = eh.code();
    } catch (std::exception const & eh) {
        result.status.ec = error::xerrc_t::unknown_error;
        result.status.extra_msg = eh.what();
    } catch (enum_xerror_code ec) {
        result.status.ec = ec;
    } catch (...) {
        result.status.ec = error::xerrc_t::unknown_error;
    }

    return result;
}

NS_END2
