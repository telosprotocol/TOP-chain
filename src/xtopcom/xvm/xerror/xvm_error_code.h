// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include "xbase/xns_macro.h"
#include "xbasic/xmodule_type.h"

#include <string>
#include <cassert>
#include <system_error>

NS_BEG2(top, xvm)

enum class enum_xvm_error_code
{
    ok = 0,

    error_base = chainbase::enum_xmodule_type::xmodule_type_xvm,
    enum_vm_code_is_exist,
    enum_vm_exec_account_error,
    enum_vm_no_func_find,
    enum_vm_exception,
    enum_vm_action_error,

    enum_lua_abi_input_error,
    enum_lua_abi_input_name_or_type_error,
    enum_lua_abi_input_type_error,

    enum_lua_code_input_type_not_support,
    enum_lua_code_owern_error,
    enum_lua_code_parse_error,
    enum_lua_code_pcall_error,

    enum_lua_api_require_owern_to_exec,
    enum_lua_api_key_value_must_be_string,
    enum_lua_api_key_must_be_string,
    enum_lua_api_range_error,
    enum_lua_api_db_error,
    enum_lua_api_param_error,
    enum_lua_exec_unkown_error,

    //system contract error code
    //vote error
    vm_vote_proposal_exist_error,
    vm_vote_option_exist_error,
    vm_vote_proposal_ruler_not_valid,
    vm_vote_voter_voted_or_proxy,
    vm_vote_update_db_error,
    vm_vote_proposal_property_error,

    query_contract_data_fail_to_get_block,
    query_contract_data_property_missing,
    query_contract_data_property_empty,

    error_max,
};


#define XVM_TO_STR(val) #val

inline std::string xvm_error_to_string(int32_t code) {
    assert(code > (int32_t)enum_xvm_error_code::error_base && code < (int32_t)enum_xvm_error_code::error_max);
    static const char* names[] = {
        XVM_TO_STR(enum_vm_code_is_exist),
        XVM_TO_STR(enum_vm_exec_account_error),
        XVM_TO_STR(enum_vm_no_func_find),
        XVM_TO_STR(enum_vm_exception),
        XVM_TO_STR(enum_vm_action_error),

        XVM_TO_STR(enum_lua_abi_input_error),
        XVM_TO_STR(enum_lua_abi_input_name_or_type_error),
        XVM_TO_STR(enum_lua_abi_input_type_error),

        XVM_TO_STR(enum_lua_code_input_type_not_support),
        XVM_TO_STR(enum_lua_code_owern_error),
        XVM_TO_STR(enum_lua_code_parse_error),
        XVM_TO_STR(enum_lua_code_pcall_error),

        XVM_TO_STR(enum_lua_api_require_owern_to_exec),
        XVM_TO_STR(enum_lua_api_key_value_must_be_string),
        XVM_TO_STR(enum_lua_api_key_must_be_string),
        XVM_TO_STR(enum_lua_api_range_error),
        XVM_TO_STR(enum_lua_api_db_error),
        XVM_TO_STR(enum_lua_api_param_error),
        XVM_TO_STR(enum_lua_exec_unkown_error),

        //system contract error code
        //vote error
        XVM_TO_STR(vm_vote_proposal_exist_error),
        XVM_TO_STR(vm_vote_option_exist_error),
        XVM_TO_STR(vm_vote_proposal_ruler_not_valid),
        XVM_TO_STR(vm_vote_voter_voted_or_proxy),
        XVM_TO_STR(vm_vote_update_db_error),
        XVM_TO_STR(vm_vote_proposal_property_error),
        XVM_TO_STR(query_contract_data_fail_to_get_block),
        XVM_TO_STR(query_contract_data_property_missing),
        XVM_TO_STR(query_contract_data_property_empty)
    };
    return names[code - (int32_t)enum_xvm_error_code::error_base - 1];
}



std::error_code make_error_code(const enum_xvm_error_code errc) noexcept;
std::error_condition make_error_condition(const enum_xvm_error_code errc) noexcept;

NS_END2

NS_BEG1(std)

template <>
struct is_error_code_enum<top::xvm::enum_xvm_error_code> : std::true_type
{
};

template <>
struct is_error_condition_enum<top::xvm::enum_xvm_error_code> : std::true_type {
};

NS_END1
