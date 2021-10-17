// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xbasic_contract.h"

#include "xbasic/xutility.h"
#include "xcontract_common/xerror/xerror.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction_v2.h"

#include <cassert>
#include <iostream>

NS_BEG2(top, contract_common)

xtop_contract_metadata::xtop_contract_metadata(common::xaccount_address_t const& account, xcontract_type_t type)
    : m_type(type), m_account(account) {
}

common::xaccount_address_t xtop_basic_contract::address() const {
    if (m_associated_execution_context) {
        return m_associated_execution_context->contract_state()->state_account_address();
    } else {
        return m_contract_meta.m_account;
    }
}

xcontract_type_t xtop_basic_contract::type() const {
    return m_contract_meta.m_type;
}

void xtop_basic_contract::register_property(properties::xbasic_property_t * property) {
    m_property_initializer.register_property(make_observer(property));
}

uint64_t xtop_basic_contract::balance() const {
    return m_balance.amount();
}

state_accessor::xtoken_t xtop_basic_contract::withdraw(std::uint64_t amount) {
    return m_balance.withdraw(amount);
}

void xtop_basic_contract::deposit(state_accessor::xtoken_t token) {
    assert(m_balance.symbol() == token.symbol());
    m_balance.deposit(std::move(token));
}

observer_ptr<xcontract_state_t> xtop_basic_contract::contract_state() const noexcept {
    assert(m_associated_execution_context != nullptr);
    return m_associated_execution_context->contract_state();
}

data::enum_xaction_type xtop_basic_contract::action_type() const {
    return m_associated_execution_context->action_type();
}

data::enum_xaction_type xtop_basic_contract::source_action_type() const {
    return m_associated_execution_context->source_action_type();
}

data::enum_xaction_type xtop_basic_contract::target_action_type() const {
    return m_associated_execution_context->target_action_type();
}

xbyte_buffer_t xtop_basic_contract::action_data() const {
    return m_associated_execution_context->action_data();
}

std::string xtop_basic_contract::source_action_data() const {
    return m_associated_execution_context->source_action_data();
}

std::string xtop_basic_contract::target_action_data() const {
    return m_associated_execution_context->target_action_data();
}

state_accessor::xtoken_t xtop_basic_contract::src_action_asset(std::error_code & ec) const {
    assert(!ec);

    auto& receipt_data = m_associated_execution_context->input_receipt_data();
    data::xproperty_asset asset_out{data::XPROPERTY_ASSET_TOP, uint64_t{0}};
    // assert(receipt_data.find(RECEITP_DATA_ASSET_OUT) != receipt_data.end());
    if (receipt_data.find(RECEITP_DATA_ASSET_OUT) != receipt_data.end()) {
        auto const src_asset_data = receipt_data.at(RECEITP_DATA_ASSET_OUT);
        receipt_data.erase(RECEITP_DATA_ASSET_OUT);
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)src_asset_data.data(), src_asset_data.size());
        stream >> asset_out.m_token_name;
        stream >> asset_out.m_amount;
    } else {
        asset_out.m_amount = 0;
        asset_out.m_token_name = "";
    }

    if (asset_out.m_token_name.empty()) asset_out.m_token_name = data::XPROPERTY_ASSET_TOP;
    return state_accessor::xtoken_t{asset_out.m_amount, asset_out.m_token_name};
}

data::enum_xtransaction_type xtop_basic_contract::transaction_type() const {
    return m_associated_execution_context->transaction_type();
}

observer_ptr<xcontract_state_t> xtop_basic_contract::state() const noexcept {
    return m_associated_execution_context->contract_state();
}

common::xaccount_address_t xtop_basic_contract::sender() const {
    return m_associated_execution_context->sender();
}

common::xaccount_address_t xtop_basic_contract::recver() const {
    return m_associated_execution_context->recver();
}

void xtop_basic_contract::call(common::xaccount_address_t const & target_addr,
                               std::string const & method_name,
                               std::string const & method_params,
                               xfollowup_transaction_schedule_type_t type) {
    assert(type != xfollowup_transaction_schedule_type_t::invalid);
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();

    tx->make_tx_run_contract(data::xproperty_asset{0}, method_name, method_params);
    tx->set_different_source_target_address(address().value(), target_addr.value());
    // tx->set_tx_type(data::enum_xtransaction_type::xtransaction_type_run_contract_new);
    data::xcons_transaction_ptr_t cons_tx;
    if (type == xfollowup_transaction_schedule_type_t::immediately) {
        // delay type need process nonce final
        auto latest_hash = state()->latest_followup_tx_hash();
        auto latest_nonce = state()->latest_followup_tx_nonce();
        tx->set_last_trans_hash_and_nonce(latest_hash, latest_nonce);
        tx->set_digest();
        tx->set_len();
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
        state()->latest_followup_tx_hash(cons_tx->get_tx_hash_256());
        state()->latest_followup_tx_nonce(cons_tx->get_tx_nonce());
    } else {
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    }

    xassert(cons_tx->is_self_tx() || cons_tx->is_send_tx());
    m_associated_execution_context->add_followup_transaction(std::move(cons_tx), type);
}

