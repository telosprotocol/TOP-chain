#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_runtime/xaction_runtime_fwd.h"
#include "xcontract_runtime/xaction_session_fwd.h"
#include "xdata/xtransaction.h"

NS_BEG2(top, contract_runtime)

template <typename ActionT>
class xtop_action_runtime {
public:
    xtop_action_runtime() = default;
    xtop_ruxtop_action_runtimentime_face(xtop_action_runtime const &) = delete;
    xtop_action_runtime & operator=(xtop_action_runtime const &) = delete;
    xtop_action_runtime(xtop_action_runtime &&) = default;
    xtop_action_runtime & operator=(xtop_action_runtime &&) = default;
    ~xtop_action_runtime() = default;

    std::unique_ptr<xaction_session_t<ActionT>> new_session(observer_ptr<contract_common::xcontract_state_t> contract_state);

    xtransaction_execution_result_t execute(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx);
};

NS_END2

#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xaction_session.h"

NS_BEG2(top, contract_runtime)

template <typename ActionT>
std::unique_ptr<xaction_sessionT<ActionT>> xtop_action_runtime<ActionT>::new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) {
    return top::make_unique<xaction_session_t<ActionT>>(top::make_observer(this), contract_state);
}

template <typename ActionT>
xtransaction_execution_result_t xtop_action_runtime<ActionT>::execute(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx) {
    xtransaction_execution_result_t result;

    // auto const vm_type = tx_ctx->vm_type(result.status.ec);
    if (result.status.ec) {
        return result;
    }

    try {
        if (exe_ctx->transaction_type() == data::enum_xtransaction_type::xtransaction_type_create_contract_account) {
            auto action_data = exe_ctx->action_data();
            uint64_t tgas_limit{ 0 };
            std::string code;
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)action_data.data(), action_data.size());
            stream >> tgas_limit;
            stream >> code;
            auto engine = std::make_shared<lua::xlua_engine>();
            //std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            engine->publish_script(code, exe_ctx);
            exe_ctx->contract_state()->deploy_src_code(code);
            // m_contract_helper->string_set(XPROPERTY_CONTRACT_TGAS_LIMIT_KEY, std::to_string(tgas_limit), true);
        } else {
            auto engin = std::make_shared<lua::xlua_engine>();
            auto src_code = exe_ctx->contract_state()->src_code();
            engin->load_script(src_code, exe_ctx);
            engin->process(exe_ctx);
        }
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
