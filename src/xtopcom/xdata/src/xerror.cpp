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

    case xerrc_t::property_already_exist:
        return "property already exists";

    case xerrc_t::property_type_invalid:
        return "property type invalid";

    case xerrc_t::property_hash_mismatch:
        return "property hash mismatch";

    case xerrc_t::property_not_exist:
        return "property not exist";

    case xerrc_t::binlog_instruction_type_invalid:
        return "invalid binlog instruction type";

    case xerrc_t::election_data_start_time_invalid:
        return "election data start time invalid";

    case xerrc_t::checkpoint_not_found:
        return "checkpoint not found";

    case xerrc_t::action_address_type_error:
        return "action address type error";

    case xerrc_t::invalid_toke_id:
        return "invalid token id";

    case xerrc_t::erc20_allowance_spender_not_found:
        return "erc20 allowance spender not found";

    case xerrc_t::erc20_allowance_not_enough:
        return "erc20 allowance not enough";

    case xerrc_t::erc20_controller_already_set:
        return "erc20 controller already set";

    case xerrc_t::erc20_owner_already_set:
        return "erc20 owner already set";

    case xerrc_t::eth_server_error:
        return "eth transaction server error";
    case xerrc_t::eth_invalid_params:
        return "eth transaction invalid params";
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
