// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <stdexcept>
#include <string>
#include <system_error>

NS_BEG3(top, contract_vm, error)

enum class xenum_errc {
    ok = 0,

    invalid_contract_type,
    transaction_execution_abort,
    none_success_tx,
    confirm_tx_num_error,

    unknown_error,
};
using xerrc_t = xenum_errc;

std::error_code make_error_code(xerrc_t const ec) noexcept;
std::error_condition make_error_condition(xerrc_t const ec) noexcept;

std::error_category const & contract_vm_category();

NS_END3

NS_BEG1(std)

template <>
struct is_error_code_enum<top::contract_vm::error::xerrc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::contract_vm::error::xerrc_t> : std::true_type {};

NS_END1
