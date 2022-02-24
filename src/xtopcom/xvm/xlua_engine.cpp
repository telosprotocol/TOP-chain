// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbasic/xscope_executer.h"
#include "xerror/xvm_error.h"
#include "xvm/xvm_context.h"
#include "xvm/xvm_engine.h"
#include "xvm/xvm_lua_api.h"

NS_BEG2(top, xvm)
using base::xcontext_t;
using base::xstream_t;

xlua_engine::xlua_engine() {
    m_lua_mgr = luaL_newstate();
    if (m_lua_mgr == NULL) {
        xerror_lua("luaL_newstate error\n");
        return;
    }
    luaL_openlibs(m_lua_mgr);
}

void xlua_engine::register_function() {
    for (size_t i = 0; i < sizeof(g_lua_chain_func) / sizeof(xlua_chain_func); i++) {
        lua_register(m_lua_mgr, g_lua_chain_func[i].name, g_lua_chain_func[i].func);
    }
}

void xlua_engine::validate_script(const std::string &code, xvm_context &ctx) {
    try {
        if (ctx.m_contract_account.size() >= 64 ) {
            std::error_code ec{ enum_xvm_error_code::enum_lua_code_owern_error };
            top::error::throw_error(ec, "contract account length error");
        }

        auto parent_addr = ctx.m_contract_helper->get_parent_account();
        lua_setcontractaccount(m_lua_mgr, parent_addr.data(), parent_addr.size());
        lua_setuserdata(m_lua_mgr, reinterpret_cast<void*>(ctx.m_contract_helper.get()));

        if (luaL_loadstring(m_lua_mgr, code.c_str())) {
            string error_msg = lua_tostring(m_lua_mgr, -1);
            xkinfo_lua("load lua code error\n %s", code.c_str());
            std::error_code ec{ enum_xvm_error_code::enum_lua_code_parse_error };
            top::error::throw_error(ec, "lua load code error:" + error_msg);
        }

        if (lua_pcall(m_lua_mgr, 0, 0, 0)) {
            string error_msg = lua_tostring(m_lua_mgr, -1);
            xkinfo_lua("lua_pcall validate:%s", error_msg.c_str());
            std::error_code ec{ enum_xvm_error_code::enum_lua_code_parse_error };
            top::error::throw_error(ec, "lua_pcall validate error:" + error_msg);
        }
        register_function();
    } catch(top::error::xtop_error_t &) {
        throw;
    } catch(const std::exception& e) {
        xkinfo_lua("%s", e.what());
        std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
        top::error::throw_error(ec, "input param code_abi is not valid json");
    } catch(...) {
        std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
        top::error::throw_error(ec, "unkown exception");
    }
}

void xlua_engine::init_gas(xvm_context& ctx, int calc_gas) {
    // clean instructionCount
    lua_clean(m_lua_mgr);
    // set netusage
    //lua_setgaslimit(m_lua_mgr, ctx.m_trace_ptr->m_gas_limit);
    lua_setcalltgas(m_lua_mgr, calc_gas);
}

void xlua_engine::publish_script(const string& code, xvm_context& ctx) {
    xtop_scope_executer on_exit([&ctx, &m_lua_mgr = m_lua_mgr] {
        ctx.m_trace_ptr->m_instruction_usage = lua_getinstructioncount(m_lua_mgr);
    });
    init_gas(ctx, CALC_GAS_TRUE);
    validate_script(code, ctx);
    call_init();
}

void xlua_engine::load_script(const std::string &code, xvm_context &ctx) {
    lua_clean(m_lua_mgr);
    lua_setcalltgas(m_lua_mgr, CALC_GAS_FALSE);
    validate_script(code, ctx);
}

void xlua_engine::call_init() {
    int32_t ret = lua_getglobal(m_lua_mgr, "init");
    if (ret != 0 && lua_pcall(m_lua_mgr, 0, 0, 0)) {
        string error_msg = lua_tostring(m_lua_mgr, -1);
        xkinfo_lua("lua_pcall init:%s", error_msg.c_str());
        std::error_code ec{ enum_xvm_error_code::enum_lua_code_pcall_error };
        top::error::throw_error(ec, "lua_pcall init error:" + error_msg);
    }
}

