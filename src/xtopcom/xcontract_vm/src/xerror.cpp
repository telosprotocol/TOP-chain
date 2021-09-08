// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_vm/xerror/xerror.h"

NS_BEG3(top, contract_vm, error)

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::invalid_contract_type:
        return "invalid contract type";

    case xerrc_t::transaction_execution_abort:
        return "transaction execution aborted";

    default:
        return "unknown contract vm error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), contract_vm_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), contract_vm_category());
}

class xtop_contract_vm_category final : public std::error_category {
    char const * name() const noexcept override {
        return "contract_vm";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xcontract_vm_category_t = xtop_contract_vm_category;

std::error_category const & contract_vm_category() {
    static xcontract_vm_category_t category{};
    return category;
}

NS_END3
