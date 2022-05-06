// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/xerror/xerror.h"

#include <string>

NS_BEG3(top, evm_common, error)

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::not_enough_data:
        return "not enough data";

    case xerrc_t::abi_data_length_error:
        return "invalid abi data length";

    case xerrc_t::abi_decode_outofrange:
        return "abi decode out of data range";

    case xerrc_t::abi_data_value_error:
        return "abi decode value error";

    default:
        return "unknown evm common error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), evm_common_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), evm_common_category());
}

class xtop_evm_common_category final : public std::error_category {
    char const * name() const noexcept override {
        return "evm_common";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xevm_common_category_t = xtop_evm_common_category;

std::error_category const & evm_common_category() {
    static xevm_common_category_t category{};
    return category;
}

NS_END3
