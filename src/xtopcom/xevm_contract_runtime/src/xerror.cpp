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