void xtop_basic_contract::transfer(common::xaccount_address_t const & target_addr, uint64_t amount, xfollowup_transaction_schedule_type_t type, std::error_code & ec) {
    assert(data::is_contract_address(common::xaccount_address_t{address()}));
    assert(type != xfollowup_transaction_schedule_type_t::invalid);
    if (data::is_user_contract_address(common::xaccount_address_t{address()})) {
        xwarn("xtop_basic_contract::transfer fail to create user contract transaction from:%s,to:%s,amount:%lu", address().value().c_str(), target_addr.value().c_str(), amount);
        ec = contract_common::error::xerrc_t::user_contract_forbid_create_transfer;
    }
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(address().value(), target_addr.value());
    tx->set_deposit(0);
    tx->set_fire_timestamp(timestamp());
    tx->set_expire_duration(0);
    data::xcons_transaction_ptr_t cons_tx;
    if (type == xfollowup_transaction_schedule_type_t::immediately) {
        // delay type need process nonce final
        auto latest_hash = state()->latest_followup_tx_hash();
        auto latest_nonce = state()->latest_followup_tx_nonce();
        tx->set_last_trans_hash_and_nonce(latest_hash, latest_nonce);
        tx->set_digest();
        tx->set_len();
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
        state()->latest_followup_tx_hash(cons_tx->get_tx_hash_256());
        state()->latest_followup_tx_nonce(cons_tx->get_tx_nonce());
        xdbg_info("xtop_basic_contract::transfer tx:%s,from:%s,to:%s,amount:%ld,nonce:%ld",
            tx->get_digest_hex_str().c_str(), address().value().c_str(), target_addr.value().c_str(), amount, tx->get_tx_nonce());
    } else {
        cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    }

    xassert(cons_tx->is_self_tx() || cons_tx->is_send_tx());
    m_associated_execution_context->add_followup_transaction(std::move(cons_tx), type);
}

std::vector<xfollowup_transaction_datum_t> xtop_basic_contract::followup_transaction() const {
    return m_associated_execution_context->followup_transaction();
}

void xtop_basic_contract::reset_execution_context(observer_ptr<xcontract_execution_context_t> exe_ctx) {
    m_associated_execution_context = exe_ctx;
}

observer_ptr<properties::xproperty_initializer_t const> xtop_basic_contract::property_initializer() const noexcept {
    return make_observer(std::addressof(m_property_initializer));
}

bool xtop_basic_contract::at_source_action_stage() const noexcept {
    assert(m_associated_execution_context != nullptr);
    return m_associated_execution_context->execution_stage() == xcontract_execution_stage_t::source_action;
}

bool xtop_basic_contract::at_target_action_stage() const noexcept {
    assert(m_associated_execution_context != nullptr);
    return m_associated_execution_context->execution_stage() == xcontract_execution_stage_t::target_action;
}

bool xtop_basic_contract::at_confirm_action_stage() const noexcept {
    assert(m_associated_execution_context != nullptr);
    return m_associated_execution_context->execution_stage() == xcontract_execution_stage_t::confirm_action;
}

xbyte_buffer_t const & xtop_basic_contract::receipt_data(std::string const & key, std::error_code & ec) const {
    return m_associated_execution_context->input_receipt_data(key, ec);
}
void xtop_basic_contract::write_receipt_data(std::string const & key, xbyte_buffer_t value, std::error_code & ec) {
    auto& receipt_data = m_associated_execution_context->output_receipt_data();
    auto const it = receipt_data.find(key);
    if (it != std::end(receipt_data)) {
        ec = error::xerrc_t::receipt_data_already_exist;
        return;
    }

    receipt_data.emplace(key, std::move(value));
}

common::xlogic_time_t xtop_basic_contract::time() const {
    return m_associated_execution_context->contract_state()->time();
}

common::xlogic_time_t xtop_basic_contract::timestamp() const {
    return m_associated_execution_context->contract_state()->timestamp();
}

uint64_t xtop_basic_contract::state_height(common::xaccount_address_t const & address) const {
    return m_associated_execution_context->contract_state()->state_height(address);
}

bool xtop_basic_contract::block_exist(common::xaccount_address_t const & address, uint64_t height) const {
    return m_associated_execution_context->contract_state()->block_exist(address, height);
}

NS_END2
