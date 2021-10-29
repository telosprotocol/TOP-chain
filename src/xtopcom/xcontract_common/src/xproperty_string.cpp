// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_string.h"

NS_BEG3(top, contract_common, properties)

xtop_string_property::xtop_string_property(std::string const & name, xcontract_face_t * contract)
    : xbasic_property_t{name, state_accessor::properties::xproperty_type_t::string, make_observer(contract)} {
}

xtop_string_property::xtop_string_property(std::string const & name, std::unique_ptr<xcontract_state_t> state_owned) : xbasic_property_t {name, state_accessor::properties::xproperty_type_t::string, std::move(state_owned)} {
}

void xtop_string_property::set(std::string const & value) {
    assert(associated_state() != nullptr);
    associated_state()->xcontract_state_t::set_property<state_accessor::properties::xproperty_type_t::string>(typeless_id(), value);
}

void xtop_string_property::clear() {
    assert(associated_state() != nullptr);
    associated_state()->clear_property(id());
}

std::string xtop_string_property::value() const {
    assert(associated_state() != nullptr);
    return associated_state()->xcontract_state_t::get_property<state_accessor::properties::xproperty_type_t::string>(typeless_id());
}

//std::string xtop_string_property::value(common::xaccount_address_t const & contract) const {
//    assert(m_associated_contract != nullptr);
//    assert(m_associated_contract->contract_state() != nullptr);
//    return m_associated_contract->contract_state()->get_property<state_accessor::properties::xproperty_type_t::string>(
//        static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), contract);
//}

size_t xtop_string_property::size() const {
    assert(associated_state() != nullptr);
    return associated_state()->property_size(id());
}

bool xtop_string_property::empty() const {
    return size() == 0;
}

NS_END3
