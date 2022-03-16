// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_accessor/xerror/xerror.h"

NS_BEG3(top, state_accessor, error)

static char const * errc_to_message(int const errc) noexcept {
    switch (static_cast<xerrc_t>(errc)) {
    case xerrc_t::ok:
        return "successful";

    case xerrc_t::invalid_state_backend:
        return "invalid state backend";

    case xerrc_t::token_insufficient:
        return "token insufficient";

    case xerrc_t::token_not_used:
        return "token not used";

    case xerrc_t::load_account_state_failed:
        return "load account state failed";

    case xerrc_t::token_symbol_not_matched:
        return "token symbol not matched";

    case xerrc_t::invalid_property_type:
        return "invalid property type";

    case xerrc_t::load_property_failed:
        return "load property fail";

    case xerrc_t::property_access_denied:
        return "property access denied";

    case xerrc_t::property_not_exist:
        return "property not exist";

    case xerrc_t::property_key_not_exist:
        return "property key not exist";

    case xerrc_t::property_key_out_of_range:
        return "property key out of range";

    case xerrc_t::property_already_exist:
        return "property already exist";

    case xerrc_t::property_value_out_of_range:
        return "property value out of range";

    case xerrc_t::property_name_out_of_range:
        return "property name length out of range";

    case xerrc_t::create_property_failed:
        return "create property failed";

    case xerrc_t::get_binlog_failed:
        return "get binlog failed.";

    case xerrc_t::property_id_conversion_invalid:
        return "property id conversion invalid";

    case xerrc_t::update_property_failed:
        return "update property failed";

    case xerrc_t::property_not_changed:
        return "property not changed";

    case xerrc_t::empty_property_name:
        return "property name is empty";

    default:
        return "unknown error";
    }
}

class xtop_state_category final : public std::error_category {
public:
    const char * name() const noexcept override {
        return "state";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xstate_category_t = xtop_state_category;

std::error_code make_error_code(xerrc_t errc) noexcept {
    return std::error_code{static_cast<int>(errc), state_category()};
}

std::error_condition make_error_condition(xerrc_t errc) noexcept {
    return std::error_condition{static_cast<int>(errc), state_category()};
}

std::error_category const & state_category() {
    static xstate_category_t category;
    return category;
}

NS_END3

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::state_accessor::error::xerrc_t>::operator()(top::state_accessor::error::xerrc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::state_accessor::error::xerrc_t>::type>(errc));
}

#endif

NS_END1
