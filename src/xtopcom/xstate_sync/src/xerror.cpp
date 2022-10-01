// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_sync/xerror.h"

#include <string>

namespace top {
namespace state_sync {
namespace error {

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::state_sync_cancel:
        return "state data download canceled";

    case xerrc_t::state_sync_overtime:
        return "state sync overtime";

    case xerrc_t::state_network_invalid:
        return "state network invalid";

    case xerrc_t::state_data_invalid:
        return "state data invalid";        

    case xerrc_t::downloader_is_running:
        return "downloader is running";

    default:
        return "unknown error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), state_sync_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), state_sync_category());
}

class xtop_state_sync_category final : public std::error_category {
    char const * name() const noexcept override {
        return "state_sync";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xstate_sync_category_t = xtop_state_sync_category;

std::error_category const & state_sync_category() {
    static xstate_sync_category_t category{};
    return category;
}

}  // namespace error
}  // namespace state_sync
}  // namespace top
