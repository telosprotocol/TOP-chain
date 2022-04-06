// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xcontract_execution_context.h"

#include "xbasic/xutility.h"
#include "xcontract_common/xerror/xerror.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"

#include <cinttypes>

NS_BEG2(top, contract_common)

xtop_contract_execution_context::xtop_contract_execution_context(std::unique_ptr<data::xbasic_top_action_t const> action, observer_ptr<xcontract_state_t> s) noexcept
  : m_contract_state{s}, m_action{std::move(action)} {
}

observer_ptr<xcontract_state_t> xtop_contract_execution_context::contract_state() const noexcept {
    assert(m_contract_state != nullptr);
    return m_contract_state;
}

void xtop_contract_execution_context::contract_state(observer_ptr<xcontract_state_t> new_state) noexcept {
    m_contract_state = new_state;
}

data::xconsensus_action_stage_t xtop_contract_execution_context::consensus_action_stage() const noexcept {
    return m_stage;
}

void xtop_contract_execution_context::consensus_action_stage(data::xconsensus_action_stage_t const stage) noexcept {
    m_stage = stage;
}

xcontract_execution_result_t const & xtop_contract_execution_context::execution_result() const noexcept {
    return m_execution_result;
}

void xtop_contract_execution_context::add_followup_transaction(data::xcons_transaction_ptr_t tx, xfollowup_transaction_schedule_type_t type) {
    m_execution_result.output.followup_transaction_data.emplace_back(std::move(tx), type);
    xdbg("xtop_contract_execution_context::add_followup_transaction add followup tx, now size: %zu", m_execution_result.output.followup_transaction_data.size());
    for (size_t i = 0; i < m_execution_result.output.followup_transaction_data.size(); i++) {
        auto const & data = m_execution_result.output.followup_transaction_data[i];
        xdbg("dump followup tx: %u, %s, %d, %d", i, base::xstring_utl::to_hex(data.followed_transaction->get_tx_hash()).c_str());
    }
}

std::vector<xfollowup_transaction_datum_t> const & xtop_contract_execution_context::followup_transaction() const noexcept {
    return m_execution_result.output.followup_transaction_data;
}

void xtop_contract_execution_context::input_data(xbytes_t const & data) {
    m_input_data = data;
}

xbytes_t const & xtop_contract_execution_context::input_data() const {
    return m_input_data;
}

void xtop_contract_execution_context::input_receipt_data(data::xreceipt_data_store_t const& receipt_data) {
    m_receipt_data = receipt_data;
}

data::xreceipt_data_store_t& xtop_contract_execution_context::output_receipt_data() noexcept {
    return m_execution_result.output.receipt_data;
}

xbyte_buffer_t xtop_contract_execution_context::input_receipt_data(std::string const & key) const {
    return m_receipt_data.receipt_data_item(key);
}

void xtop_contract_execution_context::remove_input_receipt_data(std::string const & key) {
    m_receipt_data.remove_item(key);
}

//observer_ptr<xbasic_contract_t> xtop_contract_execution_context::system_contract(common::xaccount_address_t const & address) const noexcept {
//    return m_system_contract(address);
//}

//void xtop_contract_execution_context::system_contract(xcontract_object_cb_t cb) noexcept {
//    m_system_contract = cb;
//}

data::xaction_consensus_exec_status xtop_contract_execution_context::action_consensus_result() const noexcept {
    data::xaction_consensus_exec_status ret{data::xaction_consensus_exec_status::enum_xunit_tx_exec_status_fail};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->action_consensus_result();
        break;
    }

    default:
        assert(false);
        break;
    }

    return ret;
}

common::xaccount_address_t xtop_contract_execution_context::sender() const {
    common::xaccount_address_t ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system:
    case data::xtop_action_type_t::evm:
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->sender();
        break;
    default:
        assert(false);
        break;
    }
    return ret;
}

common::xaccount_address_t xtop_contract_execution_context::recver() const {
    common::xaccount_address_t ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = common::xaccount_address_t{static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->recver()};
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = common::xaccount_address_t{static_cast<data::xuser_consensus_action_t const *>(m_action.get())->recver()};
        break;
    }
    default:
        break;
    }
    return ret;
}

