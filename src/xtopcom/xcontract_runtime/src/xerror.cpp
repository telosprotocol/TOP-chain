// Copyright (c) 2017-2021 Telos Foundation & contributors
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

    case xerrc_t::invalid_timer_interval:
        return "invalid timer interval";

    case xerrc_t::account_state_not_changed:
        return "account state not changed";

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

    case xerrc_t::enum_wasm_code_invalid:
        return "WASM code invlid";

    case xerrc_t::evm_vm_fatal:
        return "EVM VM Fatal";

    case xerrc_t::evm_vm_error:
        return "EVM VM Error";

    case xerrc_t::evm_incorrect_args:
        return "EVM VM Incorrect Args";

    case xerrc_t::evm_incorrect_nonce:
        return "EVM VM Incorrect Nonce";

    case xerrc_t::evm_protobuf_serilized_error:
        return "EVM protobuf error";

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
