// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <system_error>

NS_BEG3(top, evm_common, error)

enum class xenum_errc {
    ok,
    not_enough_data,
    abi_data_length_error,
    abi_decode_outofrange,
    abi_data_value_error,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const errc) noexcept;
std::error_condition make_error_condition(xerrc_t const errc) noexcept;

std::error_category const & evm_common_category();

NS_END3

NS_BEG1(std)

template <>
struct is_error_code_enum<top::evm_common::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::evm_common::error::xerrc_t> : std::true_type {};

NS_END1
