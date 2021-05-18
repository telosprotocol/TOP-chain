// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate/xstate_access_control.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xstate/xerror/xerror.h"

#include <cassert>

namespace top {
namespace state {

xtop_state_access_control::xtop_state_access_control(top::observer_ptr<top::base::xvbstate_t> bstate,
                                                     xstate_access_control_data_t ac_data)
    : bstate_{ bstate }, canvas_{ make_object_ptr<base::xvcanvas_t>() }, ac_data_{ std::move(ac_data) } {
}

static std::string token_property_name(properties::xproperty_identifier_t const & property_id, std::string const & symbol) {
    return property_id.full_name() + "_" + symbol;
}

xtoken_t xtop_state_access_control::withdraw(properties::xproperty_identifier_t const & property_id, std::string const & symbol, uint64_t const amount, std::error_code & ec) {
    assert(!ec);

    if (property_id.type() != properties::xproperty_type_t::token) {
        ec = error::xerrc_t::invalid_property_type;
        return xtoken_t{};
    }

    if (static_cast<base::vtoken_t>(amount) <= 0) {
        ec = error::xerrc_t::property_value_out_of_range;
        return xtoken_t{};
    }

    if (!write_permitted(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return xtoken_t{};
    }

    auto const & property_name = token_property_name(property_id, symbol);

    if (!bstate_->find_property(property_name)) {
        ec = error::xerrc_t::token_insufficient;
        return xtoken_t{ symbol };
    }

    auto token_property = bstate_->load_token_var(property_name);
    if (token_property == nullptr) {
        ec = error::xerrc_t::load_property_failed;
        return xtoken_t{ symbol };
    }

    auto const balance = token_property->get_balance();
    if (balance < 0 || amount > static_cast<uint64_t>(balance)) {
        ec = error::xerrc_t::token_insufficient;
        return xtoken_t{ symbol };
    }

    auto const new_balance = token_property->withdraw(static_cast<base::vtoken_t>(amount), canvas_.get());
    assert(new_balance < balance);
    return xtoken_t{ amount, symbol };
}

void xtop_state_access_control::deposit(properties::xproperty_identifier_t const & property_id, std::string const & symbol, xtoken_t & amount, std::error_code & ec) {
    assert(!ec);
    if (property_id.type() != properties::xproperty_type_t::token) {
        ec = error::xerrc_t::invalid_property_type;
        return;
    }

    if (static_cast<base::vtoken_t>(amount.value())) {
        ec = error::xerrc_t::property_value_out_of_range;
        return;
    }

    if (!write_permitted(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return;
    }

    auto const & property_name = token_property_name(property_id, symbol);

    xobject_ptr_t<base::xtokenvar_t> token_property{ nullptr };
    if (!bstate_->find_property(property_name)) {
        if (properties::xproperty_identifier_t::system_property(property_id)) {
            token_property = bstate_->new_token_var(property_name, canvas_.get());
        } else {
            ec = error::xerrc_t::property_not_exist;
            return;
        }
    } else {
        token_property = bstate_->load_token_var(property_name);
    }

    if (token_property == nullptr) {
        ec = error::xerrc_t::load_property_failed;
        return;
    }

    auto const balance = token_property->get_balance();
    auto const new_balance = token_property->deposit(static_cast<base::vtoken_t>(amount.value()), canvas_.get());
    if (new_balance < balance) {
        ec = error::xerrc_t::property_value_out_of_range;
        return;
    }

    amount.clear();
}

uint64_t xtop_state_access_control::balance(properties::xproperty_identifier_t const & property_id, std::string const & symbol, std::error_code & ec) const {
    if (property_id.type() != properties::xproperty_type_t::token) {
        ec = error::xerrc_t::invalid_property_type;
        return 0 ;
    }

    if (!read_permitted(property_id)) {
        ec = error::xerrc_t::property_access_denied;
        return 0;
    }

    auto const & property_name = token_property_name(property_id, symbol);
    if (!bstate_->find_property(property_name)) {
        return 0;
    }

    auto token_property = bstate_->load_token_var(property_name);
    if (token_property == nullptr) {
        ec = error::xerrc_t::load_property_failed;
        return 0;
    }

    auto const balance = token_property->get_balance();
    return balance;
}

void xtop_state_access_control::create_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec) {
    if (property_id.category() == properties::xproperty_category_t::system) {
        ec = error::xerrc_t::property_access_denied;
        return;
    }

    switch (property_id.type()) {
    case properties::xproperty_type_t::string:
        do_create_string_property(property_id, ec);
        break;

    case properties::xproperty_type_t::map:
        break;

    case properties::xproperty_type_t::vector:
        break;

    default:
        break;
    }
}

bool xtop_state_access_control::read_permitted(properties::xproperty_identifier_t const & property_id) const noexcept {
    return true;
}

bool xtop_state_access_control::write_permitted(properties::xproperty_identifier_t const & property_id) const noexcept {
    return true;
}

bool xtop_state_access_control::read_permitted(std::string const & property_full_name) const noexcept {
    return true;
}

void xtop_state_access_control::do_create_string_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec) {
    assert(!ec);
    assert(property_id.category() == properties::xproperty_category_t::user);
    assert(property_id.type() == properties::xproperty_type_t::string);

    uint32_t const custom_property_name_max_len = XGET_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_name_max_len);
    auto const property_name = property_id.full_name();

    if (property_name.length() >= custom_property_name_max_len) {
        ec = error::xerrc_t::property_name_too_long;
        return;
    }

    auto const string_property = bstate_->new_string_var(property_name, canvas_.get());
    if (string_property == nullptr) {
        ec = error::xerrc_t::create_property_failed;
        return;
    }
}

}
}
