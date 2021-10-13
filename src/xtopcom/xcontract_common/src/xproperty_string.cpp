// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_string.h"

NS_BEG3(top, contract_common, properties)

xtop_string_property::xtop_string_property(std::string const & prop_name, contract_common::xbasic_contract_t * contract)
  : xbasic_property_t{prop_name, state_accessor::properties::xproperty_type_t::string, make_observer(contract)} {
}

void xtop_string_property::create() {
    m_associated_contract->state()->create_property(m_id);
}

void xtop_string_property::update(std::string const & prop_value) {
    std::error_code ec;
    m_associated_contract->state()->set_property<state_accessor::properties::xproperty_type_t::string>(
        static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), prop_value, ec);
    top::error::throw_error(ec);
}

void xtop_string_property::clear() {
    std::error_code ec;
    m_associated_contract->state()->clear_property(m_id, ec);
    top::error::throw_error(ec);
}

std::string xtop_string_property::query() const {
    std::error_code ec;
    auto r = m_associated_contract->state()->get_property<state_accessor::properties::xproperty_type_t::string>(
        static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), ec);
    top::error::throw_error(ec);
    return r;
}

// std::string xtop_string_property::query(common::xaccount_address_t const & contract) const {
//     // return m_associated_contract->state()->access_control()->string_prop_query(accessor(), contract, m_id);
//     return {};
// }

NS_END3
