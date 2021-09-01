// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xsystem/xsystem_action_runtime.h"

#include "xbasic/xerror/xerror.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xaction_session.h"
#include "xcontract_runtime/xerror/xerror.h"

#include "xvm/xsystem_contracts/xdemo/xdemo_contract.h"
// #include "xcontract_runtime/xuser/xwasm/xwasm_engine.h"

// #include "xvm/manager/xcontract_manager.h"
// #include "xvm/xcontract/xcontract_base.h"
// #include "xvm/xvm_trace.h"
// #include "xvm/xvm_context.h"

NS_BEG2(top, contract_runtime)

using xsystem_consensus_action_t = data::xconsensus_action_t<data::xtop_action_type_t::system>;

std::unique_ptr<xaction_session_t<xsystem_consensus_action_t>> xtop_action_runtime<xsystem_consensus_action_t>::new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) {
    return top::make_unique<xaction_session_t<xsystem_consensus_action_t>>(top::make_observer(this), contract_state);
}

xtransaction_execution_result_t xtop_action_runtime<xsystem_consensus_action_t>::execute(observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    xtransaction_execution_result_t result;

    try {

        if (exe_ctx->transaction_type() == data::enum_xtransaction_type::xtransaction_type_run_contract) {
            std::string temp = exe_ctx->recver().to_string();
            if (temp == "T2000138SJedyci3eaZN1XXC2wW79RYwgYh26n2cW") {
                xvm::system_contracts::xdemo_contract_t demo_contract;
                demo_contract.execute(exe_ctx);
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
