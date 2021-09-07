// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_accessor/xunitstate.h"

#include <cstdint>

namespace top {
namespace state_accessor {

uint64_t xtop_unit_state::balance() const noexcept {
    return 0;
}

uint64_t xtop_unit_state::balance(std::string const & symbol) const noexcept {
    return 0;
}

uint64_t xtop_unit_state::nonce() const noexcept {
    return 0;
}

xtoken_t xtop_unit_state::withdraw(uint64_t const amount, std::error_code & ec) {
    return xtoken_t{};
}
xtoken_t xtop_unit_state::withdraw(std::string const & symbol, uint64_t const amount, std::error_code & ec) {
    return xtoken_t{};
}

// ============================== account context related APIs ==============================
xobject_ptr_t<base::xvbstate_t> xtop_unit_state::internal_state_object(common::xaccount_address_t const & account_address, std::error_code & ec) const {
    if (account_address.empty()) {
        return m_state;
    }

    auto other_account = m_store->query_account(account_address.value());
    if (other_account == nullptr) {
        ec = error::xerrc_t::load_account_state_failed;
        return nullptr;
    }

    auto other_state = other_account->get_bstate();
    if (other_state == nullptr) {
        ec = error::xerrc_t::load_account_state_failed;
        return nullptr;
    }

    return other_state;
}

std::map<std::string, xbyte_buffer_t> xtop_unit_state::get_map(properties::xtypeless_property_identifier_t const & property_id,
                                                               common::xaccount_address_t const & other_account_address,
                                                               std::error_code & ec) const{
    auto const & other_state = internal_state_object(other_account_address, ec);
    if (ec) {
        return {};
    }

    auto const & peroperty_name = property_id.full_name();
    auto map_property = other_state->load_string_map_var(peroperty_name);
    if (map_property == nullptr) {
        assert(!properties::system_property(property_id));
        ec = error::xerrc_t::property_not_exist;
        return {};
    }

    auto map = map_property->query();
    properties::xtype_of_t<properties::xproperty_type_t::map>::type ret;
    for (auto & pair : map) {
        ret.insert({std::move(pair.first), {std::begin(pair.second), std::end(pair.second)}});
    }
    return ret;
}

std::string xtop_unit_state::get_string(properties::xtypeless_property_identifier_t const & property_id,
                                        common::xaccount_address_t const & other_account_address,
                                        std::error_code & ec) const {
    auto const & other_state = internal_state_object(other_account_address, ec);
    if (ec) {
        return {};
    }

    auto const & peroperty_name = property_id.full_name();
    auto string_property = other_state->load_string_var(peroperty_name);
    if (string_property == nullptr) {
        if (!properties::system_property(property_id)) {
            ec = error::xerrc_t::property_not_exist;
        }
        return {};
    }

    return string_property->query();
}
}
}
