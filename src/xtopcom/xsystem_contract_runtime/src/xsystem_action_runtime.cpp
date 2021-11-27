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

        // exe_ctx->system_contract(std::bind(&system::xsystem_contract_manager_t::system_contract, system_contract_manager_, std::placeholders::_1));
        auto system_contract = system_contract_manager_->system_contract(exe_ctx->contract_address());
        assert(system_contract != nullptr);

        auto start_bin_size = exe_ctx->contract_state()->binlog_size();
        result = system_contract->execute(exe_ctx);
        if (exe_ctx->consensus_action_stage() == data::xconsensus_action_stage_t::send || exe_ctx->consensus_action_stage() == data::xconsensus_action_stage_t::self) {
            system_contract->exec_delay_followup();
        }
        auto end_bin_size = exe_ctx->contract_state()->binlog_size();
        xdbg("[xtop_action_session::xtop_action_session] op code size, %" PRIu64 " -> %" PRIu64, start_bin_size, end_bin_size);
        if (exe_ctx->consensus_action_stage() == data::xconsensus_action_stage_t::send || exe_ctx->consensus_action_stage() == data::xconsensus_action_stage_t::self) {
            if (start_bin_size == end_bin_size) {
                result.status.ec = error::xerrc_t::vm_vote_proposal_exist_error;
            }
        }

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
