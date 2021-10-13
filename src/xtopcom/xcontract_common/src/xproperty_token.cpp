// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_token.h"

#include "xcontract_common/xerror/xerror.h"
#include "xstate_accessor/xproperties/xproperty_type.h"
#include "xcontract_common/xproperties/xproperty_access_control.h"


NS_BEG3(top, contract_common, properties)

bool xtop_token_safe::transfer_safe_rule(uint64_t amount) noexcept {
    return amount < MAX_SAFE_TOKEN;
}

xtop_token_property::xtop_token_property(std::string const & prop_name, common::xsymbol_t symbol, contract_common::xbasic_contract_t * contract)
: xbasic_property_t{prop_name, state_accessor::properties::xproperty_type_t::token, make_observer(contract)}, symbol_{std::move(symbol)} {
}

 xtop_token_property::xtop_token_property(std::string const& prop_name, contract_common::xbasic_contract_t* contract)
    : xbasic_property_t{ prop_name, state_accessor::properties::xproperty_type_t::token, make_observer(contract) } {
}

xtop_token_property::xtop_token_property(common::xsymbol_t symbol, contract_common::xbasic_contract_t * contract)
  : xbasic_property_t{data::XPROPERTY_BALANCE_AVAILABLE, state_accessor::properties::xproperty_type_t::token, make_observer(contract)}, symbol_{std::move(symbol)} {
}

xtop_token_property::xtop_token_property(contract_common::xbasic_contract_t * contract)
  : xbasic_property_t{data::XPROPERTY_BALANCE_AVAILABLE, state_accessor::properties::xproperty_type_t::token, make_observer(contract)} {
}

void xtop_token_property::create() {
    assert(m_associated_contract != nullptr);
    m_associated_contract->state()->create_property(m_id);
}

uint64_t xtop_token_property::amount() const {
    assert(m_associated_contract != nullptr);
    return m_associated_contract->state()->balance(m_id, symbol_);
}

state_accessor::xtoken_t xtop_token_property::withdraw(std::uint64_t amount) {
    xproperty_utl_t::property_assert(amount > this->amount(),  error::xerrc_t::token_not_enough, "[xtop_token_property::withdraw]withdraw amount overflow, amount: " + std::to_string(amount));
    return m_associated_contract->state()->withdraw(m_id, symbol_, amount);
}

void xtop_token_property::deposit(state_accessor::xtoken_t tokens) {
    xproperty_utl_t::property_assert(xtoken_safe_t::transfer_safe_rule(tokens.amount()),  error::xerrc_t::token_not_enough, "[xtop_token_property::deposit]deposit amount overflow, amount: " + std::to_string(tokens.amount()));
    m_associated_contract->state()->deposit(m_id, std::move(tokens));
}
NS_END3
