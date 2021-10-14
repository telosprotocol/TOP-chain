// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_uint64.h"

NS_BEG3(top, contract_common, properties)

xtop_uint64_property::xtop_uint64_property(std::string const & prop_name, contract_common::xbasic_contract_t * contract)
  : xbasic_property_t{prop_name, state_accessor::properties::xproperty_type_t::uint64, make_observer(contract)} {
}

void xtop_uint64_property::create() {
    m_associated_contract->contract_state()->create_property(m_id);
}

void xtop_uint64_property::update(uint64_t const & prop_value) {
    m_associated_contract->contract_state()->set_property<state_accessor::properties::xproperty_type_t::uint64>(
        static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), prop_value);
}

void xtop_uint64_property::clear() {
    m_associated_contract->contract_state()->clear_property(m_id);
}

uint64_t xtop_uint64_property::query() const {
    return m_associated_contract->contract_state()->get_property<state_accessor::properties::xproperty_type_t::uint64>(
        static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id));
}

NS_END3
