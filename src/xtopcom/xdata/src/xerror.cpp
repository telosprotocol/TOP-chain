// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xerror/xerror.h"
#include <string>
NS_BEG3(top, data, error)

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::update_state_failed:
        return "update state failed";

    case xerrc_t::update_state_block_height_mismatch:
        return "update state failed due to mismatch block height";

    case xerrc_t::update_state_block_type_mismatch:
        return "update state failed due to mismatch block type";

    default:
        return "unknown data module error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), data_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), data_category());
}

class xtop_data_category final : public std::error_category {
    char const * name() const noexcept override {
        return "data";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xdata_category_t = xtop_data_category;

std::error_category const & data_category() {
    static xdata_category_t category{};
    return category;
}

NS_END3
