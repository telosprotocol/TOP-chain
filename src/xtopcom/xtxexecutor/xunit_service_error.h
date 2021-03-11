// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <assert.h>

#include "xbasic/xns_macro.h"
#include "xbasic/xmodule_type.h"

NS_BEG2(top, txexecutor)

enum enum_xunit_service_error_type {
    xconsensus_service_error_base = chainbase::enum_xmodule_type::xmodule_type_xtxexecutor,

    xconsensus_service_error_balance_not_enough,
    xconsensus_service_error_min_deposit_error,
    xconsensus_service_error_action_not_valid,
    xconsensus_service_error_addr_type_error,
    xconsensus_service_error_sign_error,
    xtransaction_parse_type_invalid,
    xchain_error_action_parse_exception,
    xunit_contract_exec_no_property_change,

    xtransaction_pledge_token_overflow,
    xtransaction_not_enough_pledge_token_tgas,
    xtransaction_not_enough_pledge_token_disk,
    xtransaction_over_half_total_pledge_token,
    xtransaction_not_enough_deposit,
    xtransaction_too_much_deposit,
    xtransaction_not_enough_token,
    xtransaction_non_positive_pledge_token,
    xtransaction_non_top_token_illegal,
    xtransaction_non_zero_frozen_tgas,
    xtransaction_pledge_too_much_token,
    xtransaction_contract_pledge_token_illegal,
    xtransaction_contract_pledge_token_target_illegal,
    xtransaction_contract_pledge_token_source_target_mismatch,
    xtransaction_contract_not_enough_tgas,
    xtransaction_pledge_token_source_target_type_mismatch,
    xtransaction_pledge_contract_type_illegal,
    xtransaction_pledge_contract_owner_sign_err,
    xtransaction_pledge_redeem_vote_err,

    enum_xtxexecutor_error_proposal_unit_not_match_last_unit,
    enum_xtxexecutor_error_proposal_unit_should_be_fullunit,
    enum_xtxexecutor_error_proposal_unit_should_be_lightunit,
    enum_xtxexecutor_error_proposal_unit_not_match_leader_output,
    enum_xtxexecutor_error_proposal_unit_input_invalid,
    enum_xtxexecutor_error_unit_height_less_than_leader,
    enum_xtxexecutor_error_unit_height_larger_than_leader,
    enum_xtxexecutor_error_unit_leader_hash_not_equal_backup,
    enum_xtxexecutor_error_unit_height_not_equal_blockchain_height,
    enum_xtxexecutor_error_unit_state_behind,
    enum_xtxexecutor_error_unit_property_behind,
    enum_xtxexecutor_error_unit_usable_timestamp_not_arrive,
    enum_xtxexecutor_error_proposal_tableblock_should_not_empty_block,
    enum_xtxexecutor_error_leader_tableblock_behind,
    enum_xtxexecutor_error_backup_tableblock_behind,
    enum_xtxexecutor_error_account_have_no_tx,
    enum_xtxexecutor_error_txpool_empty,
    enum_xtxexecutor_error_tx_nonce_not_match,
    enum_xtxexecutor_error_account_hehind_block,
    enum_xtxexecutor_error_account_cannot_make_unit,
    enum_xtxexecutor_error_account_cannot_make_table,

    enum_xtxexecutor_error_backup_verify_fail_table_input_hash,
    enum_xtxexecutor_error_backup_verify_fail_table_output_hash,
    enum_xtxexecutor_error_backup_verify_fail_table_header_hash,
    enum_xtxexecutor_error_backup_verify_fail_check_consensus_para,
    enum_xtxexecutor_error_backup_verify_fail_block_class,
    enum_xtxexecutor_error_backup_verify_fail_block_not_match_local,

    xconsensus_service_error_max,
};

#define XUNIT_TO_STR(val) #val

