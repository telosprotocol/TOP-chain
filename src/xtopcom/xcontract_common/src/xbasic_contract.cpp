// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xbasic_contract.h"
#include "xcontract_common/xerror/xerror.h"
#include "xdata/xtransaction_v2.h"

#include <cassert>
#include <iostream>

NS_BEG2(top, contract_common)

xtop_contract_metadata::xtop_contract_metadata(common::xaccount_address_t const& account, xcontract_type_t type)
    : m_type(type), m_account(account) {
}

//xtop_basic_contract::xtop_basic_contract(observer_ptr<contract_common::xcontract_execution_context_t> const & exec_context)
//    : m_associated_execution_context(exec_context)
//    , m_state(exec_context->contract_state())
//    , m_contract_meta(exec_context->contract_address()) {
//}

//xcontract_execution_result_t xtop_basic_contract::execute(observer_ptr<xcontract_execution_context_t> exe_ctx) {
//    // just do nothing
//    return exe_ctx->execution_result();
//}

common::xaccount_address_t xtop_basic_contract::address() const {
    if (m_associated_execution_context) {
        return m_associated_execution_context->contract_state()->access_control()->address();
    } else {
        return m_contract_meta.m_account;
    }
}

xcontract_type_t xtop_basic_contract::type() const {
    return m_contract_meta.m_type;
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

state_accessor::xtoken_t xtop_basic_contract::src_action_asset(std::error_code& ec) const {
    assert(!ec);

    auto src_data = source_action_data();
    if (src_data.empty() || data::enum_xaction_type::xaction_type_asset_out != source_action_type()) {
        ec = error::xerrc_t::src_action_asset_not_exist;
        return state_accessor::xtoken_t{};
    }

    data::xproperty_asset asset_out{data::XPROPERTY_ASSET_TOP, uint64_t{0}};
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)src_data.data(), src_data.size());
    stream >> asset_out.m_token_name;
    stream >> asset_out.m_amount;

    if (asset_out.m_token_name.empty()) asset_out.m_token_name = data::XPROPERTY_ASSET_TOP;
    return state_accessor::xtoken_t{asset_out.m_amount, asset_out.m_token_name};
}

uint64_t  xtop_basic_contract::src_action_asset_amount(std::error_code& ec) const {
    auto token = src_action_asset(ec);
    auto amount = token.value();
    token.clear();
    return amount;
}

std::string xtop_basic_contract::src_action_asset_name(std::error_code& ec) const {
    auto token = src_action_asset(ec);
    auto token_name = token.symbol();
    token.clear();
    return token_name;
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
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();

    auto latest_hash = state()->latest_sendtx_hash();
    auto latest_nonce = state()->latest_sendtx_nonce();
    tx->make_tx_run_contract(data::xproperty_asset{0}, method_name, method_params);
    tx->set_last_trans_hash_and_nonce(latest_hash, latest_nonce);
    tx->set_different_source_target_address(address().value(), target_addr.value());
    tx->set_digest();
    tx->set_len();
    tx->set_tx_type(data::enum_xtransaction_type::xtransaction_type_run_contract);
    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());

    m_associated_execution_context->add_followup_transaction(std::move(cons_tx), type);
}

std::vector<xfollowup_transaction_datum_t> xtop_basic_contract::followup_transaction() {
    return m_associated_execution_context->followup_transaction();
}

void xtop_basic_contract::reset_execution_context(observer_ptr<xcontract_execution_context_t> exe_ctx) {
    m_associated_execution_context = exe_ctx;
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
    return m_associated_execution_context->receipt_data(key, ec);
}
void xtop_basic_contract::write_receipt_data(std::string const & key, xbyte_buffer_t value, std::error_code & ec) {
    auto& receipt_data = m_associated_execution_context->receipt_data();
    auto const it = receipt_data.find(key);
    if (it != std::end(receipt_data)) {
        ec = error::xerrc_t::receipt_data_already_exist;
        return;
    }

    receipt_data.emplace(key, std::move(value));
}

common::xlogic_time_t xtop_basic_contract::time() const {
    return m_associated_execution_context->time();
}

NS_END2
