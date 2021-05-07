// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xcontract_state.h"

#include "xbasic/xerror/xthrow_error.h"
#include "xbasic/xutility.h"

#include <cassert>

NS_BEG2(top, contract_common)

xtop_contract_state::xtop_contract_state(common::xaccount_address_t action_account_addr, observer_ptr<properties::xproperty_access_control_t> ac) : m_action_account_address{ action_account_addr}, m_ac{ac} {
}

observer_ptr<properties::xproperty_access_control_t> const& xtop_contract_state::access_control() const {
    return m_ac;
}

common::xaccount_address_t xtop_contract_state::state_account_address() const {
    return m_ac->address();
}


//bool xtop_contract_state::has_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec) const noexcept {
//    assert(m_ac != nullptr);
//    return m_ac->has_property(contract_account(), property_id, ec);
//}
//
//void xtop_contract_state::create_property(properties::xproperty_identifier_t const & property_id) {
//    std::error_code ec;
//    create_property(property_id, ec);
//    throw_error(ec);
//}
//
//bool xtop_contract_state::has_property(properties::xproperty_identifier_t const & property_id) const noexcept {
//    std::error_code ec;
//    return m_ac->has_property(contract_account(), property_id, ec);
//}

std::string xtop_contract_state::src_code(std::error_code & ec) const {
    properties::xproperty_identifier_t src_property_id{"src_code", properties::xproperty_type_t::src_code, properties::xproperty_category_t::user };
    return m_ac->src_code(src_property_id, ec);
}

std::string xtop_contract_state::src_code() const {
    std::error_code ec;
    auto r = src_code(ec);
    top::error::throw_error(ec);
    return r;
}

void xtop_contract_state::deploy_src_code(std::string code, std::error_code & ec) {
    properties::xproperty_identifier_t src_property_id{"src_code", properties::xproperty_type_t::src_code, properties::xproperty_category_t::user};
    m_ac->deploy_src_code(src_property_id, std::move(code), ec);
}

void xtop_contract_state::deploy_src_code(std::string code) {
    std::error_code ec;
    deploy_src_code(std::move(code), ec);
    top::error::throw_error(ec);
}

NS_END2
