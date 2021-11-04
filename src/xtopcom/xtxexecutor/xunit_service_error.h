// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <assert.h>

#include "xbase/xns_macro.h"
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
    xtransaction_confirm_state_unchange,

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

    enum_xtxexecutor_error_tx_nonce_not_match,

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
        XUNIT_TO_STR(xtransaction_confirm_state_unchange),

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

        XUNIT_TO_STR(enum_xtxexecutor_error_tx_nonce_not_match),
    };
    return names[code - xconsensus_service_error_base - 1];
}



NS_END2
