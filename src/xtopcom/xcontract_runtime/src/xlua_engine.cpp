// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xuser/xlua/xlua_engine.h"

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

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xerror/xchain_error.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xbasic/xscope_executer.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xcontract_runtime/xuser/xlua/xlua_api.h"
#include "xcontract_runtime/xvm/xvm_define.h"

extern "C" {
#include <lauxlib.h>
#include <lualib.h>
}

NS_BEG4(top, contract_runtime, user, lua)

using base::xcontext_t;
using base::xstream_t;

xlua_engine::xlua_engine() {
    m_lua_mgr = luaL_newstate();
    if (m_lua_mgr == NULL) {
        xerror_lua("%s", "luaL_newstate error");
        return;
    }
    luaL_openlibs(m_lua_mgr);
}

xlua_engine::xlua_engine(xlua_engine && other) noexcept : m_lua_mgr {other.m_lua_mgr} {
    other.m_lua_mgr = nullptr;
}

xlua_engine & xlua_engine::operator=(xlua_engine && other) noexcept {
    if (this != std::addressof(other)) {
        if (m_lua_mgr != nullptr) {
            lua_close(m_lua_mgr);
        }

        m_lua_mgr = other.m_lua_mgr;
        other.m_lua_mgr = nullptr;
    }

    return *this;
}

xlua_engine::~xlua_engine() {
    if (m_lua_mgr != nullptr) {
        lua_clean(m_lua_mgr);
    }
}

void xlua_engine::register_function() {
    for (size_t i = 0; i < sizeof(g_lua_chain_func) / sizeof(xlua_chain_func); i++) {
        lua_register(m_lua_mgr, g_lua_chain_func[i].name, g_lua_chain_func[i].func);
    }
}

void xlua_engine::validate_script(const std::string & code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    if (exe_ctx->contract_address().value().size() >= 64) {
        std::error_code ec{ xerrc_t::enum_lua_code_owern_error };
        top::error::throw_error(ec, "contract account length error");
    }

    //auto parent_addr = ctx.contract_helper->get_parent_account();
    //lua_setcontractaccount(m_lua_mgr, parent_addr.data(), parent_addr.size());
    lua_setuserdata(m_lua_mgr, reinterpret_cast<void *>(exe_ctx->contract_state().get()));

    if (luaL_loadstring(m_lua_mgr, code.c_str())) {
        string error_msg = lua_tostring(m_lua_mgr, -1);
        xkinfo_lua("load lua code error\n %s", code.c_str());
        std::error_code ec{ xerrc_t::enum_lua_code_parse_error };
        top::error::throw_error(ec, "lua load code error:" + error_msg);
    }

    if (lua_pcall(m_lua_mgr, 0, 0, 0)) {
        string error_msg = lua_tostring(m_lua_mgr, -1);
        xkinfo_lua("lua_pcall validate:%s", error_msg.c_str());
        std::error_code ec{ xerrc_t::enum_lua_code_parse_error };
        top::error::throw_error(ec, "lua_pcall validate error:" + error_msg);
    }
    register_function();
}

//void xlua_engine::init_gas(observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx, int calc_gas) {
//    // clean instructionCount
//    lua_clean(m_lua_mgr);
//    // set netusage
//    // lua_setgaslimit(m_lua_mgr, ctx.m_trace_ptr->m_gas_limit);
//    lua_setcalltgas(m_lua_mgr, calc_gas);
//}

void xlua_engine::publish_script(const string & code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    auto * lua_mgr = m_lua_mgr;
    // xscope_executer_t on_exit([&ctx, lua_mgr] { ctx.execution_status->instruction_usage = lua_getinstructioncount(lua_mgr); });
    // init_gas(ctx, CALC_GAS_TRUE);
    validate_script(code, exe_ctx);
    call_init();
}

void xlua_engine::load_script(const std::string & code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    lua_clean(m_lua_mgr);
    lua_setcalltgas(m_lua_mgr, CALC_GAS_FALSE);
    validate_script(code, exe_ctx);
}

void xlua_engine::call_init() {
    int32_t ret = lua_getglobal(m_lua_mgr, "init");
    if (ret != 0 && lua_pcall(m_lua_mgr, 0, 0, 0)) {
        string error_msg = lua_tostring(m_lua_mgr, -1);
        xkinfo_lua("lua_pcall init:%s", error_msg.c_str());
        std::error_code ec{ xerrc_t::enum_lua_code_pcall_error };
        top::error::throw_error(ec, "lua_pcall init error:" + error_msg);
    }
}

