// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <vector>
#include "xvm_define.h"
#include "xbasic/xerror/xerror.h"
#include "xerror/xvm_error.h"
#include "xcontract_helper.h"
#include "xutility/xhash.h"

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}
using top::xvm::xcontract_helper;
using top::xvm::enum_xvm_error_code;
using top::data::xaction_asset_out;
using std::string;
using std::vector;

struct xlua_chain_func
{
	const char *name;
	lua_CFunction func;
};
#define DB_OP_INSTRUCTION_COUNT     50
#define FUNC_OP_INSTRUCTION_COUNT   5


static int L_require_owner(lua_State *L)
{
    lua_addinstructioncount(L, FUNC_OP_INSTRUCTION_COUNT);
    if (memcmp(lua_getcontractaccount(L), lua_getexecaccount(L), 64)) {
        lua_pushboolean(L, false);
    } else {
        lua_pushboolean(L, true);
    }
    return 1;
}

static int L_create_key(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->string_create(key);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "create_key error, key is not string");
    }
    lua_pushnil(L);
    return 0;
}

static int L_set_key(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        string key = luaL_checkstring(L, 1);
        string value = luaL_checkstring(L, 2);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->string_set(key, value);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "set_key error, key value is not string");
    }
    lua_pushnil(L);
    return 0;
}

static int L_get_key(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    string value{};
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        value = contract_helper->string_get(key);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "get_key error, key is not string");
    }
    lua_pushstring(L, value.c_str());
    return 1;
}

static int L_lcreate(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->list_create(key);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "lcreate error, key is not string");
    }
    lua_pushnil(L);
    return 0;
}

static int L_lpush(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        string key = luaL_checkstring(L, 1);
        string field = luaL_checkstring(L, 2);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->list_push_front(key, field);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "lpush error, key value is not string");
    }
    lua_pushnil(L);
    return 0;
}

static int L_rpush(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        string key = luaL_checkstring(L, 1);
        string field = luaL_checkstring(L, 2);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->list_push_back(key, field);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "rpush error, key value is not string");
    }
    lua_pushnil(L);
    return 0;
}

static int L_lpop(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    string value;
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->list_pop_front(key, value);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "lpop error, key is not string");
    }
    lua_pushstring(L, value.c_str());
    return 1;
}

static int L_rpop(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    string value;
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->list_pop_back(key, value);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "rpop error, key is not string");
    }
    lua_pushstring(L, value.c_str());
    return 1;
}

static int L_ldel(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    string value;
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->list_clear(key);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "ldel error, key is not string");
    }
    lua_pushnil(L);
    return 0;
}

static int L_llen(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    int32_t iLen{0};
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        iLen = contract_helper->list_size(key);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "llen error, key is not string");
    }
    lua_pushnumber(L, iLen);
    return 1;
}

static int L_lall(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        vector<string> result_list = std::move(contract_helper->list_get_all(key));
        if (result_list.empty()) {
            lua_pushnil(L);
        } else {
            lua_newtable(L);
            int i = 1;
            for (const auto& iter : result_list) {
                lua_pushnumber(L, i++);
                lua_pushstring(L, iter.c_str());
                lua_settable(L, -3);
            }
        }
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "lall error, key is not string");
    }
    return 1;
}

static int L_hcreate(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->map_create(key);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "hcreate error, key is not string");
    }
    lua_pushnil(L);
    return 0;
}

static int L_hset(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3)) {
        string key = luaL_checkstring(L, 1);
        string field = luaL_checkstring(L, 2);
        string value = luaL_checkstring(L, 3);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->map_set(key, field, value);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "hset error, key, field,value is not string");
    }
    lua_pushnil(L);
    return 0;
}

static int L_hget(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    string value{};
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        string key = luaL_checkstring(L, 1);
        string field = luaL_checkstring(L, 2);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        value = contract_helper->map_get(key, field);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "hget error, key is not string");
    }
    lua_pushstring(L, value.c_str());
    return 1;
}

static int L_hlen(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    int32_t iLen{0};
    if (lua_isstring(L, 1)) {
        string key = luaL_checkstring(L, 1);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        iLen = contract_helper->map_size(key);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "hlen error, key is not string");
    }
    lua_pushnumber(L, iLen);
    return 1;
}

static int L_hdel(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        string key = luaL_checkstring(L, 1);
        string field = luaL_checkstring(L, 2);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->map_remove(key, field);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "hdel error, key,field is not string");
    }
    lua_pushnil(L);
    return 0;
}

static int L_grant(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isnumber(L, 2)) {
        string grant_account = luaL_checkstring(L, 1);
        uint64_t amount = luaL_checknumber(L, 2);
        xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        contract_helper->create_transfer_tx(grant_account, amount);
    } else {
        std::error_code ec{ enum_xvm_error_code::enum_vm_exception };
        top::error::throw_error(ec, "grant error, key must string, value must number");
    }
    lua_pushnil(L);
    return 0;
}
static int L_get_balance(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
    uint64_t balance = contract_helper->get_balance();
    lua_pushnumber(L, balance);
    return 1;
}

static int L_get_pay_fee(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
    const xaction_asset_out& pay_info = contract_helper->get_pay_fee();
    lua_pushstring(L, pay_info.m_asset_out.m_token_name.c_str());
    lua_pushnumber(L, pay_info.m_asset_out.m_amount);
    return 3;
}

static int L_exec_account(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    lua_pushstring(L, lua_getexecaccount(L));
    return 1;
}

static int L_random_seed(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
    auto random_seed = contract_helper->get_random_seed();
    lua_pushnumber(L, top::utl::xxh64_t::digest(random_seed.c_str(), random_seed.size()));
    return 1;
}

static xlua_chain_func g_lua_chain_func[] = {
    { "require_owner_auth",     L_require_owner },
    { "exec_account",           L_exec_account },
    { "get_balance",            L_get_balance },
    { "get_pay_fee",            L_get_pay_fee },
    { "create_key",             L_create_key},
    { "set_key",                L_set_key},
    { "get_key",                L_get_key },
    { "lcreate",                L_lcreate },
    { "lpush",                  L_lpush },
    { "rpush",                  L_rpush },
    { "lpop",                   L_lpop },
    { "rpop",                   L_rpop },
    { "ldel",                   L_ldel },
    { "llen",                   L_llen },
    { "lall",                   L_lall },
    { "hcreate",                L_hcreate },
    { "hset",                   L_hset },
    { "hget",                   L_hget},
    { "hlen",                   L_hlen },
    { "hdel",                   L_hdel },
    { "grant",                  L_grant },
    { "random_seed",            L_random_seed },
};