common::xaccount_address_t xtop_contract_execution_context::contract_address() const {
    return recver();
}

data::enum_xtransaction_type xtop_contract_execution_context::transaction_type() const noexcept {
    data::enum_xtransaction_type ret = data::enum_xtransaction_type::xtransaction_type_max;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->transaction_type();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->transaction_type();
        break;
    }
    default:
        break;
    }
    return ret;
}

std::string xtop_contract_execution_context::action_name() const {
    switch (consensus_action_stage()) {
    case data::xconsensus_action_stage_t::send:
    case data::xconsensus_action_stage_t::confirm:
        return source_action_name();

    case data::xconsensus_action_stage_t::self:
    case data::xconsensus_action_stage_t::recv:
        return target_action_name();

    default:
        assert(false);
        return {};
    }
}

std::string xtop_contract_execution_context::source_action_name() const {
    std::string ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->source_action_name();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->source_action_name();
        break;
    }
    default:
        break;
    }
    return ret;
}

std::string xtop_contract_execution_context::target_action_name() const {
    std::string ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->target_action_name();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->target_action_name();
        break;
    }
    default:
        break;
    }
    return ret;
}

data::enum_xaction_type xtop_contract_execution_context::action_type() const {
    data::enum_xaction_type ret{};
    switch (consensus_action_stage()) {
    case data::xconsensus_action_stage_t::send:
    case data::xconsensus_action_stage_t::confirm:
        ret = source_action_type();
        break;

    case data::xconsensus_action_stage_t::self:
    case data::xconsensus_action_stage_t::recv:
        ret = target_action_type();
        break;

    default:
        assert(false);
        break;
    }

    return ret;
}

data::enum_xaction_type xtop_contract_execution_context::source_action_type() const {
    data::enum_xaction_type ret{data::enum_xaction_type::xaction_type_asset_out};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->source_action_type();
        break;
    }

    default:
        assert(false);
        break;
    }

    return ret;
}

data::enum_xaction_type xtop_contract_execution_context::target_action_type() const {
    data::enum_xaction_type ret{data::enum_xaction_type::xaction_type_asset_out};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->target_action_type();
        break;
    }

    default:
        assert(false);
        break;
    }

    return ret;
}

xbytes_t xtop_contract_execution_context::action_data() const {
    xbytes_t ret;
    switch (consensus_action_stage()) {
    case data::xconsensus_action_stage_t::send:
    case data::xconsensus_action_stage_t::confirm:
        ret = source_action_data();
        break;

    case data::xconsensus_action_stage_t::recv:
    case data::xconsensus_action_stage_t::self:
        ret = target_action_data();
        break;

    default:
        assert(false);
        break;
    }

    return xbyte_buffer_t{ret.data(), ret.data() + ret.size()};
}

xbytes_t xtop_contract_execution_context::source_action_data() const {
    xbytes_t ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->source_action_data();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->source_action_data();
        break;
    }
    default:
        break;
    }
    return ret;
}

xbytes_t xtop_contract_execution_context::target_action_data() const {
    xbytes_t ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->target_action_data();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->target_action_data();
        break;
    }
    default:
        break;
    }
    return ret;
}

data::xconsensus_action_stage_t xtop_contract_execution_context::action_stage() const {
    data::xconsensus_action_stage_t ret = data::xconsensus_action_stage_t::invalid;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->stage();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->stage();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint64_t xtop_contract_execution_context::deposit() const {
    uint64_t ret{0};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->deposit();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->deposit();
        break;
    }
    default:
        break;
    }
    return ret;
}