const uint8_t ARG_TYPE_UINT64 = 1;
const uint8_t ARG_TYPE_STRING = 2;
const uint8_t ARG_TYPE_BOOL = 3;

int32_t xlua_engine::arg_parse(xbyte_buffer_t const & action_param) {
    int32_t argn{0};
    try {
        if (!action_param.empty()) {
            xstream_t stream(xcontext_t::instance(), (uint8_t *)action_param.data(), action_param.size());
            uint8_t arg_num{0}, arg_type{0};
            // int64_t arg_int64{0};
            uint64_t arg_uint64{0};
            string arg_str;
            bool arg_bool;
            stream >> arg_num;
            if (arg_num > MAX_ARG_NUM) {
                std::error_code ec{ xerrc_t::enum_lua_abi_input_error };
                top::error::throw_error(ec, "arg num " + std::to_string(arg_num) + " great than max number 16");
            }
            argn = arg_num;
            while (arg_num--) {
                stream >> arg_type;
                switch (arg_type) {
                case ARG_TYPE_UINT64:
                    stream >> arg_uint64;
                    lua_pushnumber(m_lua_mgr, arg_uint64);
                    break;
                case ARG_TYPE_STRING:
                    stream >> arg_str;
                    if (arg_str.size() > MAX_ARG_STRING_SIZE) {
                        std::error_code ec{ xerrc_t::enum_lua_abi_input_error };
                        top::error::throw_error(ec, "arg string size " + std::to_string(arg_str.size()) + " length greater than 128");
                    }
                    lua_pushstring(m_lua_mgr, arg_str.c_str());
                    break;
                case ARG_TYPE_BOOL:
                    stream >> arg_bool;
                    lua_pushboolean(m_lua_mgr, arg_bool);
                    break;
                default:
                {
                    std::error_code ec{ xerrc_t::enum_lua_abi_input_error };
                    top::error::throw_error(ec, "param stream not valid");
                }
                }
            }
        }
    } catch (top::error::xchain_error_t & e) {
        throw;
    } catch (const std::exception & e) {
        xkinfo_lua("%s", e.what());
        throw;
    } catch (...) {
        std::error_code ec{ xerrc_t::enum_lua_abi_input_error };
        top::error::throw_error(ec, "unkown exception");
    }
    return argn;
}

void xlua_engine::process(observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    auto * lua_mgr = m_lua_mgr;
    auto const contract_account = exe_ctx->contract_address();

    if (contract_account.value().size() >= 64) {
        std::error_code ec{ xerrc_t::enum_lua_code_owern_error };
        top::error::throw_error(ec, "contract m_exec_account length error");
    }
    lua_setexecaccount(m_lua_mgr, contract_account.c_str(), contract_account.size());
    lua_setuserdata(m_lua_mgr, reinterpret_cast<void *>(exe_ctx->contract_state().get()));
    // init_gas(ctx, CALC_GAS_TRUE);

    lua_getglobal(m_lua_mgr, exe_ctx->action_name().c_str());

    try {
        if (exe_ctx->action_name() == "init") {
            std::error_code ec{ xerrc_t::enum_lua_abi_input_error };
            top::error::throw_error(ec, "can't call init function");
        }

        if (lua_pcall(m_lua_mgr, arg_parse(exe_ctx->action_data()), 0, 0)) {
            string error_msg = lua_tostring(m_lua_mgr, -1);
            xkinfo_lua("lua_pcall:%s", error_msg.c_str());
            std::error_code ec{ xerrc_t::enum_lua_code_pcall_error };
            top::error::throw_error(ec, "lua_pcall error:" + error_msg);
        } else {
            // TODO: if luapcall is called with third arg non-zero, retrive the return value here
        }
    } catch (top::error::xchain_error_t const &) {
        throw;
    } catch (const std::exception & e) {
        xkinfo_lua("%s", e.what());
        top::error::throw_error(std::error_code{ xerrc_t::enum_lua_abi_input_error }, "param not valid");
    } catch (...) {
        top::error::throw_error(std::error_code{ xerrc_t::enum_lua_abi_input_error }, "unkown exception");
    }
}

void xlua_engine::close() {
    xdbg("close xlua_engine");
    if (m_lua_mgr != NULL) {
        lua_close(m_lua_mgr);
        m_lua_mgr = NULL;
    }
}


NS_END4
