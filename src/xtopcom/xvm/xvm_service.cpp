// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xvm_service.h"
#include "xbasic/xscope_executer.h"
#include "xbasic/xmodule_type.h"
#include "xerror/xvm_error.h"
#include "xvm_context.h"

using namespace top::data;

NS_BEG2(top, xvm)

REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xvm, xvm::xvm_error_to_string, (int32_t)xvm::enum_xvm_error_code::error_base + 1, (int32_t)xvm::enum_xvm_error_code::error_max);

xvm_service::xvm_service()
:m_vm_cache(16) {
}

xtransaction_trace_ptr xvm_service::deal_transaction(const xtransaction_ptr_t& trx, xaccount_context_t* account_context) {
    xinfo_lua("source action:%s",trx->get_source_action_str().c_str());
    xinfo_lua("target action:%s",trx->get_target_action_str().c_str());

    xtransaction_trace_ptr trace = std::make_shared<xtransaction_trace>();
    xtop_scope_executer on_exit([trace] {
        xinfo_lua("tgas micro seconds:%lld, %u, errno:%d, %s", trace->m_duration_us, trace->m_instruction_usage, static_cast<uint32_t>(trace->m_errno), trace->m_errmsg.c_str());
     });
    try {
        shared_ptr<xvm_context> trx_context = make_shared<xvm_context>(*this, trx, account_context, trace);
        trx_context->exec();
    } catch(top::error::xtop_error_t const & e) {
        xwarn_lua("%d,%s", e.code().value(), e.what());
        trace->m_errno = static_cast<top::xvm::enum_xvm_error_code>(e.code().value());
        trace->m_errmsg = e.what();
        return trace;
    } catch(const std::exception& e) {
        xkinfo_lua("%s", e.what());
        trace->m_errno = enum_xvm_error_code::enum_lua_exec_unkown_error;
        trace->m_errmsg = e.what();
        return trace;
    } catch(...) {
        xkinfo_lua("unkown exception");
        trace->m_errno = enum_xvm_error_code::enum_lua_exec_unkown_error;
        trace->m_errmsg = "unkown exception";
        return trace;
    }
    //xkinfo_lua("ret:%d,%s", static_cast<uint32_t>(trace->m_errno), trace->m_errmsg.c_str());
    return trace;
}

native_handler* xvm_service::get_native_handler(string action_name) {
    auto iter = m_native_func.m_native_func_map.find(action_name);
    if (iter != m_native_func.m_native_func_map.end()) {
        return &(iter->second);
    }
    return nullptr;
}

NS_END2
