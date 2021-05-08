// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <vector>
// #include "xvm_define.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xproperties/xproperty_identifier.h"
#include "xcommon/xaddress.h"
// #include "xutility/xhash.h"

extern "C"
{
    #include <lua.h>
    #include <lualib.h>
    #include <lauxlib.h>
}

using top::contract_runtime::error::xerrc_t;
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


static int L_grant(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isnumber(L, 2)) {
        string grant_account = luaL_checkstring(L, 1);
        uint64_t amount = luaL_checknumber(L, 2);
        // xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
        // contract_helper->create_transfer_tx(grant_account, amount);
    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "set_key error, key value is not string"};
    }
    lua_pushnil(L);
    return 0;
}
static int L_get_balance(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    // xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
    // uint64_t balance = contract_helper->get_balance();
    // lua_pushnumber(L, balance);
    return 1;
}

static int L_get_pay_fee(lua_State *L)
{
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    // xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
    // const xaction_asset_out& pay_info = contract_helper->get_pay_fee();
    // lua_pushstring(L, pay_info.m_asset_out.m_token_name.c_str());
    // lua_pushnumber(L, pay_info.m_asset_out.m_amount);
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
    // xcontract_helper* contract_helper = reinterpret_cast<xcontract_helper*>(lua_getuserdata(L));
    // auto random_seed = contract_helper->get_random_seed();
    // lua_pushnumber(L, top::utl::xxh64_t::digest(random_seed.c_str(), random_seed.size()));
    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int L_map_prop_create(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    std::string user = lua_getexecaccount(L);
    if (lua_isstring(L, 1)) {
        // std::string prop_name = luaL_checkstring(L, 1);
        // top::contract_common::xaccount_state_t* account_state = reinterpret_cast<top::contract_common::xaccount_state_t*>(lua_getuserdata(L));
        // account_state->access_control()
        // account_state->api_handler()->MAP_PROP_CREATE<std::string, std::string>(prop_name);

    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_map_prop_create params error"};
    }

    return 0;
}


static int L_map_prop_exist(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        std::string prop_name = luaL_checkstring(L, 1);
        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));

        // if (account_state->api_handler()->MAP_PROP_EXIST(prop_name)) {
        //     lua_pushboolean(L, 1);
        // } else {
        //     lua_pushboolean(L, 0);
        // }
    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_map_prop_exist params error"};
    }

    return 1;
}

static int L_map_prop_add(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3)) {
        // std::string prop_name = luaL_checkstring(L, 1);
        // std::string key = luaL_checkstring(L, 2);
        // std::string value = luaL_checkstring(L, 3);

        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));

        // account_state->api_handler()->MAP_PROP_ADD<std::string, std::string>(prop_name, key, value);

    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_map_prop_add params error"};
    }

    return 0;
}

static int L_map_prop_update(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2) && lua_isstring(L, 3)) {
        // std::string prop_name = luaL_checkstring(L, 1);
        // std::string key = luaL_checkstring(L, 2);
        // std::string value = luaL_checkstring(L, 3);

        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));
        // account_state->api_handler()->MAP_PROP_UPDATE<std::string, std::string>(prop_name, key, value);

    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_map_prop_update params error"};
    }

    return 0;
}

static int L_map_prop_erase(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        // std::string prop_name = luaL_checkstring(L, 1);
        // std::string key = luaL_checkstring(L, 2);

        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));

        // account_state->api_handler()->MAP_PROP_ERASE<std::string, std::string>(prop_name, key);

    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_map_prop_erase params error"};
    }

    return 0;
}

static int L_map_prop_clear(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        // std::string prop_name = luaL_checkstring(L, 1);

        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));

        // account_state->api_handler()->MAP_PROP_CLEAR<std::string, std::string>(prop_name);

    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_map_prop_clear params error"};
    }

    return 0;
}

static int L_map_prop_query(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        std::string prop_name = luaL_checkstring(L, 1);
        std::string key = luaL_checkstring(L, 2);

        top::contract_common::xcontract_state_t* account_state = reinterpret_cast<top::contract_common::xcontract_state_t*>(lua_getuserdata(L));
        top::contract_common::properties::xproperty_identifier_t id{prop_name, top::contract_common::properties::xproperty_type_t::map, top::contract_common::properties::xenum_property_category::user};
        // auto res = account_state->access_control()->map_prop_query<std::string, std::string>(account_state->state_account_address(), id, key);
        auto res = account_state->map_at<std::string, std::string>(id.full_name(), key);

        lua_pushstring(L, res.c_str());
    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_map_prop_query params error"};
    }

    return 1;
}


static int L_str_prop_create(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        // std::string prop_name = luaL_checkstring(L, 1);

        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));
        // account_state->api_handler()->STR_PROP_CREATE(prop_name);
    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_str_prop_create params error"};
    }

    return 0;
}

static int L_str_prop_exist(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        // std::string prop_name = luaL_checkstring(L, 1);
        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));

        // if (account_state->api_handler()->STR_PROP_EXIST(prop_name)) {
        //     lua_pushboolean(L, 1);
        // } else {
        //     lua_pushboolean(L, 0);
        // }
    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_str_prop_exist params error"};
    }

    return 1;
}

static int L_str_prop_update(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1) && lua_isstring(L, 2)) {
        // std::string prop_name = luaL_checkstring(L, 1);
        // std::string value = luaL_checkstring(L, 2);

        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));
        // account_state->api_handler()->STR_PROP_UPDATE(prop_name, value);

    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_str_prop_update params error"};
    }

    return 0;
}

static int L_str_prop_clear(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        // std::string prop_name = luaL_checkstring(L, 1);

        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));
        // account_state->api_handler()->STR_PROP_CLEAR(prop_name);

    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_str_prop_clear params error"};
    }

    return 0;
}

static int L_str_prop_query(lua_State* L) {
    lua_addinstructioncount(L, DB_OP_INSTRUCTION_COUNT);
    if (lua_isstring(L, 1)) {
        // std::string prop_name = luaL_checkstring(L, 1);

        // top::contract::xaccount_state_t* account_state = reinterpret_cast<top::contract::xaccount_state_t*>(lua_getuserdata(L));
        // auto res = account_state->api_handler()->STR_PROP_QUERY(prop_name);

        // lua_pushstring(L, res.c_str());

    } else {
        // throw xcontract_runtime_error_t{xerrc_t::enum_vm_exception, "L_str_prop_query params error"};
    }

    return 1;
}

static xlua_chain_func g_lua_chain_func[] = {
    { "require_owner_auth",     L_require_owner },
    { "exec_account",           L_exec_account },
    { "get_balance",            L_get_balance },
    { "get_pay_fee",            L_get_pay_fee },
    { "map_prop_create",        L_map_prop_create },
    { "map_prop_exist",         L_map_prop_exist },
    { "map_prop_add",           L_map_prop_add },
    { "map_prop_update",        L_map_prop_update } ,
    { "map_prop_erase",         L_map_prop_erase },
    { "map_prop_clear",         L_map_prop_clear },
    { "map_prop_query",         L_map_prop_query },
    { "str_prop_create",        L_str_prop_create },
    { "str_prop_exist",         L_str_prop_exist },
    { "str_prop_update",        L_str_prop_update },
    { "str_prop_clear",         L_str_prop_clear },
    { "str_prop_query",         L_str_prop_query },
    { "grant",                  L_grant },
    { "random_seed",            L_random_seed },
};
