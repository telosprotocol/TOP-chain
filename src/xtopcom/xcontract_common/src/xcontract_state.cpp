// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xcontract_state.h"

#include "xbasic/xerror/xerror.h"
#include "xbasic/xutility.h"

#include <cassert>

NS_BEG2(top, contract_common)

xtop_contract_state::xtop_contract_state(common::xaccount_address_t action_account_addr,
                                         observer_ptr<state_accessor::xstate_accessor_t> sa,
                                         xcontract_execution_param_t const & execution_param)
  : m_action_account_address{std::move(action_account_addr)}, m_state_accessor{sa}, m_param{execution_param} {
    std::error_code ec;
    m_latest_followup_tx_hash = latest_sendtx_hash(ec);
    top::error::throw_error(ec);
    m_latest_followup_tx_nonce = latest_sendtx_nonce(ec);
    top::error::throw_error(ec);
}

common::xaccount_address_t xtop_contract_state::state_account_address() const {
    assert(m_state_accessor != nullptr);
    return m_state_accessor->account_address();
}

uint64_t xtop_contract_state::state_height(common::xaccount_address_t const & address) const {
    assert(m_state_accessor != nullptr);
    return m_state_accessor->state_height(address);
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
    assert(!ec);
    m_state_accessor->create_property(property_id, ec);
}

void xtop_contract_state::create_property(state_accessor::properties::xproperty_identifier_t const & property_id) {
    std::error_code ec;
    create_property(property_id, ec);
    top::error::throw_error(ec);
}

bool xtop_contract_state::property_exist(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
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
    assert(!ec);
    return m_state_accessor->binlog(ec);
}

std::string xtop_contract_state::binlog() const {
    assert(m_state_accessor != nullptr);
    std::error_code ec;
    auto r = binlog(ec);
    top::error::throw_error(ec);
    return r;
}

std::string xtop_contract_state::fullstate_bin(std::error_code & ec) const {
    assert(m_state_accessor != nullptr);
    assert(!ec);
    return m_state_accessor->fullstate_bin(ec);
}

std::string xtop_contract_state::fullstate_bin() const {
    assert(m_state_accessor != nullptr);
    std::error_code ec;
    auto r = fullstate_bin(ec);
    top::error::throw_error(ec);
    return r;
}

common::xlogic_time_t xtop_contract_state::time() const {
    return m_param.m_clock;
}

common::xlogic_time_t xtop_contract_state::timestamp() const {
    return m_param.m_timestamp;
}

uint256_t xtop_contract_state::latest_sendtx_hash(std::error_code & ec) const {
    assert(!ec);
    auto r = get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
        state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system},
        data::XPROPERTY_TX_INFO_LATEST_SENDTX_HASH,
        ec);

    if (r.empty()) {
        return uint256_t{};
    } else {
        return top::from_bytes<uint256_t>(r);
    }
}

void xtop_contract_state::latest_sendtx_hash(uint256_t hash, std::error_code & ec) {
    assert(!ec);
    set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
        state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system},
        data::XPROPERTY_TX_INFO_LATEST_SENDTX_HASH,
        top::to_bytes<uint256_t>(hash),
        ec);
}

uint64_t xtop_contract_state::latest_sendtx_nonce(std::error_code & ec) const {
    assert(!ec);
    auto r = get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
        state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system},
        data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM,
        ec);

    if (r.empty()) {
        return uint64_t(0);
    } else {
        return top::from_bytes<uint64_t>(r);
    }
}

void xtop_contract_state::latest_sendtx_nonce(uint64_t nonce, std::error_code & ec) {
    assert(!ec);
    set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
        state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system},
        data::XPROPERTY_TX_INFO_LATEST_SENDTX_NUM,
        top::to_bytes<uint64_t>(nonce),
        ec);
}

uint256_t xtop_contract_state::latest_followup_tx_hash() const {
    return m_latest_followup_tx_hash;
}

void xtop_contract_state::latest_followup_tx_hash(uint256_t hash) {
    m_latest_followup_tx_hash = hash;
}

uint64_t  xtop_contract_state::latest_followup_tx_nonce() const {
    return m_latest_followup_tx_nonce;
}

void xtop_contract_state::latest_followup_tx_nonce(uint64_t nonce) {
    m_latest_followup_tx_nonce = nonce;
}

uint64_t xtop_contract_state::recvtx_num(std::error_code & ec) const {
    assert(!ec);
    auto r = get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
        state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system},
        data::XPROPERTY_TX_INFO_RECVTX_NUM,
        ec);
    
    if (r.empty()) {
        return uint64_t(0);
    } else {
        return top::from_bytes<uint64_t>(r);
    }
}

void xtop_contract_state::recvtx_num(uint64_t num, std::error_code & ec) {
    assert(!ec);
    set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
        state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system},
        data::XPROPERTY_TX_INFO_RECVTX_NUM,
        top::to_bytes<uint64_t>(num),
        ec);
}

uint64_t xtop_contract_state::unconfirm_sendtx_num(std::error_code & ec) const {
    assert(!ec);
    auto r = get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
        state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system},
        data::XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM,
        ec);

    if (r.empty()) {
        return uint64_t(0);
    } else {
        return top::from_bytes<uint64_t>(r);
    }
}

void xtop_contract_state::unconfirm_sendtx_num(uint64_t num, std::error_code & ec) {
    assert(!ec);
    set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
        state_accessor::properties::xtypeless_property_identifier_t{data::XPROPERTY_TX_INFO, state_accessor::properties::xproperty_category_t::system},
        data::XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM,
        top::to_bytes<uint64_t>(num),
        ec);
}

void xtop_contract_state::create_time(std::error_code& ec) {

}

bool xtop_contract_state::block_exist(common::xaccount_address_t const & user, uint64_t height) const {
    assert(m_state_accessor != nullptr);
    return m_state_accessor->block_exist(user, height);
}

template <>
state_accessor::properties::xvalue_type_of_t<state_accessor::properties::xproperty_type_t::string>::type
xtop_contract_state::get_property<state_accessor::properties::xproperty_type_t::string>(state_accessor::properties::xtypeless_property_identifier_t const & property_id,
                                                                                        std::error_code & ec) const {
    assert(!ec);
    assert(m_state_accessor != nullptr);
    return m_state_accessor->get_property<state_accessor::properties::xproperty_type_t::string>(property_id, ec);
}

template <>
void xtop_contract_state::set_property<state_accessor::properties::xproperty_type_t::string>(
    state_accessor::properties::xtypeless_property_identifier_t const & property_id,
    state_accessor::properties::xtype_of_t<state_accessor::properties::xproperty_type_t::string>::type const & value,
    std::error_code & ec) {
    assert(!ec);
    assert(m_state_accessor != nullptr);
    m_state_accessor->set_property<state_accessor::properties::xproperty_type_t::string>(property_id, value, ec);
}

void xtop_contract_state::clear_property(state_accessor::properties::xproperty_identifier_t const & property_id, std::error_code & ec) {
    assert(!ec);
    assert(m_state_accessor != nullptr);
    m_state_accessor->clear_property(property_id, ec);
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
