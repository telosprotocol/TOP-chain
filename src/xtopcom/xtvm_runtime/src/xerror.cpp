// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtvm_runtime/xerror.h"

NS_BEG3(top, tvm, error)

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::protobuf_serilized_error:
        return "protobuf error";

    default:
        return "unknown tvm runtime error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), tvm_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), tvm_category());
}

class xtop_tvm_category final : public std::error_category {
    char const * name() const noexcept override {
        return "tvm";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xtvm_category_t = xtop_tvm_category;

std::error_category const & tvm_category() {
    static xtvm_category_t category{};
    return category;
}

NS_END3
