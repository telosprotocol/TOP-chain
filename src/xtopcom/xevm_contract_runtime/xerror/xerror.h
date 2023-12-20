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

    bsc_unknown_ancestor,
    bsc_snapshot_not_found,
    bsc_invalid_gas_limit,
    bsc_invalid_gas_used,
    bsc_invalid_extra_data,
    bsc_invalid_attestation,
    bsc_header_missing_excess_blob_gas,
    bsc_header_missing_blob_gas_used,
    bsc_blob_gas_used_exceeds_maximum_allowance,
    bsc_blob_gas_used_not_a_multiple_of_blob_gas_per_blob,
    bsc_invalid_excess_blob_gas,
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
