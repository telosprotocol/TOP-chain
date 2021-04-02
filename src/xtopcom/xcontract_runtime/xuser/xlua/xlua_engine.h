// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_runtime/xuser/xlua/xengine.h"

extern "C" {
#include <lua.h>
}

#include <memory>
#include <string>


NS_BEG4(top, contract_runtime, user, lua)

#define CALC_GAS_TRUE 1
#define CALC_GAS_FALSE 0
#define MAX_ARG_NUM 16
#define MAX_ARG_STRING_SIZE 128

class xlua_engine {
public:
    xlua_engine();
    xlua_engine(xlua_engine const &) = delete;
    xlua_engine & operator=(xlua_engine const &) = delete;
    xlua_engine(xlua_engine && other) noexcept;
    xlua_engine & operator=(xlua_engine && other) noexcept;
    ~xlua_engine();

    observer_ptr<contract_common::xcontract_execution_context_t> m_exe_ctx;

    void process(observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx);
    void validate_script(const std::string & code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx);
    void publish_script(const std::string & code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx);
    void load_script(const std::string & code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx);
    void call_init();
    void close();
    void register_function();
    int32_t arg_parse(xbyte_buffer_t const & action_param);

private:
    lua_State * m_lua_mgr;
};
NS_END4
