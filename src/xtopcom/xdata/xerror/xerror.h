// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <system_error>

NS_BEG3(top, data, error)

enum class xenum_errc {
    ok,
    update_state_failed,
    update_state_block_height_mismatch,
    update_state_block_type_mismatch,

    property_already_exist,
    property_type_invalid,
    property_hash_mismatch,
    property_not_exist,

    binlog_instruction_type_invalid,

    election_data_start_time_invalid,

    checkpoint_not_found,

    action_address_type_error,
    invalid_toke_id,

    erc20_allowance_spender_not_found,
    erc20_allowance_not_enough,
    erc20_controller_already_set,
    erc20_owner_already_set,

    eth_server_error = -32000,

    eth_invalid_params = -32602
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const errc) noexcept;
std::error_condition make_error_condition(xerrc_t const errc) noexcept;

std::error_category const & data_category();

NS_END3

NS_BEG1(std)

template <>
struct is_error_code_enum<top::data::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::data::error::xerrc_t> : std::true_type {};

NS_END1
