// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cassert>
#include <string>

#include "xbasic/xmodule_type.h"

NS_BEG2(top, xverifier)

/**
 * @brief error type definition for xverifier module
 *
 */
enum enum_xverifier_error_type: std::int32_t {
    xverifier_success = 0,
    xverifier_error_base = chainbase::enum_xmodule_type::xmodule_type_xverifier,

    xverifier_error_addr_invalid,
    xverifier_error_src_dst_addr_same,
    xverifier_error_priv_pub_not_match,
    xverifier_error_tx_signature_invalid,
    xverifier_error_tx_hash_invalid,
    xverifier_error_tx_fire_expired,
    xverifier_error_tx_duration_expired,
    xverifier_error_block_hash_invalid,
    xverifier_error_account_min_deposit_invalid,
    xverifier_error_tx_min_deposit_invalid,
    xverifier_error_transfer_tx_min_amount_invalid,
    xverifier_error_trnsafer_tx_src_dst_amount_not_same,
    xverifier_error_whitelist_limit,
    xverifier_error_contract_not_allowed,
    xverifier_error_transfer_tx_amount_over_max,
    xverifier_error_sendtx_by_normal_contrwact,
    xverifier_error_tx_param_invalid,
    xverifier_error_send_tx_source_invalid,
    xverifier_error_tx_basic_validation_invalid,
    xverifier_error_tx_whitelist_invalid,
    xverifier_error_local_tx_invalid,
    xverifier_error_burn_tx_invalid,
    xverifier_error_tx_blacklist_invalid,

    xverifier_error_max
};

using xverifier_error = enum_xverifier_error_type;

#define XVERIFIER_ERROR_TO_STR(val) #val

/**
 * @brief  convert error code to string
 *
 * @param code  the code to convert
 * @return std::string  the string format of the code
 */
inline std::string xverifier_error_to_string(int32_t code) {
    assert(code > xverifier_error_base && code < xverifier_error_max);
    static const char* names[] = {
        XVERIFIER_ERROR_TO_STR(xverifier_error_addr_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_src_dst_addr_same),
        XVERIFIER_ERROR_TO_STR(xverifier_error_priv_pub_not_match),
        XVERIFIER_ERROR_TO_STR(xverifier_error_tx_signature_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_tx_hash_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_tx_fire_expired),
        XVERIFIER_ERROR_TO_STR(xverifier_error_tx_duration_expired),
        XVERIFIER_ERROR_TO_STR(xverifier_error_block_hash_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_account_min_deposit_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_tx_min_deposit_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_transfer_tx_min_amount_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_trnsafer_tx_src_dst_amount_not_same),
        XVERIFIER_ERROR_TO_STR(xverifier_error_whitelist_limit),
        XVERIFIER_ERROR_TO_STR(xverifier_error_contract_not_allowed),
        XVERIFIER_ERROR_TO_STR(xverifier_error_transfer_tx_amount_over_max),
        XVERIFIER_ERROR_TO_STR(xverifier_error_sendtx_by_normal_contrwact),
        XVERIFIER_ERROR_TO_STR(xverifier_error_tx_param_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_send_tx_source_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_tx_basic_validation_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_tx_whitelist_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_local_tx_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_burn_tx_invalid),
        XVERIFIER_ERROR_TO_STR(xverifier_error_tx_blacklist_invalid),
    };
    return names[code - xverifier_error_base - 1];
}



NS_END2
