// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xerror/xerror.h"

NS_BEG3(top, contract_runtime, error)

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::invalid_vm_type:
        return "invalid vm type";

    case xerrc_t::invalid_contract_type:
        return "invalid contract type";

    case xerrc_t::invalid_transaction_type:
        return "invalid transaction type";

    case xerrc_t::invalid_transaction_subtype:
        return "invalid transaction subtype";

    case xerrc_t::contract_not_found:
        return "contract not found";

    case xerrc_t::contract_api_not_found:
        return "contract api not found";

    case xerrc_t::enum_vm_code_is_exist:
        return "contract code exist";

    case xerrc_t::enum_vm_exec_account_error:
        return "contract sender account error";

    case xerrc_t::enum_vm_no_func_find:
        return "contract API not found";

    case xerrc_t::enum_vm_exception:
        return "vm exception";

    case xerrc_t::enum_vm_action_error:
        return "vm action error";

    case xerrc_t::enum_lua_abi_input_error:
        return "LUA ABI input error";

    case xerrc_t::enum_lua_abi_input_name_or_type_error:
        return "LUA ABI input name or type error";

    default:
        return "unknown contract runtime error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), contract_runtime_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), contract_runtime_category());
}

xtop_contract_runtime_error::xtop_contract_runtime_error() : std::runtime_error{ec_.message()} {
}

xtop_contract_runtime_error::xtop_contract_runtime_error(xerrc_t const error_code) : xtop_contract_runtime_error{make_error_code(error_code)} {
}

xtop_contract_runtime_error::xtop_contract_runtime_error(xerrc_t const error_code, std::string const & message) : xtop_contract_runtime_error{make_error_code(error_code), message} {
}

xtop_contract_runtime_error::xtop_contract_runtime_error(std::error_code const & ec) : std::runtime_error{ec.message()}, ec_{ec} {
}

xtop_contract_runtime_error::xtop_contract_runtime_error(std::error_code const & ec, const std::string& message) : std::runtime_error{message}, ec_{ec} {
}

const std::error_code & xtop_contract_runtime_error::code() const noexcept {
    return ec_;
}

class xtop_contract_runtime_category final : public std::error_category {
    char const * name() const noexcept override {
        return "contract_runtime";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xcontract_runtime_category_t = xtop_contract_runtime_category;

std::error_category const & contract_runtime_category() {
    static xcontract_runtime_category_t category{};
    return category;
}

NS_END3