std::string xtop_contract_execution_context::digest_hex() const {
    std::string ret{};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->digest_hex();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->digest_hex();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint64_t xtop_contract_execution_context::last_nonce() const {
    uint64_t ret{0};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_nonce();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->last_nonce();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint64_t xtop_contract_execution_context::nonce() const {
    uint64_t ret{0};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->nonce();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->nonce();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint256_t xtop_contract_execution_context::hash() const {
    uint256_t ret{};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->hash();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->hash();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint32_t xtop_contract_execution_context::size() const {
    uint32_t ret{0};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->size();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->size();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint32_t xtop_contract_execution_context::used_tgas() const {
    uint32_t ret{0};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->used_tgas();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->used_tgas();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint32_t xtop_contract_execution_context::used_disk() const {
    uint32_t ret{0};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->used_disk();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->used_disk();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint32_t xtop_contract_execution_context::last_action_send_tx_lock_tgas() const {
    uint32_t ret{0};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_action_send_tx_lock_tgas();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->last_action_send_tx_lock_tgas();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint32_t xtop_contract_execution_context::last_action_used_deposit() const {
    uint32_t ret{0};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_action_used_deposit();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->last_action_used_deposit();
        break;
    }
    default:
        break;
    }
    return ret;
}

uint32_t xtop_contract_execution_context::last_action_recv_tx_use_send_tx_tgas() const {
    uint32_t ret{0};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_action_recv_tx_use_send_tx_tgas();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->last_action_recv_tx_use_send_tx_tgas();
        break;
    }
    default:
        break;
    }
    return ret;
}

data::enum_xunit_tx_exec_status xtop_contract_execution_context::last_action_exec_status() const {
    data::enum_xunit_tx_exec_status ret{data::enum_xunit_tx_exec_status::enum_xunit_tx_exec_status_success};
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_action_exec_status();
        break;
    }
    case data::xtop_action_type_t::user: {
        ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->last_action_exec_status();
        break;
    }
    default:
        break;
    }
    return ret;
}

data::xproperty_asset xtop_contract_execution_context::asset() const {
    data::xproperty_asset asset_out{0};
    if (source_action_type() != data::xaction_type_asset_out) {
        return asset_out;
    }

    auto const & asset_param = source_action_data();
    if (asset_param.empty()) {
        return asset_out;
    }

    xdbg("[xtop_contract_execution_context::asset] parse asset");
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)asset_param.data(), (size_t)asset_param.size());
    stream >> asset_out.m_token_name;
    stream >> asset_out.m_amount;
    xdbg("[xtop_contract_execution_context::asset] asset: %" PRIu64, asset_out.m_amount);
    return asset_out;
}

void xtop_contract_execution_context::nonce_preprocess(std::error_code & ec) {
    auto const stage_ = consensus_action_stage();
    auto const last_nonce_ = last_nonce();
    auto const nonce_ = nonce();
    auto const hash_ = hash();

    // set create time
    state_accessor::properties::xtypeless_property_identifier_t time_property{data::XPROPERTY_ACCOUNT_CREATE_TIME, state_accessor::properties::xproperty_category_t::system};
    auto time_property_exist =
        contract_state()->property_exist(state_accessor::properties::xproperty_identifier_t{time_property, state_accessor::properties::xproperty_type_t::uint64});
    if (!time_property_exist) {
        auto create_time = contract_state()->time() == 0 ? base::TOP_BEGIN_GMTIME : contract_state()->time();
        contract_state()->set_property<state_accessor::properties::xproperty_type_t::uint64>(time_property, create_time);
    }

    // update nonce
    if (stage_ == data::xconsensus_action_stage_t::send || stage_ == data::xconsensus_action_stage_t::self) {
        auto state_nonce = contract_state()->latest_sendtx_nonce();
        if (state_nonce != last_nonce_) {
            xwarn("account nonce not matched. last nonce %" PRIu64 " state stored last noce %" PRIu64, last_nonce_, state_nonce);
            ec = error::xerrc_t::nonce_mismatch;
            return;
        }
        contract_state()->latest_sendtx_nonce(nonce_);
        contract_state()->latest_sendtx_hash(hash_);
        contract_state()->latest_followup_tx_nonce(nonce_);
        contract_state()->latest_followup_tx_hash(hash_);
    }
    if (stage_ == data::xconsensus_action_stage_t::send) {
        auto old_unconfirm_tx_num = contract_state()->unconfirm_sendtx_num();
        contract_state()->unconfirm_sendtx_num(old_unconfirm_tx_num + 1);
    } else if (stage_ == data::xconsensus_action_stage_t::recv) {
        // left it in final pack because recv count is special
        // auto old_recv_tx_num = contract_state()->recvtx_num();
        // contract_state()->recvtx_num(old_recv_tx_num + 1);
    } else if (stage_ == data::xconsensus_action_stage_t::confirm) {
        auto old_unconfirm_tx_num = contract_state()->unconfirm_sendtx_num();
        assert(old_unconfirm_tx_num > 0);
        contract_state()->unconfirm_sendtx_num(old_unconfirm_tx_num - 1);
    } else if (stage_ == data::xconsensus_action_stage_t::self) {
    } else {
        assert(false);
    }
}

