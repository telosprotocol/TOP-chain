// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <assert.h>
#include <string>

#include "xbasic/xmodule_type.h"

namespace top { namespace data {

enum enum_xdata_error_type {
    xdata_error_base = chainbase::enum_xmodule_type::xmodule_type_xdata,
    xdata_error_native_property_not_exist,
    xdata_error_native_property_value_not_exist,

    xchain_error_action_param_zero,
    xchain_error_action_param_not_empty,
    xchain_error_action_param_empty,
    xchain_error_action_param_memo_too_long,
    xchain_error_action_param_code_length_too_long,
    xchain_error_action_param_size_too_large,
    xchain_error_action_param_invalid,
    xchain_error_action_type_invalid,
    xchain_error_action_action_name_invalid,
    xchain_error_lock_token_pubkey_size_beyond_limit,
    xchain_error_lock_token_time_param_invalid,
    xchain_error_set_account_key_len_invalid,
    xchain_error_unlock_keys_num_invalid,
    xchain_error_unlock_keys_len_invalid,

    xdata_error_add_block_repeat,
    xdata_error_add_block_prev_hash_not_match,
    xdata_error_add_block_invalid,

    xreceipt_check_no_block_header,
    xreceipt_check_transaction_hash_check_fail,
    xreceipt_check_transaction_hash_not_match,
    xreceipt_check_transaction_hash_not_in_root,
    xreceipt_check_block_hash_check_fail,
    xreceipt_check_invalid_receipt,

    xblock_check_header_hash_check_fail,
    xblock_check_body_hash_check_fail,
    xblock_check_body_hash_not_match_in_block,

    xaccount_cmd_property_has_already_delete,
    xaccount_cmd_property_not_create,
    xaccount_cmd_property_set_value_same,
    xaccount_cmd_property_operate_fail,
    xaccount_cmd_property_map_field_not_create,
    xaccount_cmd_property_operate_type_unmatch,
    xaccount_cmd_property_has_already_create,

    xdata_error_max,
};


#define XDATA_TO_STR(val) #val

inline std::string xdata_error_to_string(int32_t code) {
    assert(code > xdata_error_base && code < xdata_error_max);
    static const char* names[] = {
        XDATA_TO_STR(xdata_error_native_property_not_exist),
        XDATA_TO_STR(xdata_error_native_property_value_not_exist),

        XDATA_TO_STR(xchain_error_action_param_zero),
        XDATA_TO_STR(xchain_error_action_param_not_empty),
        XDATA_TO_STR(xchain_error_action_param_empty),
        XDATA_TO_STR(xchain_error_action_param_memo_too_long),
        XDATA_TO_STR(xchain_error_action_param_code_length_too_long),
        XDATA_TO_STR(xchain_error_action_param_size_too_large),
        XDATA_TO_STR(xchain_error_action_type_invalid),
        XDATA_TO_STR(xchain_error_action_action_name_invalid),
        XDATA_TO_STR(xchain_error_lock_token_pubkey_size_beyond_limit),
        XDATA_TO_STR(xchain_error_lock_token_time_param_invalid),
        XDATA_TO_STR(xchain_error_set_account_key_len_invalid),
        XDATA_TO_STR(xchain_error_unlock_keys_num_invalid),

        XDATA_TO_STR(xdata_error_add_block_repeat),
        XDATA_TO_STR(xdata_error_add_block_prev_hash_not_match),
        XDATA_TO_STR(xdata_error_add_block_invalid),

        XDATA_TO_STR(xreceipt_check_no_block_header),
        XDATA_TO_STR(xreceipt_check_transaction_hash_check_fail),
        XDATA_TO_STR(xreceipt_check_transaction_hash_not_match),
        XDATA_TO_STR(xreceipt_check_transaction_hash_not_in_root),
        XDATA_TO_STR(xreceipt_check_block_hash_check_fail),
        XDATA_TO_STR(xreceipt_check_invalid_receipt),

        XDATA_TO_STR(xblock_check_header_hash_check_fail),
        XDATA_TO_STR(xblock_check_body_hash_check_fail),
        XDATA_TO_STR(xblock_check_body_hash_not_match_in_block),

        XDATA_TO_STR(xaccount_cmd_property_has_already_delete),
        XDATA_TO_STR(xaccount_cmd_property_not_create),
        XDATA_TO_STR(xaccount_cmd_property_set_value_same),
        XDATA_TO_STR(xaccount_cmd_property_operate_fail),
        XDATA_TO_STR(xaccount_cmd_property_map_field_not_create),
        XDATA_TO_STR(xaccount_cmd_property_operate_type_unmatch),
        XDATA_TO_STR(xaccount_cmd_property_has_already_create),
    };
    return names[code - xdata_error_base - 1];
}



}  // namespace data
}  // namespace top
