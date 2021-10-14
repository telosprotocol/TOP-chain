// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_string.h"

NS_BEG3(top, contract_common, properties)

xtop_string_property::xtop_string_property(std::string const & name, xcontract_face_t * contract)
    : xbasic_property_t{name, state_accessor::properties::xproperty_type_t::string, make_observer(contract)} {
}

void xtop_string_property::create() {
    assert(m_associated_contract != nullptr);
    assert(m_associated_contract->contract_state() != nullptr);
    m_associated_contract->contract_state()->create_property(m_id);
}

void xtop_string_property::set(std::string const & value) {
    assert(m_associated_contract != nullptr);
    assert(m_associated_contract->contract_state() != nullptr);
    m_associated_contract->contract_state()->set_property<state_accessor::properties::xproperty_type_t::string>(
        static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), value);
}

void xtop_string_property::clear() {
    assert(m_associated_contract != nullptr);
    assert(m_associated_contract->contract_state() != nullptr);
    m_associated_contract->contract_state()->clear_property(m_id);
}

std::string xtop_string_property::value() const {
    assert(m_associated_contract != nullptr);
    assert(m_associated_contract->contract_state() != nullptr);
    return m_associated_contract->contract_state()->get_property<state_accessor::properties::xproperty_type_t::string>(
        static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id));
}

// std::string xtop_string_property::query(common::xaccount_address_t const & contract) const {
//     // return m_associated_contract->state()->access_control()->string_prop_query(accessor(), contract, m_id);
//     return {};
// }

NS_END3