xcontract_execution_fee_t xtop_contract_execution_context::action_preprocess(std::error_code & ec) {
    xcontract_execution_fee_t fee_change;
    auto const stage_ = consensus_action_stage();
    // default action
    if (stage_ == data::xconsensus_action_stage_t::send) {
        fee_change = execute_default_source_action(ec);
    } else if (stage_ == data::xconsensus_action_stage_t::recv) {
        // left it in final pack because recv count is special
        // auto old_recv_tx_num = contract_state()->recvtx_num();
        // contract_state()->recvtx_num(old_recv_tx_num + 1);
        fee_change = execute_default_target_action(ec);
    } else if (stage_ == data::xconsensus_action_stage_t::confirm) {
        fee_change = execute_default_confirm_action(ec);
    } else if (stage_ == data::xconsensus_action_stage_t::self) {
        fee_change = execute_default_target_action(ec);
    } else {
        assert(false);
    }
    return fee_change;
}

xcontract_execution_fee_t xtop_contract_execution_context::execute_default_source_action(std::error_code & ec) {
    xassert(sender() == contract_state()->state_account_address());
    xdbg("[xtop_contract_execution_context::execute_default_source_action] %s to %s", sender().value().c_str(), recver().value().c_str());

    if (sender().value() == sys_contract_zec_reward_addr) {
        xdbg("[xtop_contract_execution_context::execute_default_source_action] reward contract issue, ignore");
        return {};
    }
    if (base::xvaccount_t::is_contract_address_type(sender().type()) && base::xvaccount_t::is_unit_address_type(recver().type())) {
        xdbg("[xtop_contract_execution_context::execute_default_source_action] contract to user, ignore");
        return {};
    }

    xcontract_execution_fee_t fee_change;
    state_accessor::properties::xproperty_identifier_t balance_prop{
        data::XPROPERTY_BALANCE_AVAILABLE, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system};
    state_accessor::properties::xproperty_identifier_t burn_balance_prop{
        data::XPROPERTY_BALANCE_BURN, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system};
    state_accessor::properties::xproperty_identifier_t lock_balance_prop{
        data::XPROPERTY_BALANCE_LOCK, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system};

    // step1: service fee
    uint64_t service_fee{0};
#ifndef XENABLE_MOCK_ZEC_STAKE
    if (!data::is_sys_contract_address(sender()) && data::is_beacon_contract_address(recver())) {
        service_fee = XGET_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee);
    }
#endif
    xdbg("[xtop_contract_execution_context::execute_default_source_action] service_fee to burn: %" PRIu64, service_fee);
    if (service_fee > 0) {
        contract_state()->transfer_internal(balance_prop, burn_balance_prop, service_fee);
    }
    // step2: fee
#if defined(XENABLE_MOCK_ZEC_STAKE)
    if (data::is_sys_contract_address(recver()) && target_action_name() == "nodeJoinNetwork2") {
        contract_state()->deposit(balance_prop, state_accessor::xtoken_t{10000000000, common::SYMBOL_TOP_TOKEN});
    }
