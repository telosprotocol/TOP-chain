// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xbasic_contract.h"
#include "xcontract_common/xerror/xerror.h"
#include "xdata/xtransaction.h"

#include <cassert>

NS_BEG2(top, contract_common)

xtop_contract_metadata::xtop_contract_metadata(xtop_contract_metadata const& meta): m_type(meta.m_type), m_account(meta.m_account) {}

common::xaccount_address_t xtop_basic_contract::address() const {
    return m_contract_meta.m_account;
}

xcontract_type_t xtop_basic_contract::type() const {
    return m_contract_meta.m_type;
}

observer_ptr<xcontract_state_t> const & xtop_basic_contract::state() const noexcept {
    return m_state;
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
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_t>();
    tx->make_tx_run_contract(data::xproperty_asset{0}, method_name, method_params);
    // tx->set_last_trans_hash_and_nonce(account_send_trans_hash(), account_send_trans_number()); //todo set nonce and hash
    tx->set_different_source_target_address(address().value(), target_addr.value());
    tx->set_digest();
    tx->set_len();

    m_associated_execution_context->add_followup_transaction(std::move(tx), type);
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

NS_END2
