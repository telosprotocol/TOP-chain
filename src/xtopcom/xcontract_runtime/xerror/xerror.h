// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <stdexcept>
#include <string>
#include <system_error>

NS_BEG3(top, contract_runtime, error)

enum class xenum_errc : uint32_t {
    ok = 0,

    invalid_vm_type,
    invalid_contract_type,
    invalid_transaction_type,
    invalid_transaction_subtype,
    transaction_execution_abort,
    contract_not_found,
    contract_api_not_found,
    invalid_timer_interval,
    account_state_not_changed,

    enum_vm_api_exception,
    enum_vm_code_is_exist,
    enum_vm_exec_account_error,
    enum_vm_no_func_find,
    enum_vm_exception,
    enum_vm_action_error,
    enum_vm_not_correct_action_stage_error,

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

    // system contract error code
    // vote error
    vm_vote_proposal_exist_error,
    vm_vote_option_exist_error,
    vm_vote_proposal_ruler_not_valid,
    vm_vote_voter_voted_or_proxy,
    vm_vote_update_db_error,
    vm_vote_proposal_property_error,

    // wasm user contract
    enum_wasm_code_invalid,
    enum_bin_code_not_changed,

    /// eth contract
    /// here is regular error code exit from rust evm interface 
    /// should be defined in same order as `evm_engine_rs/engine/src/error.rs` `enum EngineErrorKind`
    /// use `as_error_code()` to convert into u32 than pass to C and add `evm_vm_ec_begin`
    evm_vm_ec_begin,
    evm_vm_error,
    evm_vm_fatal,
    evm_incorrect_args,
    evm_incorrect_nonce,
    evm_protobuf_serilized_error,

    unknown_error,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const ec) noexcept;
std::error_condition make_error_condition(xerrc_t const ec) noexcept;

std::error_category const & contract_runtime_category();

NS_END3

NS_BEG1(std)

template <>
struct is_error_code_enum<top::contract_runtime::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::contract_runtime::error::xerrc_t> : std::true_type {};

NS_END1
