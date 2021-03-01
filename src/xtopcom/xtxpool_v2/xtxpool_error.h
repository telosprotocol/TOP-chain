// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmodule_type.h"
#include "xbasic/xns_macro.h"

#include <assert.h>

#include <string>

NS_BEG2(top, xtxpool_v2)

enum enum_xtxpool_error_type {
    xtxpool_error_base = chainbase::enum_xmodule_type::xmodule_type_xtxpool,

    xtxpool_error_queue_reached_upper_limit,
    xtxpool_error_account_send_txs_reached_upper_limit,
    xtxpool_error_pending_reached_upper_limit,
    xtxpool_error_pending_account_reached_upper_limit,
    xtxpool_error_tx_duplicate,
    xtxpool_error_tx_nonce_incontinuity,
    xtxpool_error_tx_nonce_duplicate,
    xtxpool_error_max,
};

#define XTXPOOL_TO_STR(val) #val

inline std::string xtxpool_error_to_string(int32_t code) {
    assert(code > xtxpool_error_base && code < xtxpool_error_max);
    static const char * names[] = {XTXPOOL_TO_STR(xtxpool_error_send_tx_queue_over_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_account_send_txs_reached_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_pending_reached_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_pending_account_reached_upper_limit),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_duplicate),
                                   XTXPOOL_TO_STR(xtxpool_error_tx_nonce_incontinuity)};
    return names[code - xtxpool_error_base - 1];
}

NS_END2