inline std::string xunit_error_to_string(int32_t code) {
    assert(code > xconsensus_service_error_base && code < xconsensus_service_error_max);
    static const char* names[] = {
        XUNIT_TO_STR(xconsensus_service_error_balance_not_enough),
        XUNIT_TO_STR(xconsensus_service_error_min_deposit_error),
        XUNIT_TO_STR(xconsensus_service_error_action_not_valid),
        XUNIT_TO_STR(xconsensus_service_error_addr_type_error),
        XUNIT_TO_STR(xconsensus_service_error_sign_error),
        XUNIT_TO_STR(xtransaction_parse_type_invalid),
        XUNIT_TO_STR(xchain_error_action_parse_exception),
        XUNIT_TO_STR(xunit_contract_exec_no_property_change),
        XUNIT_TO_STR(xtransaction_pledge_token_overflow),
        XUNIT_TO_STR(xtransaction_not_enough_pledge_token_tgas),
        XUNIT_TO_STR(xtransaction_not_enough_pledge_token_disk),
        XUNIT_TO_STR(xtransaction_over_half_total_pledge_token),
        XUNIT_TO_STR(xtransaction_not_enough_deposit),
        XUNIT_TO_STR(xtransaction_too_much_deposit),
        XUNIT_TO_STR(xtransaction_not_enough_token),
        XUNIT_TO_STR(xtransaction_non_positive_pledge_token),
        XUNIT_TO_STR(xtransaction_non_top_token_illegal),
        XUNIT_TO_STR(xtransaction_non_zero_frozen_tgas),
        XUNIT_TO_STR(xtransaction_pledge_too_much_token),
        XUNIT_TO_STR(xtransaction_contract_pledge_token_illegal),
        XUNIT_TO_STR(xtransaction_contract_pledge_token_target_illegal),
        XUNIT_TO_STR(xtransaction_contract_pledge_token_source_target_mismatch),
        XUNIT_TO_STR(xtransaction_contract_not_enough_tgas),
        XUNIT_TO_STR(xtransaction_pledge_token_source_target_type_mismatch),
        XUNIT_TO_STR(xtransaction_pledge_contract_type_illegal),
        XUNIT_TO_STR(xtransaction_pledge_contract_owner_sign_err),
        XUNIT_TO_STR(xtransaction_pledge_redeem_vote_err),

        XUNIT_TO_STR(enum_xtxexecutor_error_proposal_unit_not_match_last_unit),
        XUNIT_TO_STR(enum_xtxexecutor_error_proposal_unit_should_be_fullunit),
        XUNIT_TO_STR(enum_xtxexecutor_error_proposal_unit_should_be_lightunit),
        XUNIT_TO_STR(enum_xtxexecutor_error_proposal_unit_not_match_leader_output),
        XUNIT_TO_STR(enum_xtxexecutor_error_proposal_unit_input_invalid),
        XUNIT_TO_STR(enum_xtxexecutor_error_unit_height_less_than_leader),
        XUNIT_TO_STR(enum_xtxexecutor_error_unit_height_larger_than_leader),
        XUNIT_TO_STR(enum_xtxexecutor_error_unit_leader_hash_not_equal_backup),
        XUNIT_TO_STR(enum_xtxexecutor_error_unit_height_not_equal_blockchain_height),
        XUNIT_TO_STR(enum_xtxexecutor_error_unit_state_behind),
        XUNIT_TO_STR(enum_xtxexecutor_error_unit_property_behind),
        XUNIT_TO_STR(enum_xtxexecutor_error_unit_usable_timestamp_not_arrive),
        XUNIT_TO_STR(enum_xtxexecutor_error_proposal_tableblock_should_not_empty_block),
        XUNIT_TO_STR(enum_xtxexecutor_error_leader_tableblock_behind),
        XUNIT_TO_STR(enum_xtxexecutor_error_backup_tableblock_behind),
        XUNIT_TO_STR(enum_xtxexecutor_error_account_have_no_tx),
        XUNIT_TO_STR(enum_xtxexecutor_error_txpool_empty),
        XUNIT_TO_STR(enum_xtxexecutor_error_tx_nonce_not_match),
        XUNIT_TO_STR(enum_xtxexecutor_error_account_hehind_block),
        XUNIT_TO_STR(enum_xtxexecutor_error_account_cannot_make_unit),
        XUNIT_TO_STR(enum_xtxexecutor_error_account_cannot_make_table),

        XUNIT_TO_STR(enum_xtxexecutor_error_backup_verify_fail_table_input_hash),
        XUNIT_TO_STR(enum_xtxexecutor_error_backup_verify_fail_table_output_hash),
        XUNIT_TO_STR(enum_xtxexecutor_error_backup_verify_fail_table_header_hash),
        XUNIT_TO_STR(enum_xtxexecutor_error_backup_verify_fail_check_consensus_para),
        XUNIT_TO_STR(enum_xtxexecutor_error_backup_verify_fail_block_class),
        XUNIT_TO_STR(enum_xtxexecutor_error_backup_verify_fail_block_not_match_local)
    };
    return names[code - xconsensus_service_error_base - 1];
}



NS_END2