#endif
    auto const tx_deposit = deposit();
    xdbg("[xtop_contract_execution_context::execute_default_source_action] deposit: %" PRIu64, tx_deposit);
    auto const asset_ = asset();
    xdbg("[xtop_contract_execution_context::execute_default_source_action] asset: %" PRIu64, asset_.m_amount);
    auto const balance = contract_state()->balance(balance_prop, common::SYMBOL_TOP_TOKEN);
    xdbg("[xtop_contract_execution_context::execute_default_source_action] balance: %" PRIu64, balance);
    if (balance < asset_.m_amount + tx_deposit) {
        xwarn("[xtop_contract_execution_context::execute_default_source_action] balance not enough, balance: %" PRIu64 ", asset: %" PRIu64 ", deposit: %" PRIu64,
              balance,
              asset_.m_amount,
              tx_deposit);
        ec = error::xenum_errc::transaction_not_enough_balance;
        return {};
    }
    if (!data::is_sys_contract_address(sender())) {
        xdbg("sender not system contract, do tags calc!");
        update_tgas_disk_sender(true, fee_change, ec);
        if (ec) {
            xwarn("[xtop_contract_execution_context::execute_default_source_action] update_tgas_disk_sender failed, catagory: %s, msg: %s",
                  ec.category().name(),
                  ec.message().c_str());
            return {};
        }
    }
    if (tx_deposit > 0) {
        xdbg("[xtop_contract_execution_context::execute_default_source_action] do deposit to lock: %" PRIu64, tx_deposit);
        contract_state()->transfer_internal(balance_prop, lock_balance_prop, tx_deposit);
    }
    xdbg("[xtop_contract_execution_context::execute_default_source_action] deposit to lock ok: %" PRIu64, tx_deposit);

    return fee_change;
}

xcontract_execution_fee_t xtop_contract_execution_context::execute_default_target_action(std::error_code & ec) {
    xassert((recver() == m_contract_state->state_account_address()) ||
            (consensus_action_stage() == data::xconsensus_action_stage_t::self && data::is_black_hole_address(common::xaccount_address_t{recver()})));
    xdbg("[xtop_contract_execution_context::execute_default_target_action] %s to %s", sender().value().c_str(), recver().value().c_str());

    xcontract_execution_fee_t fee_change;
    // step1: fee
    // target_fee_exec();
    // step2: action exec
    if (consensus_action_stage() == data::xconsensus_action_stage_t::self && data::is_black_hole_address(common::xaccount_address_t{recver()})) {
        return fee_change;
    }
    if (!data::is_sys_contract_address(sender())) {
        // ret = m_fee.update_tgas_disk_after_sc_exec(trace);
        // xdbg("[target_action_exec] gas: %u, disk: %u, tx_hash: %s, source: %s, target: %s",
        //      trace->m_tgas_usage,
        //      trace->m_disk_usage,
        //      m_trans->get_digest_hex_str().c_str(),
        //      m_trans->get_source_addr().c_str(),
        //      m_trans->get_target_addr().c_str());
        // assert(0);
    } else {
        fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::send_tx_lock_tgas, last_action_send_tx_lock_tgas()));
        fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::recv_tx_use_send_tx_tgas, 0));
        fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_deposit, last_action_used_deposit()));
        xdbg("[xtop_contract_execution_context::execute_default_target_action] tx: %s, lock_tgas: %" PRIu32 ", use_send_tx_tgas: %" PRIu32 ", used_deposit: %" PRIu32,
             digest_hex().c_str(),
             last_action_send_tx_lock_tgas(),
             0,
             last_action_used_deposit());
    }

    return fee_change;
}

