
#include "xcontract_runtime/xuser/xuser_contract_runtime.h"

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xcontext.h"
#include "xbase/xmem.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xerror/xchain_error.h"
#include "xbasic/xutility.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xcontract_runtime/xuser/xlua/xlua_engine.h"
#ifdef DBUILD_RUSTVM
#include "xcontract_runtime/xuser/xwasm/xwasm_engine.h"
#endif
#include <cassert>

NS_BEG3(top, contract_runtime, user)

xtransaction_execution_result_t xtop_user_contract_runtime::execute_transaction(observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    xtransaction_execution_result_t result;

    // auto const vm_type = tx_ctx->vm_type(result.status.ec);
    if (result.status.ec) {
        return result;
    }

    try {
        if (exe_ctx->transaction_type() == data::enum_xtransaction_type::xtransaction_type_create_contract_account) {
            auto action_data = exe_ctx->action_data();
            uint64_t tgas_limit{0};
            std::string code;
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)action_data.data(), action_data.size());
            stream >> tgas_limit;
            stream >> code;
            auto engine = std::make_shared<lua::xlua_engine>();
            engine->publish_script(code, exe_ctx);
            exe_ctx->contract_state()->deploy_src_code(code);
        #ifdef DBUILD_RUSTVM
        } else if (exe_ctx->transaction_type() == data::enum_xtransaction_type::xtransaction_type_deploy_wasm_contract) {
            auto action_data = exe_ctx->action_data();
            uint64_t tgas_limit{0};
            xbyte_buffer_t code;
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)action_data.data(), action_data.size());
            stream >> tgas_limit;
            stream >> code;
            auto engine = std::make_shared<user::xwasm_engine_t>();
            //std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
            engine->deploy_contract(code, exe_ctx);
        #endif
        } else {
            auto engin = std::make_shared<lua::xlua_engine>();
            auto src_code = exe_ctx->contract_state()->src_code();
            engin->load_script(src_code, exe_ctx);
            engin->process(exe_ctx);
        }
    } catch (top::error::xchain_error_t const & eh) {
        result.status.ec = eh.code();
    } catch (std::exception const & eh) {
        result.status.ec = error::xerrc_t::unknown_error;
        result.status.extra_msg = eh.what();
    } catch (...) {
        result.status.ec = error::xerrc_t::unknown_error;
    }

    return result;
}

NS_END3
