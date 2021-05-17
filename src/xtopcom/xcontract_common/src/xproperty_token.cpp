// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_token.h"
#include "xcontract_common/xerror/xerror.h"
#include "xcontract_common/xcontract_state.h"


NS_BEG3(top, contract_common, properties)

 bool xtop_token_safe::transfer_safe_rule(uint64_t amount) noexcept {
     return amount < MAX_SAFE_TOKEN;
 }

xtop_token_property::xtop_token_property(std::string const& prop_name, contract_common::xbasic_contract_t* contract)
                                           :xbasic_property_t{prop_name, xproperty_type_t::token, make_observer(contract)} {
    m_contract_state->access_control()->token_prop_create(accessor() , m_id);

}

uint64_t xtop_token_property::value() const {
    return m_contract_state->access_control()->balance(accessor(), m_id);
}

void xtop_token_property::withdraw(std::uint64_t amount) {
    xproperty_utl_t::property_assert(amount > value(),  error::xerrc_t::token_not_enough, "[xtop_token_property::withdraw]withdraw amount overflow, amount: " + std::to_string(amount));
    m_contract_state->access_control()->withdraw(accessor(), m_id, amount);
}

void xtop_token_property::deposit(std::uint64_t amount) {
    xproperty_utl_t::property_assert(xtoken_safe_t::transfer_safe_rule(amount),  error::xerrc_t::token_not_enough, "[xtop_token_property::deposit]deposit amount overflow, amount: " + std::to_string(amount));
    m_contract_state->access_control()->deposit(accessor(), m_id, amount);
}
NS_END3