xcontract_execution_fee_t xtop_contract_execution_context::execute_default_confirm_action(std::error_code & ec) {
    xassert(sender() == contract_state()->state_account_address());
    xdbg("[xtop_contract_execution_context::execute_default_confirm_action] %s to %s", sender().value().c_str(), recver().value().c_str());

    xcontract_execution_fee_t fee_change;
    if (consensus_action_stage() == data::xconsensus_action_stage_t::self) {
        return fee_change;
    }
    if (data::is_sys_contract_address(sender())) {
        return fee_change;
    }

    state_accessor::properties::xproperty_identifier_t balance_prop{
        data::XPROPERTY_BALANCE_AVAILABLE, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system};
    state_accessor::properties::xproperty_identifier_t burn_balance_prop{
        data::XPROPERTY_BALANCE_BURN, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system};
    state_accessor::properties::xproperty_identifier_t lock_balance_prop{
        data::XPROPERTY_BALANCE_LOCK, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system};

    uint64_t lock_tgas = last_action_send_tx_lock_tgas();
    uint64_t used_deposit = last_action_used_deposit();
    uint64_t target_used_tgas = last_action_recv_tx_use_send_tx_tgas();
    uint64_t status = last_action_exec_status();
    uint64_t tx_deposit = deposit();
    xdbg("[xtop_contract_execution_context::execute_default_confirm_action] tx: %s, deposit: %u, recv_tx_use_send_tx_tgas: %llu, used_deposit: %u, lock_tgas: %u, status: %d",
         digest_hex().c_str(),
         tx_deposit,
         target_used_tgas,
         used_deposit,
         lock_tgas,
         status);
    if (lock_tgas > 0) {
        auto cur_lock_tgas = contract_state()->lock_tgas();
        assert(cur_lock_tgas >= lock_tgas);
        contract_state()->lock_tgas(cur_lock_tgas - lock_tgas);
    }
    xdbg("[xtop_contract_execution_context::execute_default_confirm_action] tgas out lock_tgas: %" PRIu64, tx_deposit);
    if (tx_deposit > 0) {
        contract_state()->transfer_internal(lock_balance_prop, balance_prop, tx_deposit);
    }
    xdbg("[xtop_contract_execution_context::execute_default_confirm_action] deposit out lock: %" PRIu64, tx_deposit);

    // 0 for contract
    // std::error_code ec;
    // calc_resource(tx_deposit, target_used_tgas, target_used_tgas, ec);
    // top::error::throw_error(ec);
    if (used_deposit > 0) {
        xdbg("[xtop_contract_execution_context::execute_default_confirm_action] use deposit for tgas: %" PRIu64 ", burn!", used_deposit);
        contract_state()->transfer_internal(balance_prop, burn_balance_prop, used_deposit);
    }
    fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_tgas, target_used_tgas));
    fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_deposit, used_deposit));

    return fee_change;
}

void xtop_contract_execution_context::update_tgas_disk_sender(bool is_contract, contract_common::xcontract_execution_fee_t & fee_change, std::error_code & ec) {
    // step0: check deposit
    auto const tx_deposit = deposit();
    auto const min_tx_deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit);
    if (!data::is_sys_contract_address(sender()) && tx_deposit < min_tx_deposit) {
        xwarn("[xtop_contract_execution_context::update_tgas_disk_sender] not_enough_deposit, sender: %s, recver: %s, deposit: %" PRIu64 ", min_deposit %" PRIu64,
              sender().value().c_str(),
              recver().value().c_str(),
              tx_deposit,
              min_tx_deposit);
        ec = error::xenum_errc::transaction_not_enough_deposit;
        return;
    }
    uint64_t tgas_usage = calc_cost_tgas(is_contract);
    uint64_t disk_usage = calc_cost_disk(is_contract);
    xdbg("[xtop_contract_execution_context::update_tgas_disk_sender] tx_deposit: %" PRIu64 ", cost_tgas: %" PRIu32 ", cost_disk: %" PRIu32, tx_deposit, tgas_usage, disk_usage);
    // step1: used tgas
    uint64_t tgas_deposit_usage{0};
    uint64_t tgas_self_usage{0};
    calc_used_tgas(tx_deposit, tgas_usage, tgas_deposit_usage, ec);
    if (ec) {
        return;
    }
    incr_used_tgas(tgas_usage, ec);
    if (ec) {
        return;
    }
    tgas_self_usage = tgas_usage - tgas_deposit_usage / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
    fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_tgas, tgas_self_usage));
    fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_deposit, tgas_deposit_usage));
    xdbg("[xtop_contract_execution_context::update_tgas_disk_sender] tgas_self_usage: %" PRIu64 ", tgas_deposit_usage: %" PRIu64, tgas_self_usage, tgas_deposit_usage);
    // step2: disk
    contract_state()->disk(disk_usage);
    fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_disk, disk_usage));
    xdbg("[xtop_contract_execution_context::update_tgas_disk_sender] disk_usage: %" PRIu64, disk_usage);
    // step3: frozen tgas
    auto const available_tgas = calc_available_tgas();
    auto const lock_tgas = contract_state()->lock_tgas();
    auto const frozen_tgas = std::min((tx_deposit - tgas_deposit_usage) / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio), available_tgas - lock_tgas);
    xdbg("[xtop_contract_execution_context::update_tgas_disk_sender] frozen_tgas: %" PRIu64 ", available_tgas: %" PRIu64 ", lock_tgas: %" PRIu64,
         frozen_tgas,
         available_tgas,
         lock_tgas);
    if (is_contract && frozen_tgas > 0) {
        fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::send_tx_lock_tgas, frozen_tgas));
        contract_state()->lock_tgas(frozen_tgas);
        xdbg("[xtop_contract_execution_context::update_tgas_disk_sender] tgas to lock_tgas: %" PRIu64, frozen_tgas);
    }
}

