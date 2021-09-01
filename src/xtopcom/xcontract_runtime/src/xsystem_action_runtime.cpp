// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xsystem/xsystem_action_runtime.h"

#include "xbasic/xerror/xerror.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xaction_session.h"

#include "xcontract_runtime/xerror/xerror.h"
#include "xcontract_runtime/xsystem_contract_manager.h"
#include "xcontract_runtime/xsystem/xsystem_contract_runtime_helper.h"
#include "xcontract_runtime/xuser/xwasm/xwasm_engine.h"

#include "xsystem_contracts/xtransfer_contract.h"

NS_BEG2(top, contract_runtime)

using xsystem_consensus_action_t = data::xconsensus_action_t<data::xtop_action_type_t::system>;

std::unique_ptr<xaction_session_t<xsystem_consensus_action_t>> xtop_action_runtime<xsystem_consensus_action_t>::new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) {
    return top::make_unique<xaction_session_t<xsystem_consensus_action_t>>(top::make_observer(this), contract_state);
}

xtransaction_execution_result_t xtop_action_runtime<xsystem_consensus_action_t>::execute(observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    xtransaction_execution_result_t result;

    try {
        // auto system_contract = xsystem_contract_manager_t::instance().system_contract(exe_ctx->contract_address());
        system_contracts::xbasic_system_contract_t* transfer_contract = new system_contracts::xtop_transfer_contract(exe_ctx);
        result = transfer_contract->execute(exe_ctx);
        result.binlog = exe_ctx->contract_state()->binlog();
        result.fullstate_log = exe_ctx->contract_state()->fullstate_bin();

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
