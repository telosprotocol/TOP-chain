// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvledger/xerror.h"

#include <string>

namespace top {
namespace base {
namespace error {

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::block_input_output_data_not_exist:
        return "input output data not exist";
    case xerrc_t::block_input_output_create_object_fail:
        return "input output create object fail";

    default:
        return "unknown error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), base_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), base_category());
}

class xtop_base_category final : public std::error_category {
    char const * name() const noexcept override {
        return "xvledger";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xbase_category_t = xtop_base_category;

std::error_category const & base_category() {
    static xbase_category_t category{};
    return category;
}

}  // namespace error
}  // namespace base
}  // namespace top