void xtop_contract_execution_context::calc_used_tgas(uint64_t deposit, uint64_t & cur_tgas_usage, uint64_t & deposit_usage, std::error_code & ec) const {
    xdbg("[xtop_contract_execution_context::calc_used_tgas] last_hour: %" PRIu64 ", timer_height: %" PRIu64 ", no decay used_tgas: %" PRIu64 ", decayed used_tgas: %" PRIu64
         ", token_price: %" PRIu64 ", total_tgas: %" PRIu64 ", tgas_usage: %" PRIu64 ", deposit: %" PRIu64,
         contract_state()->last_tx_hour(),
         contract_state()->time(),
         used_tgas(),
         calc_decayed_tgas(),
         calc_token_price(),
         calc_total_tgas(),
         cur_tgas_usage,
         deposit);
    auto available_tgas = calc_available_tgas();
    if (cur_tgas_usage > (available_tgas + deposit / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio))) {
        xwarn("[xtop_contract_execution_context::calc_used_tgas] not enough pledge token tgas, cur_tgas_usage: %" PRIu64 ", available_tgas: %" PRIu64 ", deposit: %" PRIu64,
              cur_tgas_usage,
              available_tgas,
              deposit);
        deposit_usage = deposit;
        cur_tgas_usage = available_tgas;
        ec = error::xenum_errc::transaction_not_enough_pledge_token_tgas;
    } else if (cur_tgas_usage > available_tgas) {
        deposit_usage = (cur_tgas_usage - available_tgas) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
        cur_tgas_usage = available_tgas;
        xdbg("[xtop_contract_execution_context::calc_used_tgas] available_tgas: %" PRIu64 ", cur_tgas_usage: %" PRIu64 ", tx_deposit_usage: %" PRIu64 ", deposit: %" PRIu64,
             available_tgas,
             deposit_usage,
             deposit);
    }
}

uint64_t xtop_contract_execution_context::calc_available_tgas() const {
    uint64_t available_tgas{0};
    auto token_price = calc_token_price();
    auto used_tgas = calc_decayed_tgas();
    auto total_tgas = calc_total_tgas();
    if (total_tgas > used_tgas) {
        available_tgas = total_tgas - used_tgas;
    }
    return available_tgas;
}

uint64_t xtop_contract_execution_context::calc_total_tgas() const {
    uint64_t max_tgas{0};
    auto const pledge_token = contract_state()->balance(
        state_accessor::properties::xproperty_identifier_t{
            data::XPROPERTY_BALANCE_PLEDGE_TGAS, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system},
        common::SYMBOL_TOP_TOKEN);
    auto token_price = calc_token_price();
    auto total_tgas = pledge_token * token_price / TOP_UNIT + calc_free_tgas();
    // contract account, max tgas is different
    if (data::is_user_contract_address(contract_state()->state_account_address())) {
        max_tgas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract);
    } else {
        max_tgas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account);
    }
    return std::min(total_tgas, max_tgas);
}

