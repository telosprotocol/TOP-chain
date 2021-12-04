// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmodule_type.h"

#include <assert.h>

#include <string>

NS_BEG2(top, xtxpool_v2)

enum enum_xtxpool_error_type {
    xtxpool_success = 0,
    xtxpool_error_base = chainbase::enum_xmodule_type::xmodule_type_xtxpool,
    xtxpool_error_table_reached_upper_limit,
    xtxpool_error_role_reached_upper_limit,
    xtxpool_error_request_tx_repeat,
    xtxpool_error_account_unconfirm_txs_reached_upper_limit,
    xtxpool_error_pending_reached_upper_limit,
    xtxpool_error_pending_account_reached_upper_limit,
    xtxpool_error_tx_duplicate,
    xtxpool_error_tx_nonce_expired,
    xtxpool_error_tx_nonce_out_of_scope,
    xtxpool_error_tx_nonce_uncontinuous,
    xtxpool_error_tx_last_hash_not_match,
    xtxpool_error_tx_nonce_duplicate,
    xtxpool_error_unitblock_lack,
    xtxpool_error_transaction_not_belong_to_this_service,
    xtxpool_error_receipt_invalid,
    xtxpool_error_tx_multi_sign_error,
    xtxpool_error_tx_invalid_type,
    xtxpool_error_account_not_in_charge,
    xtxpool_error_account_state_fall_behind,
    xtxpool_error_service_not_running,
    xtxpool_error_max,
};

#define XTXPOOL_TO_STR(val) #val

inline std::string xtxpool_error_to_string(int32_t code) {
    assert(code > xtxpool_error_base && code < xtxpool_error_max);
    static const char * names[] = {XTXPOOL_TO_STR(xtxpool_error_table_reached_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_role_reached_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_request_tx_repeat),
                                   XTXPOOL_TO_STR(xtxpool_error_account_unconfirm_txs_reached_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_pending_reached_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_pending_account_reached_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_duplicate),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_nonce_expired),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_nonce_out_of_scope),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_nonce_uncontinuous),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_last_hash_not_match),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_nonce_duplicate),
                                   XTXPOOL_TO_STR(xtxpool_error_unitblock_lack),
                                   XTXPOOL_TO_STR(xtxpool_error_transaction_not_belong_to_this_service),
                                   XTXPOOL_TO_STR(xtxpool_error_receipt_invalid),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_multi_sign_error),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_invalid_type),
                                   XTXPOOL_TO_STR(xtxpool_error_account_not_in_charge),
                                   XTXPOOL_TO_STR(xtxpool_error_account_state_fall_behind),
                                   XTXPOOL_TO_STR(xtxpool_error_service_not_running)};

    return names[code - xtxpool_error_base - 1];
}

NS_END2
