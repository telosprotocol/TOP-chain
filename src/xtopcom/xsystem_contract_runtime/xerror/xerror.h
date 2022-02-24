// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <stdexcept>
#include <string>
#include <system_error>

NS_BEG4(top, contract_runtime, system, error)

enum class xenum_errc {
    ok = 0,

    unknown_error,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const ec) noexcept;
std::error_condition make_error_condition(xerrc_t const ec) noexcept;

std::error_category const & system_contract_runtime_category();

NS_END4

NS_BEG1(std)

template <>
struct is_error_code_enum<top::contract_runtime::system::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::contract_runtime::system::error::xerrc_t> : std::true_type {};

NS_END1