uint64_t xtop_contract_execution_context::calc_free_tgas() const {
    auto const balance = contract_state()->balance(
        state_accessor::properties::xproperty_identifier_t{
            data::XPROPERTY_BALANCE_AVAILABLE, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system},
        common::SYMBOL_TOP_TOKEN);
    auto const lock_balance = contract_state()->balance(
        state_accessor::properties::xproperty_identifier_t{
            data::XPROPERTY_BALANCE_LOCK, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system},
        common::SYMBOL_TOP_TOKEN);
    auto const tgas_balance = contract_state()->balance(
        state_accessor::properties::xproperty_identifier_t{
            data::XPROPERTY_BALANCE_PLEDGE_TGAS, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system},
        common::SYMBOL_TOP_TOKEN);
    auto const vote_balance = contract_state()->balance(
        state_accessor::properties::xproperty_identifier_t{
            data::XPROPERTY_BALANCE_PLEDGE_VOTE, state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xproperty_category_t::system},
        common::SYMBOL_TOP_TOKEN);
    uint64_t disk_balance = 0;  // disk is 0
    auto total_asset = balance + lock_balance + tgas_balance + disk_balance + vote_balance;
    auto min_free_tgas_asset = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset);
    if (total_asset >= min_free_tgas_asset) {
        return XGET_ONCHAIN_GOVERNANCE_PARAMETER(free_gas);
    } else {
        return 0;
    }
}

uint64_t xtop_contract_execution_context::calc_decayed_tgas() const {
    uint64_t decayed_tgas{0};
    auto last_hour = contract_state()->last_tx_hour();
    auto height = contract_state()->time();
    auto decay_time = XGET_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle);
    if (height <= last_hour) {
        decayed_tgas = contract_state()->used_tgas();
    } else if (height - last_hour < decay_time) {
        decayed_tgas = (decay_time - (height - last_hour)) * contract_state()->used_tgas() / decay_time;
    }
    return decayed_tgas;
}

uint64_t xtop_contract_execution_context::calc_cost_tgas(bool is_contract) const {
#ifdef ENABLE_SCALE
    uint16_t amplify = 5;
#else
    uint16_t amplify = 1;
#endif
    if (is_contract) {
        amplify = 1;
    }
    uint32_t multiple = (action_stage() == data::xconsensus_action_stage_t::self) ? 1 : 3;
    return multiple * amplify * size();
}

uint64_t xtop_contract_execution_context::calc_cost_disk(bool is_contract) const {
#ifdef ENABLE_SCALE
    uint16_t amplify = 100;
#else
    uint16_t amplify = 1;
#endif
#if 1
    return 0;
#else
    uint32_t multiple = 2;
    if (m_trans->get_transaction()->get_tx_type() == data::enum_xtransaction_type::xtransaction_type_transfer && m_trans->get_target_addr() == black_hole_addr) {
        multiple = 1;
    }
    return is_contract ? size() : multiple * amplify * size();
#endif
}

void xtop_contract_execution_context::incr_used_tgas(uint64_t num, std::error_code & ec) {
    auto decayed_tgas = calc_decayed_tgas();
    auto last_hour = contract_state()->time();
    contract_state()->used_tgas(num + decayed_tgas);
    contract_state()->last_tx_hour(last_hour);
    xdbg("[xtop_contract_execution_context::incr_used_tgas] last_hour_set: %" PRIu64 ", tgas_used this time: %" PRIu64 ", decayed: %" PRIu64 ", total: %" PRIu64,
         time,
         num,
         decayed_tgas,
         num + decayed_tgas);
}

uint64_t xtop_contract_execution_context::calc_token_price() const {
    auto initial_total_pledge_token = XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token);
    auto total_lock_tgas = contract_state()->system_lock_tgas();
    auto total_pledge_token = total_lock_tgas + initial_total_pledge_token;
    auto token_price = XGET_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard) * XGET_CONFIG(validator_group_count) * TOP_UNIT / total_pledge_token;
    xdbg("[xtop_contract_execution_context::calc_token_price] get total pledge token from beacon: %" PRIu64 " + %" PRIu64 " = %" PRIu64 ", price: %" PRIu64,
         initial_total_pledge_token,
         total_lock_tgas,
         total_pledge_token,
         token_price);
    return token_price;
}

NS_END2
