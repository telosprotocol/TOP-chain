// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xuser/xuser_action_runtime.h"

#include "xbasic/xerror/xerror.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xaction_session.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xcontract_runtime/xuser/xwasm/xwasm_engine.h"

NS_BEG2(top, contract_runtime)

using xuser_consensus_action_t = data::xconsensus_action_t<data::xtop_action_type_t::user>;

std::unique_ptr<xaction_session_t<xuser_consensus_action_t>> xtop_action_runtime<xuser_consensus_action_t>::new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) {
    return top::make_unique<xaction_session_t<xuser_consensus_action_t>>(top::make_observer(this), contract_state);
}

xtransaction_execution_result_t xtop_action_runtime<xuser_consensus_action_t>::execute(observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    xtransaction_execution_result_t result;

    try {
        // if (exe_ctx->transaction_type() == data::enum_xtransaction_type::xtransaction_type_deploy_wasm_contract) {
        //     auto action_data = exe_ctx->action_data();
        //     uint64_t tgas_limit{ 0 };
        //     xbyte_buffer_t code;
        //     base::xstream_t stream(base::xcontext_t::instance(), action_data.data(), action_data.size());
        //     stream >> tgas_limit;
        //     stream >> code;
        //     auto engine = std::make_shared<user::xwasm_engine_t>();
        //     engine->deploy_contract(code, exe_ctx);
        // } else {

        // }
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
