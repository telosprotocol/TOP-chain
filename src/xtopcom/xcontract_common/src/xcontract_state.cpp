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
    state_accessor::properties::xproperty_identifier_t src_property_id{
        "src_code", state_accessor::properties::xproperty_type_t::src_code, state_accessor::properties::xproperty_category_t::user};
    return m_ac->src_code(src_property_id, ec);
}

std::string xtop_contract_state::src_code() const {
    std::error_code ec;
    auto r = src_code(ec);
    top::error::throw_error(ec);
    return r;
}

void xtop_contract_state::deploy_src_code(std::string code, std::error_code & ec) {
    state_accessor::properties::xproperty_identifier_t src_property_id{
        "src_code", state_accessor::properties::xproperty_type_t::src_code, state_accessor::properties::xproperty_category_t::user};
    m_ac->deploy_src_code(src_property_id, std::move(code), ec);
}

void xtop_contract_state::deploy_src_code(std::string code) {
    std::error_code ec;
    deploy_src_code(std::move(code), ec);
    top::error::throw_error(ec);
}

void xtop_contract_state::deploy_bin_code(xbyte_buffer_t code, std::error_code & ec) {
    state_accessor::properties::xproperty_identifier_t src_property_id{
        "src_code", state_accessor::properties::xproperty_type_t::src_code, state_accessor::properties::xproperty_category_t::user};
    m_ac->deploy_bin_code(src_property_id, std::move(code), ec);
}

void xtop_contract_state::deploy_bin_code(xbyte_buffer_t code) {
    std::error_code ec;
    deploy_bin_code(std::move(code), ec);
    top::error::throw_error(ec);
}

std::string xtop_contract_state::binlog(std::error_code & ec) const {
    assert(m_ac);
    return m_ac->binlog(ec);
}

std::string xtop_contract_state::binlog() const {
    return m_ac->binlog();
}

std::string xtop_contract_state::fullstate_bin() const {
    return m_ac->fullstate_bin();
}

uint64_t xtop_contract_state::token_withdraw(uint64_t amount, std::error_code& ec) const {
    assert(m_ac);

    state_accessor::properties::xproperty_identifier_t property_id{
        data::XPROPERTY_BALANCE_AVAILABLE, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system};

    uint64_t res = 0;
    try {
        res = m_ac->withdraw(state_account_address(), property_id, amount);
    } catch (top::error::xtop_error_t const& err) {
        ec = err.code();
    }

    return  res;
}

uint64_t xtop_contract_state::token_withdraw(uint64_t amount) const {
    state_accessor::properties::xproperty_identifier_t property_id{
        data::XPROPERTY_BALANCE_AVAILABLE, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system};
    return m_ac->withdraw(state_account_address(), property_id, amount);
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

NS_END2
