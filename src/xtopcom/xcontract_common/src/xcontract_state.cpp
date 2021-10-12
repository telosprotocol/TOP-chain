// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xcontract_state.h"

#include "xbasic/xerror/xerror.h"
#include "xbasic/xutility.h"

#include <cassert>

NS_BEG2(top, contract_common)

//xtop_contract_state::xtop_contract_state(common::xaccount_address_t action_account_addr, observer_ptr<properties::xproperty_access_control_t> ac) : m_action_account_address{ action_account_addr}, m_ac{ac} {
//}

xtop_contract_state::xtop_contract_state(common::xaccount_address_t action_account_addr,
                                         observer_ptr<state_accessor::xstate_accessor_t> sa,
                                         xcontract_execution_param_t const & execution_param)
  : m_action_account_address{std::move(action_account_addr)}, m_state_accessor{sa}, m_param{execution_param} {
}

common::xaccount_address_t xtop_contract_state::state_account_address() const {
    assert(m_state_accessor != nullptr);
    return m_state_accessor->account_address();
}

state_accessor::xtoken_t xtop_contract_state::withdraw(state_accessor::properties::xproperty_identifier_t const & property_id,
                                                       common::xsymbol_t const & symbol,
                                                       uint64_t amount,
                                                       std::error_code & ec) {
    assert(!ec);
    assert(m_state_accessor != nullptr);

    return m_state_accessor->withdraw(property_id, symbol, amount, ec);
}

state_accessor::xtoken_t xtop_contract_state::withdraw(state_accessor::properties::xproperty_identifier_t const & property_id, common::xsymbol_t const & symbol, uint64_t amount) {
    std::error_code ec;
    auto r = withdraw(property_id, symbol, amount, ec);
    top::error::throw_error(ec);
    return r;
}

void xtop_contract_state::deposit(state_accessor::properties::xproperty_identifier_t const & property_id, state_accessor::xtoken_t tokens, std::error_code & ec) {
    assert(!ec);
    assert(m_state_accessor != nullptr);
    m_state_accessor->deposit(property_id, std::move(tokens), ec);
}

void xtop_contract_state::deposit(state_accessor::properties::xproperty_identifier_t const & property_id, state_accessor::xtoken_t tokens) {
    std::error_code ec;
    deposit(property_id, std::move(tokens), ec);
    top::error::throw_error(ec);
}

void xtop_contract_state::create_property(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec) {
    assert(m_state_accessor != nullptr);
    m_state_accessor->create_property(property_id, ec);
}

void xtop_contract_state::create_property(state_accessor::properties::xproperty_identifier_t const & property_id) {
    std::error_code ec;
    create_property(property_id, ec);
    top::error::throw_error(ec);
}

bool xtop_contract_state::property_exist(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec) const {
    assert(m_state_accessor != nullptr);
    return m_state_accessor->property_exist(property_id, ec);
}

bool xtop_contract_state::property_exist(state_accessor::properties::xproperty_identifier_t const & property_id) const {
    std::error_code ec;
    auto const r = property_exist(property_id, ec);
    top::error::throw_error(ec);
    return r;
}

void xtop_contract_state::deploy_bin_code(state_accessor::properties::xproperty_identifier_t const & property_id, xbyte_buffer_t code, std::error_code & ec) {
    assert(!ec);
    assert(m_state_accessor != nullptr);

    m_state_accessor->deploy_bin_code(property_id, std::move(code), ec);
}

void xtop_contract_state::deploy_bin_code(state_accessor::properties::xproperty_identifier_t const & property_id, xbyte_buffer_t code) {
    std::error_code ec;
    deploy_bin_code(property_id, std::move(code), ec);
    top::error::throw_error(ec);
}

uint64_t xtop_contract_state::balance(state_accessor::properties::xproperty_identifier_t const & property_id,
                                      common::xsymbol_t const & symbol,
                                      std::error_code & ec) const {
    assert(m_state_accessor != nullptr);
    assert(!ec);

    return m_state_accessor->balance(property_id, symbol, ec);
}

uint64_t xtop_contract_state::balance(state_accessor::properties::xproperty_identifier_t const & property_id, common::xsymbol_t const & symbol) const {
    assert(m_state_accessor != nullptr);
    std::error_code ec;
    auto const r = m_state_accessor->balance(property_id, symbol, ec);
    top::error::throw_error(ec);
    return r;
}

std::string xtop_contract_state::binlog(std::error_code & ec) const {
    assert(m_state_accessor != nullptr);
    return m_state_accessor->binlog(ec);
}

