// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsystem_contract_runtime/xerror/xerror.h"

NS_BEG4(top, contract_runtime, system, error)

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    default:
        return "unknown contract runtime error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), system_contract_runtime_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), system_contract_runtime_category());
}

class xtop_system_contract_runtime_category final : public std::error_category {
    char const * name() const noexcept override {
        return "system_contract_runtime";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xsystem_contract_runtime_category_t = xtop_system_contract_runtime_category;

std::error_category const & system_contract_runtime_category() {
    static xsystem_contract_runtime_category_t category{};
    return category;
}

NS_END4
