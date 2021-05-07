// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xbasic_property.h"

#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xbasic_contract.h"

NS_BEG3(top, contract_common, properties)

xtop_basic_property::xtop_basic_property(std::string const& name, xproperty_type_t type, observer_ptr<xbasic_contract_t> associated_contract) noexcept
            :m_associated_contract{associated_contract}, m_contract_state{associated_contract->state()}, m_id{name, type, convert_from_contract_type(m_associated_contract->type())}, m_owner{m_contract_state->state_account_address()} {}


xproperty_identifier_t const & xtop_basic_property::identifier() const {
    return m_id;
}

xproperty_category_t xtop_basic_property::convert_from_contract_type(contract_common::xcontract_type_t type) {
    switch (type)
    {
    case contract_common::xcontract_type_t::sys_kernel:
        return xproperty_category_t::sys_kernel;

    case contract_common::xcontract_type_t::sys_business:
        return xproperty_category_t::sys_business;

    case contract_common::xcontract_type_t::user:
        return xproperty_category_t::user;

    default:
        return xproperty_category_t::invalid;
    }
}

common::xaccount_address_t xtop_basic_property::owner() const {
    return m_owner;
}

common::xaccount_address_t xtop_basic_property::accessor() const {
    return m_contract_state->state_account_address();
}

NS_END3