std::string xtop_contract_state::binlog() const {
    assert(m_state_accessor != nullptr);
    return m_state_accessor->binlog();
}

std::string xtop_contract_state::fullstate_bin() const {
    assert(m_state_accessor != nullptr);
    return m_state_accessor->fullstate_bin();
}

common::xlogic_time_t xtop_contract_state::time() const {
    return m_param.m_clock;
}

common::xlogic_time_t xtop_contract_state::timestamp() const {
    return m_param.m_timestamp;
}

uint256_t xtop_contract_state::latest_sendtx_hash(std::error_code& ec) const {
    assert(m_ac);
    return m_ac->latest_sendtx_hash(ec);
}


uint256_t  xtop_contract_state::latest_sendtx_hash() const {
    assert(m_ac);
    return m_ac->latest_sendtx_hash();
}

void xtop_contract_state::latest_sendtx_hash(uint256_t hash, std::error_code& ec) {
    assert(m_ac);
    m_ac->latest_sendtx_hash(hash, ec);
}
void xtop_contract_state::latest_sendtx_hash(uint256_t hash) {
    assert(m_ac);
    m_ac->latest_sendtx_hash(hash);
}

uint64_t  xtop_contract_state::latest_sendtx_nonce(std::error_code& ec) const {
    assert(m_ac);
    return m_ac->latest_sendtx_nonce(ec);
}


uint64_t  xtop_contract_state::latest_sendtx_nonce() const {
    assert(m_ac);
    return m_ac->latest_sendtx_nonce();
}

void xtop_contract_state::latest_sendtx_nonce(uint64_t nonce, std::error_code& ec) {
    assert(m_ac);
    m_ac->latest_sendtx_nonce(nonce, ec);
}
void xtop_contract_state::latest_sendtx_nonce(uint64_t nonce) {
    assert(m_ac);
    m_ac->latest_sendtx_nonce(nonce);
}

uint256_t xtop_contract_state::latest_followup_tx_hash(std::error_code& ec) const {
    assert(m_ac);
    return m_ac->latest_followup_tx_hash(ec);
}

uint256_t xtop_contract_state::latest_followup_tx_hash() const {
    assert(m_ac);
    return m_ac->latest_followup_tx_hash();
}

void xtop_contract_state::latest_followup_tx_hash(uint256_t hash, std::error_code& ec) {
    assert(m_ac);
    m_ac->latest_followup_tx_hash(hash, ec);
}
void xtop_contract_state::latest_followup_tx_hash(uint256_t hash) {
    assert(m_ac);
    m_ac->latest_followup_tx_hash(hash);
}

uint64_t  xtop_contract_state::latest_followup_tx_nonce(std::error_code& ec) const {
    assert(m_ac);
    return m_ac->latest_followup_tx_nonce(ec);
}
uint64_t  xtop_contract_state::latest_followup_tx_nonce() const {
    assert(m_ac);
    return m_ac->latest_followup_tx_nonce();
}

void xtop_contract_state::latest_followup_tx_nonce(uint64_t nonce, std::error_code& ec) {
    assert(m_ac);
    m_ac->latest_followup_tx_nonce(nonce, ec);
}

void xtop_contract_state::latest_followup_tx_nonce(uint64_t nonce) {
    assert(m_ac);
    m_ac->latest_followup_tx_nonce(nonce);
}

template <>
void xtop_contract_state::set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                                                                                     state_accessor::properties::xkey_type_of_t<state_accessor::properties::xproperty_type_t::map>::type const & key,
                                                                                                     state_accessor::properties::xvalue_type_of_t<state_accessor::properties::xproperty_type_t::map>::type const & value,
                                                                                                     std::error_code & ec) {
    assert(m_state_accessor != nullptr);
    assert(!ec);
    m_state_accessor->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(property_id, key, value, ec);
}

template <>
state_accessor::properties::xvalue_type_of_t<state_accessor::properties::xproperty_type_t::map>::type
xtop_contract_state::get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                                                                                state_accessor::properties::xkey_type_of_t<state_accessor::properties::xproperty_type_t::map>::type const & key,
                                                                                                std::error_code & ec) const {
    assert(m_state_accessor != nullptr);
    assert(!ec);

    return m_state_accessor->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(property_id, key, ec);
}

template <>
bool xtop_contract_state::exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                                                                                     state_accessor::properties::xkey_type_of_t<state_accessor::properties::xproperty_type_t::map>::type const & key,
                                                                                                     std::error_code & ec) const {
    assert(m_state_accessor != nullptr);
    assert(!ec);

    return m_state_accessor->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(property_id, key, ec);
}
NS_END2
