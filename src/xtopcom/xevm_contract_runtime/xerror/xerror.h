// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <functional>
#include <system_error>

NS_BEG3(top, evm_runtime, error)

enum class xenum_errc {
    ok = 0,

    precompiled_contract_erc20_mint,
    precompiled_contract_erc20_burn,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const ec) noexcept;
std::error_condition make_error_condition(xerrc_t const ec) noexcept;

std::error_category const & evm_runtime_category();

NS_END3

NS_BEG1(std)

template <>
struct is_error_code_enum<top::evm_runtime::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::evm_runtime::error::xerrc_t> : std::true_type {};

NS_END1
