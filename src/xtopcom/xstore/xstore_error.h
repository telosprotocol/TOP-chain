// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <assert.h>

#include "xbasic/xmodule_type.h"

namespace top { namespace store {

enum enum_xstore_error_type {
    xstore_success = 0,

    xstore_error_base = chainbase::enum_xmodule_type::xmodule_type_xstore,
    xaccount_balance_not_enough,
    xaccount_account_not_exist,
    xaccount_lock_less_min_deposit,
    xaccount_transfer_less_than_safe_amount,
    xaccount_unit_height_should_not_zero,
    xaccount_unit_height_should_zero,
    xreceiver_address_same_with_sender,
    xaccount_contract_address_create_repeat,
    xaccount_not_exist,
    xaccount_state_behind,
    xaccount_property_number_exceed_max,
    xaccount_property_name_length_exceed_max,
    xaccount_property_contract_count_exceed_max,
    xaccount_property_contract_code_size_invalid,
    xaccount_property_can_only_set_once,

    xaccount_property_not_create,
    xaccount_property_map_field_not_create,
    xaccount_property_behind_not_exist,
    xaccount_property_behind_not_match,



    xaccount_property_create_fail,
    xaccount_property_not_exist,
    xaccount_property_operate_fail,
    xaccount_property_cmd_invalid,
    xaccount_property_exec_exception,
    xaccount_property_lock_token_key_same,
    xaccount_property_unlock_token_key_not_exist,
    xaccount_property_unlock_token_sign_illegal,
    xaccount_property_unlock_token_time_not_reach,
    xaccount_property_set_hash_not_match,
    xaccount_property_parent_account_exist,
    xaccount_property_parent_account_not_exist,
    xaccount_property_sub_account_exist,
    xaccount_property_contract_sub_account_exist,
    xaccount_property_sub_account_overflow,
    xaccount_set_keys_key_illegal,
    xstore_check_repeat_block,
    xstore_check_block_flags_change,
    xstore_check_block_hash_error,
    xstore_check_block_prev_hash_error,
    xstore_check_block_horizontal_consistency_invalid,
    xstore_check_block_not_exist,
    xstore_check_not_lightunit,
    xstore_check_transaction_hash_error,
    xstore_check_transaction_already_exist,
    xstore_check_transaction_not_include_in_unit,
    xstore_check_property_log_not_include_in_unit,
    xstore_check_property_log_future_binlog_error,
    xstore_check_property_log_hash_error,
    xstore_check_property_hash_error,
    xstore_create_new_block_fail,
    xstore_blockchain_state_behind,
    xaccount_contract_number_exceed_max,
    xstore_user_contract_can_not_initiate_transaction,
    xstore_user_contract_should_not_do_native_property,
    xaccount_property_confirm_height_conflict,
    xaccount_property_mortgage_invalid,
    xaccount_property_log_not_exist,
    xstore_check_block_verify_fail,
    xstore_check_block_verify_lack_auth_data,
    xtransaction_not_enough_pledge_token_tgas,
    xtransaction_not_enough_pledge_token_disk,
    xtransaction_not_enough_token,
    xtransaction_non_positive_pledge_token,
    xtransaction_pledge_too_much_token,
    xtransaction_contract_not_enough_tgas,
    xtransaction_pledge_redeem_vote_err,
    xtransaction_param_invalid,
    xaccount_property_already_exist,
    xstore_error_max,
};

#define XSTORE_TO_STR(val) #val

inline std::string xstore_error_to_string(int32_t code) {
    assert(code > xstore_error_base && code < xstore_error_max);
    static const char* names[] = {
        XSTORE_TO_STR(xaccount_balance_not_enough),
        XSTORE_TO_STR(xaccount_account_not_exist),
        XSTORE_TO_STR(xaccount_lock_less_min_deposit),
        XSTORE_TO_STR(xaccount_transfer_less_than_safe_amount),
        XSTORE_TO_STR(xaccount_unit_height_should_not_zero),
        XSTORE_TO_STR(xaccount_unit_height_should_zero),
        XSTORE_TO_STR(xreceiver_address_same_with_sender),
        XSTORE_TO_STR(xaccount_contract_address_create_repeat),
        XSTORE_TO_STR(xaccount_not_exist),
        XSTORE_TO_STR(xaccount_state_behind),
        XSTORE_TO_STR(xaccount_property_number_exceed_max),
        XSTORE_TO_STR(xaccount_property_name_length_exceed_max),
        XSTORE_TO_STR(xaccount_property_contract_count_exceed_max),
        XSTORE_TO_STR(xaccount_property_contract_code_size_invalid),
        XSTORE_TO_STR(xaccount_property_can_only_set_once),

        XSTORE_TO_STR(xaccount_property_not_create),
        XSTORE_TO_STR(xaccount_property_map_field_not_create),
        XSTORE_TO_STR(xaccount_property_behind_not_exist),
        XSTORE_TO_STR(xaccount_property_behind_not_match),



        XSTORE_TO_STR(xaccount_property_create_fail),
        XSTORE_TO_STR(xaccount_property_not_exist),
        XSTORE_TO_STR(xaccount_property_operate_fail),
        XSTORE_TO_STR(xaccount_property_cmd_invalid),
        XSTORE_TO_STR(xaccount_property_exec_exception),
        XSTORE_TO_STR(xaccount_property_lock_token_key_same),
        XSTORE_TO_STR(xaccount_property_unlock_token_key_not_exist),
        XSTORE_TO_STR(xaccount_property_unlock_token_sign_illegal),
        XSTORE_TO_STR(xaccount_property_unlock_token_time_not_reach),
        XSTORE_TO_STR(xaccount_property_set_hash_not_match),
        XSTORE_TO_STR(xaccount_property_parent_account_exist),
        XSTORE_TO_STR(xaccount_property_parent_account_not_exist),
        XSTORE_TO_STR(xaccount_property_sub_account_exist),
        XSTORE_TO_STR(xaccount_property_contract_sub_account_exist),
        XSTORE_TO_STR(xaccount_property_sub_account_overflow),
        XSTORE_TO_STR(xaccount_set_keys_key_illegal),
        XSTORE_TO_STR(xstore_check_repeat_block),
        XSTORE_TO_STR(xstore_check_block_flags_change),
        XSTORE_TO_STR(xstore_check_block_hash_error),
        XSTORE_TO_STR(xstore_check_block_prev_hash_error),
        XSTORE_TO_STR(xstore_check_block_horizontal_consistency_invalid),
        XSTORE_TO_STR(xstore_check_block_not_exist),
        XSTORE_TO_STR(xstore_check_not_lightunit),
        XSTORE_TO_STR(xstore_check_transaction_hash_error),
        XSTORE_TO_STR(xstore_check_transaction_already_exist),
        XSTORE_TO_STR(xstore_check_transaction_not_include_in_unit),
        XSTORE_TO_STR(xstore_check_property_log_not_include_in_unit),
        XSTORE_TO_STR(xstore_check_property_log_hash_error),
        XSTORE_TO_STR(xstore_check_property_log_future_binlog_error),
        XSTORE_TO_STR(xstore_check_property_hash_error),
        XSTORE_TO_STR(xstore_create_new_block_fail),
        XSTORE_TO_STR(xstore_blockchain_state_behind),
        XSTORE_TO_STR(xaccount_contract_number_exceed_max),
        XSTORE_TO_STR(xstore_user_contract_can_not_initiate_transaction),
        XSTORE_TO_STR(xstore_user_contract_should_not_do_native_property),
        XSTORE_TO_STR(xaccount_property_confirm_height_conflict),
        XSTORE_TO_STR(xaccount_property_mortgage_invalid),
        XSTORE_TO_STR(xaccount_property_log_not_exist),
        XSTORE_TO_STR(xstore_check_block_verify_fail),
        XSTORE_TO_STR(xstore_check_block_verify_lack_auth_data),
        XSTORE_TO_STR(xtransaction_not_enough_pledge_token_tgas),
        XSTORE_TO_STR(xtransaction_not_enough_pledge_token_disk),
        XSTORE_TO_STR(xtransaction_not_enough_token),
        XSTORE_TO_STR(xtransaction_non_positive_pledge_token),
        XSTORE_TO_STR(xtransaction_pledge_too_much_token),
        XSTORE_TO_STR(xtransaction_contract_not_enough_tgas),
        XSTORE_TO_STR(xtransaction_pledge_redeem_vote_err),
        XSTORE_TO_STR(xtransaction_param_invalid),
        XSTORE_TO_STR(xaccount_property_already_exist),
    };
    return names[code - xstore_error_base - 1];
}

}  // namespace store
}  // namespace top
