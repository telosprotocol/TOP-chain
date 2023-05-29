// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <system_error>

NS_BEG3(top, common, error)

enum class xenum_errc {
    ok,
    invalid_rotation_status,
    invalid_account_address,
    invalid_account_base_address,
    invalid_account_index,
    invalid_table_id,
    invalid_ledger_id,
    invalid_account_type,
    invalid_zone_index,
    invalid_table_type,
    token_not_used,
    token_symbol_not_matched,
    token_not_predefined,
    invalid_rlp_stream,
    empty_token_symbol,
    token_symbol_unknown,
    invalid_block,
    invalid_db_load,
    invalid_eth_tx,
    invalid_eth_header,
    invalid_bloom,
    table_id_mismatch,
    table_base_address_is_empty,
    invalid_table_base_address,
    invalid_table_address,
    not_enough_data,
    invalid_eth_address,
    rlp_input_empty,
    rlp_invalid_encoded_data,
    rlp_invalid_size_of_length_field,
    rlp_not_enough_data,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t errc) noexcept;
std::error_condition make_error_condition(xerrc_t errc) noexcept;

std::error_category const & common_category() noexcept;

NS_END3

NS_BEG1(std)

#if !defined(XCXX14)

template <>
struct hash<top::common::error::xerrc_t> final {
    size_t operator()(top::common::error::xerrc_t const errc) const noexcept;
};

#endif

template <>
struct is_error_code_enum<top::common::error::xerrc_t> : true_type {};

template <>
struct is_error_condition_enum<top::common::error::xerrc_t> : true_type {};

NS_END1
