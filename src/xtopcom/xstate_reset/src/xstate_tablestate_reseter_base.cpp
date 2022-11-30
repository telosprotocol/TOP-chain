// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_reset/xstate_tablestate_reseter_base.h"

#include "xstatectx/xstatectx.h"

NS_BEG2(top, state_reset)

xstate_tablestate_reseter_base::xstate_tablestate_reseter_base(statectx::xstatectx_face_ptr_t statectx_ptr)
  : m_statectx_ptr{statectx_ptr}, m_table_account{m_statectx_ptr->get_table_address()} {
}

void xstate_tablestate_reseter_base::account_set_tep1_token(std::string const & account_address, common::xtoken_id_t const & token_id, evm_common::u256 const & token_value) {
    auto unit_state = m_statectx_ptr->load_unit_state(common::xaccount_address_t{account_address});
    assert(unit_state);
    if (unit_state == nullptr) {
        xwarn("xstate_tablestate_reseter_base::account_set_tep1_token find empty unit state object of account %s", account_address.c_str());
        return;
    }
    unit_state->set_tep_balance(token_id, token_value);
}

void xstate_tablestate_reseter_base::account_set_top_balance(std::string const & account_address, std::string const & property_name, uint64_t property_value) {
    auto unit_state = m_statectx_ptr->load_unit_state(common::xaccount_address_t{account_address});
    assert(unit_state);
    if (unit_state == nullptr) {
        xwarn("xstate_tablestate_reseter_base::account_set_top_balance find empty unit state object of account %s", account_address.c_str());
        return;
    }
    // auto origin_balance = unit_state->token_balance(property_name);
    // if (origin_balance > property_value) {
    //     unit_state->token_withdraw(property_name, (base::vtoken_t)(origin_balance - property_value));
    // } else if (origin_balance < property_value) {
    //     unit_state->token_deposit(property_name, (base::vtoken_t)(property_value - origin_balance));
    // }
    unit_state->set_token_balance(property_name, (base::vtoken_t)property_value);
}

void xstate_tablestate_reseter_base::account_set_property(std::string const & account_address,
                                                          std::string const & property_name,
                                                          std::string const & property_type,
                                                          std::string const & property_value) {
    auto unit_state = m_statectx_ptr->load_unit_state(common::xaccount_address_t{account_address});
    assert(unit_state);
    if (unit_state == nullptr) {
        xwarn("xstate_tablestate_reseter_base::account_set_property find empty unit state object of account %s", account_address.c_str());
        return;
    }
    // unit_state->get_bstate()->get_property_value
    if (property_type == "map") {
        // TODO
        // unit_state->map_set(property_name,)
    }
    // else if (property_type == "string") {
    //     unit_state->string_set(property_name, property_value);
    // }
}

NS_END2