int32_t xlua_engine::arg_parse(const string& action_param) {
    int32_t argn{0};
    try {
        if (!action_param.empty()) {
            xstream_t stream(xcontext_t::instance(), (uint8_t*)action_param.data(), action_param.size());
            uint8_t arg_num{0}, arg_type{0};
            //int64_t arg_int64{0};
            uint64_t arg_uint64{0};
            string arg_str;
            bool   arg_bool;
            stream >> arg_num;
            if (arg_num > MAX_ARG_NUM) {
                std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
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
                            std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
                            top::error::throw_error(ec, "arg string size " + std::to_string(arg_str.size()) + " length greater than 128");
                        }
                        lua_pushstring(m_lua_mgr, arg_str.c_str());
                        break;
                    case ARG_TYPE_BOOL:
                        stream >> arg_bool;
                        lua_pushboolean(m_lua_mgr, arg_bool);
                        break;
                    default:
                        std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
                        top::error::throw_error(ec, "param stream not valid");
                }
            }
        }
    } catch (top::error::xtop_error_t const &) {
        throw;
    } catch (enum_xerror_code& e) {
        std::error_code ec{ e };
        top::error::throw_error(ec, "action_param stream is not valid");
    } catch (const std::exception& e) {
        xkinfo_lua("%s", e.what());
        std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
        top::error::throw_error(ec, "param not valid");
    } catch(...) {
        std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
        top::error::throw_error(ec, "unkown exception");
    }
    return argn;
}

void xlua_engine::process(common::xaccount_address_t const &contract_account, const string &code, xvm_context &ctx) {
    xtop_scope_executer on_exit([&ctx, &m_lua_mgr = m_lua_mgr] {
        ctx.m_trace_ptr->m_instruction_usage = lua_getinstructioncount(m_lua_mgr);
        ctx.m_contract_helper->get_gas_and_disk_usage(ctx.m_trace_ptr->m_tgas_usage, ctx.m_trace_ptr->m_disk_usage);
    });
    if (ctx.m_exec_account.size() >= 64) {
        std::error_code ec{ enum_xvm_error_code::enum_lua_code_owern_error };
        top::error::throw_error(ec, "contract m_exec_account length error");
    }
    lua_setexecaccount(m_lua_mgr, ctx.m_exec_account.data(), ctx.m_exec_account.size());
    lua_setuserdata(m_lua_mgr, reinterpret_cast<void*>(ctx.m_contract_helper.get()));
    init_gas(ctx, CALC_GAS_TRUE);

    lua_getglobal(m_lua_mgr, ctx.m_action_name.c_str());

    try {
        if (ctx.m_action_name == "init") {
            std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
            top::error::throw_error(ec, "can't call init function");
        }

        if (lua_pcall(m_lua_mgr, arg_parse(ctx.m_action_para), 0, 0)) {
            string error_msg = lua_tostring(m_lua_mgr, -1);
            xkinfo_lua("lua_pcall:%s", error_msg.c_str());
            std::error_code ec{ enum_xvm_error_code::enum_lua_code_pcall_error };
            top::error::throw_error(ec, "lua_pcall error:" + error_msg);
        }
    } catch(top::error::xtop_error_t const &) {
        throw;
    } catch(const std::exception & e) {
        xkinfo_lua("%s", e.what());
        std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
        top::error::throw_error(ec, "param not valid");
    } catch(...) {
        std::error_code ec{ enum_xvm_error_code::enum_lua_abi_input_error };
        top::error::throw_error(ec, "unkown exception");
    }
}

void xlua_engine::close() {
    xdbg("close xlua_engine");
    if (m_lua_mgr != NULL) {
        lua_close(m_lua_mgr);
        m_lua_mgr = NULL;
    }
}

xlua_engine::~xlua_engine() {
    close();
}

NS_END2


