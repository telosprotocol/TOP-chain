// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <cstdint>
#include <mutex>
#include "xvm/xvm_engine.h"
extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}
NS_BEG2(top, xvm)
#define CALC_GAS_TRUE   1
#define CALC_GAS_FALSE  0
#define MAX_ARG_NUM     16
#define MAX_ARG_STRING_SIZE 128

class xlua_engine : public xengine, public std::enable_shared_from_this<xlua_engine>
{
public:
    xlua_engine();
    ~xlua_engine();
    void process(common::xaccount_address_t const & contract_account, const string& code, xvm_context& ctx) override;
    void validate_script(const string& code, xvm_context& ctx);
    void publish_script(const string& code, xvm_context& ctx);
    void load_script(const std::string &code, xvm_context &ctx);
    void call_init();
    void close();
    void register_function();
    void init_gas(xvm_context& ctx, int calc_gas);
    int32_t arg_parse(const string& action_param);
private:
    lua_State* m_lua_mgr;
};
NS_END2
