// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xerror/xerror.h"

NS_BEG3(top, evm_runtime, error)

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::precompiled_contract_erc20_mint:
        return "precompiled contract erc20 mint failed";

    case xerrc_t::precompiled_contract_erc20_burn:
        return "precompiled contract erc20 burn failed";

    case xenum_errc::bsc_unknown_ancestor:
        return "bsc: unknown ancestor";

    case xenum_errc::bsc_snapshot_not_found:
        return "bsc: snapshot not found";

    case xenum_errc::bsc_invalid_gas_limit:
        return "bsc: invalid gas limit";

    case xenum_errc::bsc_invalid_gas_used:
        return "bsc: invalid gas used";

    case xenum_errc::bsc_invalid_extra_data:
        return "bsc: invalid extra data";

    case xenum_errc::bsc_invalid_attestation:
        return "bsc: invalid attestation";

    case xenum_errc::bsc_header_missing_excess_blob_gas:
        return "bsc: header is missing excessBlobGas";

    case xenum_errc::bsc_header_missing_blob_gas_used:
        return "bsc: header is missing blobGasUsed";

    case xenum_errc::bsc_blob_gas_used_exceeds_maximum_allowance:
        return "bsc: blob gas used exceeds maximum allowance";

    case xenum_errc::bsc_blob_gas_used_not_a_multiple_of_blob_gas_per_blob:
        return "bsc: blob gas used is not a multiple of blob gas per blob";

    case xenum_errc::bsc_invalid_excess_blob_gas:
        return "bsc: invalid excess blob gas";

    default:
        return "unknown contract vm error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), evm_runtime_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), evm_runtime_category());
}

class xtop_evm_runtime_category final : public std::error_category {
    char const * name() const noexcept override {
        return "evm_runtime";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xevm_runtime_category_t = xtop_evm_runtime_category;

std::error_category const & evm_runtime_category() {
    static xevm_runtime_category_t category{};
    return category;
}

NS_END3
