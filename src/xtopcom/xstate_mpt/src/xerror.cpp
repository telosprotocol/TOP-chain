// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xerror.h"

#include <string>

namespace top {
namespace state_mpt {
namespace error {

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::state_mpt_db_get_error:
        return "state mpt db get error";

    case xerrc_t::state_mpt_db_set_error:
        return "state mpt db set error";

    case xerrc_t::state_mpt_db_delete_error:
        return "state mpt db delete error";

    case xerrc_t::state_mpt_db_not_found:
        return "state mpt db not found";

    case xerrc_t::state_mpt_leaf_empty:
        return "state mpt leaf empty";

    case xerrc_t::state_mpt_unit_hash_mismatch:
        return "state mpt unit hash mismatch";

    default:
        return "unknown error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), state_mpt_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), state_mpt_category());
}

class xtop_state_mpt_category final : public std::error_category {
    char const * name() const noexcept override {
        return "state_mpt";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xstate_mpt_category_t = xtop_state_mpt_category;

std::error_category const & state_mpt_category() {
    static xstate_mpt_category_t category{};
    return category;
}

}  // namespace error
}  // namespace state_mpt
}  // namespace top
