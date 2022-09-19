// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstatestore/xerror.h"

#include <string>

namespace top {
namespace statestore {
namespace error {

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";
    case xerrc_t::statestore_extract_state_root_err:
        return "statestore extract state root error";
    case xerrc_t::statestore_load_tableblock_err:
        return "statestore load tableblock error";
    case xerrc_t::statestore_load_tableblock_output_err:
        return "statestore load tableblock output error";
    case xerrc_t::statestore_load_unitblock_err:
        return "statestore load unitblock error";
    case xerrc_t::statestore_block_root_unmatch_mpt_root_err:
        return "statestore mpt and block root unmatch error";
    case xerrc_t::statestore_tablestate_exec_fail:
        return "statestore tablestate exec fail";
    case xerrc_t::statestore_block_unmatch_prev_err:
        return "statestore block unmatch prev fail";
    case xerrc_t::statestore_try_limit_arrive_err:
        return "statestore try limit arrive error";
    case xerrc_t::statestore_cannot_execute_for_long_distance_err:
        return "statestore cannot execute for long distance error";

    default:
        return "unknown error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), statestore_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), statestore_category());
}

class xtop_statestore_category final : public std::error_category {
    char const * name() const noexcept override {
        return "xstatestore";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xstatestore_category_t = xtop_statestore_category;

std::error_category const & statestore_category() {
    static xstatestore_category_t category{};
    return category;
}

}  // namespace error
}  // namespace state_mpt
}  // namespace top
