// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xuser_action_runtime.h"

#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xaction_session.h"
// #include "xcontract_common/xerror/xerror.h"
#include "xcontract_runtime/xerror/xerror.h"

NS_BEG2(top, contract_runtime)

std::unique_ptr<xaction_session_t<data::xconsensus_action_t<data::xtop_action_type_t::user>>> xtop_action_runtime<data::xconsensus_action_t<data::xtop_action_type_t::user>>::new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) {
    return top::make_unique<xaction_session_t<data::xconsensus_action_t<data::xtop_action_type_t::user>>>(top::make_observer(this), contract_state);
}

xtransaction_execution_result_t xtop_action_runtime<data::xconsensus_action_t<data::xtop_action_type_t::user>>::execute(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx) {
    xtransaction_execution_result_t result;

    // auto const vm_type = tx_ctx->vm_type(result.status.ec);
    if (result.status.ec) {
        return result;
    }

    try {
        //if (exe_ctx->transaction_type() == data::enum_xtransaction_type::xtransaction_type_create_contract_account) {
        //    auto action_data = exe_ctx->action_data();
        //    uint64_t tgas_limit{ 0 };
        //    std::string code;
        //    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)action_data.data(), action_data.size());
        //    stream >> tgas_limit;
        //    stream >> code;
        //    auto engine = std::make_shared<lua::xlua_engine>();
        //    //std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        //    engine->publish_script(code, exe_ctx);
        //    exe_ctx->contract_state()->deploy_src_code(code);
        //    // m_contract_helper->string_set(XPROPERTY_CONTRACT_TGAS_LIMIT_KEY, std::to_string(tgas_limit), true);
        //} else {
        //    auto engin = std::make_shared<lua::xlua_engine>();
        //    auto src_code = exe_ctx->contract_state()->src_code();
        //    engin->load_script(src_code, exe_ctx);
        //    engin->process(exe_ctx);
        //}
    } catch (error::xcontract_runtime_error_t const & eh) {
        result.status.ec = eh.code();
    } catch (std::exception const & eh) {
        result.status.ec = error::xerrc_t::unknown_error;
        result.status.extra_msg = eh.what();
    } catch (...) {
        result.status.ec = error::xerrc_t::unknown_error;
    }

    return result;
}

NS_END2
