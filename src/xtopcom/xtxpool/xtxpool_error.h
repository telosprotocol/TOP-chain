// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmodule_type.h"
#include "xbasic/xns_macro.h"

#include <assert.h>

#include <string>

NS_BEG2(top, xtxpool)

enum enum_xtxpool_error_type {
    xtxpool_error_base = chainbase::enum_xmodule_type::xmodule_type_xtxpool,

    xtxpool_error_tx_nonce_too_old,
    xtxpool_error_tx_nonce_repeat,
    xtxpool_error_tx_nonce_incontinuity,
    xtxpool_error_send_tx_queue_over_upper_limit,
    xtxpool_error_request_tx_repeat,
    xtxpool_error_tx_last_hash_error,
    xtxpool_error_tx_expired,
    xtxpool_error_accountobj_destroy,
    xtxpool_error_unconfirm_sendtx_exist,
    xtxpool_error_unconfirm_sendtx_not_exist,
    xtxpool_error_unconfirm_sendtx_data_not_complete,
    xtxpool_error_sendtx_receipt_duplicate,
    xtxpool_error_sendtx_receipt_expired,
    xtxpool_error_sendtx_receipt_data_not_synced,
    xtxpool_error_unconfirm_sendtx_cache_height_not_match_consensusing_height,
    xtxpool_error_unconfirm_sendtx_cache_update_fail_no_account,
    xtxpool_error_unconfirm_sendtx_cache_update_fail_no_unit,
    xtxpool_error_unconfirm_sendtx_cache_update_fail_state_behind,
    xtxpool_error_tx_miss_failed,
    xtxpool_error_account_unit_height_behind,
    xtxpool_error_account_unit_height_higher,
    xtxpool_error_account_state_behind,
    xtxpool_error_account_property_behind,
    xtxpool_error_account_usable_timestamp_not_arrive,
    xtxpool_error_transaction_not_belong_to_this_service,
    xtxpool_error_service_stoped,
    xtxpool_error_transaction_whitelist_limit,
    xtxpool_error_tx_validate_sign_error,
    xtxpool_error_tx_multi_sign_error,
    xtxpool_error_tx_invalid_type,
    xtxpool_error_send_tx_reach_upper_limit,
    xtxpool_error_receipt_invalid,

    xtxpool_error_max,
};

#define XTXPOOL_TO_STR(val) #val

inline std::string xtxpool_error_to_string(int32_t code) {
    assert(code > xtxpool_error_base && code < xtxpool_error_max);
    static const char * names[] = {XTXPOOL_TO_STR(xtxpool_error_tx_nonce_too_old),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_nonce_repeat),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_nonce_incontinuity),
                                   XTXPOOL_TO_STR(xtxpool_error_send_tx_queue_over_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_request_tx_repeat),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_last_hash_error),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_expired),
                                   XTXPOOL_TO_STR(xtxpool_error_accountobj_destroy),
                                   XTXPOOL_TO_STR(xtxpool_error_unconfirm_sendtx_exist),
                                   XTXPOOL_TO_STR(xtxpool_error_unconfirm_sendtx_not_exist),
                                   XTXPOOL_TO_STR(xtxpool_error_unconfirm_sendtx_data_not_complete),
                                   XTXPOOL_TO_STR(xtxpool_error_sendtx_receipt_duplicate),
                                   XTXPOOL_TO_STR(xtxpool_error_sendtx_receipt_expired),
                                   XTXPOOL_TO_STR(xtxpool_error_sendtx_receipt_data_not_synced),
                                   XTXPOOL_TO_STR(xtxpool_error_unconfirm_sendtx_cache_height_not_match_consensusing_height),
                                   XTXPOOL_TO_STR(xtxpool_error_unconfirm_sendtx_cache_update_fail_no_account),
                                   XTXPOOL_TO_STR(xtxpool_error_unconfirm_sendtx_cache_update_fail_no_unit),
                                   XTXPOOL_TO_STR(xtxpool_error_unconfirm_sendtx_cache_update_fail_state_behind),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_miss_failed),
                                   XTXPOOL_TO_STR(xtxpool_error_account_unit_height_behind),
                                   XTXPOOL_TO_STR(xtxpool_error_account_unit_height_higher),
                                   XTXPOOL_TO_STR(xtxpool_error_account_state_behind),
                                   XTXPOOL_TO_STR(xtxpool_error_account_property_behind),
                                   XTXPOOL_TO_STR(xtxpool_error_account_usable_timestamp_not_arrive),
                                   XTXPOOL_TO_STR(xtxpool_error_transaction_not_belong_to_this_service),
                                   XTXPOOL_TO_STR(xtxpool_error_service_stoped),
                                   XTXPOOL_TO_STR(xtxpool_error_transaction_whitelist_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_validate_sign_error),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_multi_sign_error),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_invalid_type),
                                   XTXPOOL_TO_STR(xtxpool_error_send_tx_reach_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_receipt_invalid)};
    return names[code - xtxpool_error_base - 1];
}

NS_END2
