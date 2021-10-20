// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xcontract_execution_context.h"

#include "xbasic/xutility.h"
#include "xcontract_common/xerror/xerror.h"
#include "xdata/xgenesis_data.h"

NS_BEG2(top, contract_common)

//xtop_contract_execution_context::xtop_contract_execution_context(xobject_ptr_t<data::xtransaction_t> tx, observer_ptr<xcontract_state_t> s) noexcept
//  : m_contract_state{s}, m_tx{std::move(tx)} {
//}

xtop_contract_execution_context::xtop_contract_execution_context(std::unique_ptr<data::xbasic_top_action_t const> action,
                                                                 observer_ptr<xcontract_state_t> s) noexcept
  : m_contract_state{s}, m_action{std::move(action)} {
}

//xtop_contract_execution_context::xtop_contract_execution_context(std::unique_ptr<data::xbasic_top_action_t const> action,
//                                                                 observer_ptr<xcontract_state_t> s,
//                                                                 xcontract_execution_param_t param) noexcept
//  : m_contract_state{s}, m_action{std::move(action)} {
//}

observer_ptr<xcontract_state_t> xtop_contract_execution_context::contract_state() const noexcept {
    return m_contract_state;
}

xcontract_execution_stage_t xtop_contract_execution_context::execution_stage() const noexcept {
    return m_execution_stage;
}

void xtop_contract_execution_context::execution_stage(xcontract_execution_stage_t const stage) noexcept {
    m_execution_stage = stage;
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
}

std::vector<xfollowup_transaction_datum_t> const & xtop_contract_execution_context::followup_transaction() const noexcept {
    return m_execution_result.output.followup_transaction_data;
}

void xtop_contract_execution_context::input_receipt_data(std::map<std::string, xbyte_buffer_t> receipt_data) {
    m_receipt_data = std::move(receipt_data);
}

std::map<std::string, xbyte_buffer_t> & xtop_contract_execution_context::input_receipt_data() noexcept {
    return m_receipt_data;
}

std::map<std::string, xbyte_buffer_t> & xtop_contract_execution_context::output_receipt_data() noexcept {
    return m_execution_result.output.receipt_data;
}

xbyte_buffer_t const & xtop_contract_execution_context::input_receipt_data(std::string const & key, std::error_code & ec) const noexcept {
    static xbyte_buffer_t const empty;
    auto const it = m_receipt_data.find(key);
    if (it != std::end(m_receipt_data)) {
        return top::get<xbyte_buffer_t>(*it);
    }

    ec = error::xerrc_t::receipt_data_not_found;
    return empty;
}

common::xaccount_address_t xtop_contract_execution_context::sender() const {
    common::xaccount_address_t ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system:
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->from_address();
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
        case data::xtop_action_type_t::system:{
            ret = common::xaccount_address_t{static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->to_address()};
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = common::xaccount_address_t{static_cast<data::xuser_consensus_action_t const *>(m_action.get())->to_address()};
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->transaction_type();
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->transaction_type();
            break;
        }
        default:
            break;
    }
    return ret;
}

std::string xtop_contract_execution_context::action_name() const {
    if (data::xconsensus_action_stage_t::send == consensus_action_stage()) {
        return source_action_name();
    } else if (data::xconsensus_action_stage_t::recv == consensus_action_stage()) {
        return target_action_name();
    }

    return std::string{};
}

std::string xtop_contract_execution_context::source_action_name() const {
    std::string ret;
    switch (m_action->type()) {
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->source_action_name();
            break;
        }
        case data::xtop_action_type_t::user:{
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->target_action_name();
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->target_action_name();
            break;
        }
        default:
            break;
    }
    return ret;
}

data::enum_xaction_type xtop_contract_execution_context::action_type() const {
    data::enum_xaction_type ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->transaction_target_action_type();
        break;
    }

    default:
        assert(false);
        break;
    }

    return ret;
}

data::enum_xaction_type xtop_contract_execution_context::source_action_type() const {
    data::enum_xaction_type ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->transaction_source_action_type();
        break;
    }

    default:
        assert(false);
        break;
    }

    return ret;
}

data::enum_xaction_type xtop_contract_execution_context::target_action_type() const {
    data::enum_xaction_type ret;
    switch (m_action->type()) {
    case data::xtop_action_type_t::system: {
        ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->transaction_target_action_type();
        break;
    }

    default:
        assert(false);
        break;
    }

    return ret;
}

xbyte_buffer_t xtop_contract_execution_context::action_data() const {
    std::string ret;
    if (data::xconsensus_action_stage_t::send == consensus_action_stage()) {
        ret = source_action_data();
    } else if (data::xconsensus_action_stage_t::recv == consensus_action_stage()) {
        ret = target_action_data();
    }

    return xbyte_buffer_t{ret.data(), ret.data() + ret.size()};
}

std::string xtop_contract_execution_context::source_action_data() const {
    std::string ret;
    switch (m_action->type()) {
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->transaction_source_action_data();
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->transaction_source_action_data();
            break;
        }
        default:
            break;
    }
    return ret;
}

std::string xtop_contract_execution_context::target_action_data() const {
    std::string ret;
    switch (m_action->type()) {
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->transaction_target_action_data();
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->transaction_target_action_data();
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->stage();
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->stage();
            break;
        }
        default:
            break;
    }
    return ret;
}

uint32_t xtop_contract_execution_context::deposit() const {
    uint32_t ret{0};
    switch (m_action->type()) {
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->deposit();
            break;
        }
        case data::xtop_action_type_t::user:{
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->digest_hex();
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->digest_hex();
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->size();
            break;
        }
        case data::xtop_action_type_t::user:{
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->used_tgas();
            break;
        }
        case data::xtop_action_type_t::user:{
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->used_disk();
            break;
        }
        case data::xtop_action_type_t::user:{
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_action_send_tx_lock_tgas();
            break;
        }
        case data::xtop_action_type_t::user:{
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_action_used_deposit();
            break;
        }
        case data::xtop_action_type_t::user:{
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_action_recv_tx_use_send_tx_tgas();
            break;
        }
        case data::xtop_action_type_t::user:{
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
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_action_exec_status();
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->last_action_exec_status();
            break;
        }
        default:
            break;
    }
    return ret;
}

bool xtop_contract_execution_context::verify_action(std::error_code & ec) {
    assert(!ec);

    uint64_t last_nonce{0};
    uint64_t nonce{0};
    uint256_t hash{};
    switch (m_action->type()) {
        case data::xtop_action_type_t::system:{
            auto stage = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->stage();
            if (stage != data::xconsensus_action_stage_t::send && stage != data::xconsensus_action_stage_t::self) {
                return true;
            }
            last_nonce = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->last_nonce();
            nonce = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->nonce();
            hash = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->hash();

            break;
        }
        case data::xtop_action_type_t::user:{
            auto stage = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->stage();
            if (stage != data::xconsensus_action_stage_t::send && stage != data::xconsensus_action_stage_t::self) {
                return true;
            }
            last_nonce = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->last_nonce();
            nonce = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->nonce();
            hash = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->hash();
            break;
        }
        default:
            break;
    }

    auto state_nonce = contract_state()->latest_sendtx_nonce(ec);
    top::error::throw_error(ec);
    if (state_nonce != last_nonce) {
        ec = error::xerrc_t::nonce_mismatch;
        return false;
    }

    contract_state()->latest_sendtx_nonce(nonce, ec);
    top::error::throw_error(ec);
    contract_state()->latest_sendtx_hash(hash, ec);
    top::error::throw_error(ec);
    contract_state()->latest_followup_tx_nonce(nonce);
    contract_state()->latest_followup_tx_hash(hash);

    return true;
}

void xtop_contract_execution_context::execute_default_source_action(contract_common::xcontract_execution_fee_t & fee_change) {
    xassert(sender() == contract_state()->state_account_address());

    // step1: service fee
    uint64_t service_fee{0};
#ifndef XENABLE_MOCK_ZEC_STAKE
    if (!data::is_sys_contract_address(sender()) && data::is_beacon_contract_address(recver())) {
        service_fee = XGET_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee);
    }
#endif
    if (service_fee > 0) {
        contract_state()->transfer_internal(data::XPROPERTY_BALANCE_AVAILABLE, data::XPROPERTY_BALANCE_BURN, service_fee);
    }

    // step2: fee
    if (!data::is_sys_contract_address(sender())) {
        std::error_code ec;
        update_tgas_disk_sender(true, fee_change, ec);
        top::error::throw_error(ec);
    }
}

void xtop_contract_execution_context::execute_default_target_action(contract_common::xcontract_execution_fee_t & fee_change) {
    xassert((recver() == m_contract_state->state_account_address()) ||
            (action_stage() == data::xconsensus_action_stage_t::self && data::is_black_hole_address(common::xaccount_address_t{recver()})));
    // step1: fee
    // target_fee_exec();

    // step2: action exec
    if (action_stage() == data::xconsensus_action_stage_t::self && data::is_black_hole_address(common::xaccount_address_t{recver()})) {
        return;
    }
    if (!data::is_sys_contract_address(sender())) {
        // ret = m_fee.update_tgas_disk_after_sc_exec(trace);
        // xdbg("[target_action_exec] gas: %u, disk: %u, tx_hash: %s, source: %s, target: %s",
        //      trace->m_tgas_usage,
        //      trace->m_disk_usage,
        //      m_trans->get_digest_hex_str().c_str(),
        //      m_trans->get_source_addr().c_str(),
        //      m_trans->get_target_addr().c_str());
        assert(0);
    } else {
        auto ret = fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::send_tx_lock_tgas, last_action_send_tx_lock_tgas()));
        assert(ret.second == true);
        ret = fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::recv_tx_use_send_tx_tgas, 0));
        assert(ret.second == true);
        ret = fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_deposit, last_action_used_deposit()));
        assert(ret.second == true);
        xdbg("tgas_disk tx hash: %s, lock_tgas: %u, use_send_tx_tgas: %u, used_deposit: %u", digest_hex().c_str(), last_action_send_tx_lock_tgas(), 0, last_action_used_deposit());
    }
}

void xtop_contract_execution_context::execute_default_confirm_action(contract_common::xcontract_execution_fee_t & fee_change) {
    xassert(sender() == contract_state()->state_account_address());
    if (action_stage() == data::xconsensus_action_stage_t::self) {
        return;
    }
    if (data::is_sys_contract_address(sender())) {
        return;
    }

    auto lock_tgas = last_action_send_tx_lock_tgas();
    auto used_deposit = last_action_used_deposit();
    auto target_used_tgas = last_action_recv_tx_use_send_tx_tgas();
    auto status = last_action_exec_status();
    auto tx_deposit = deposit();
    xdbg("tgas_disk tx hash: %s, deposit: %u, recv_tx_use_send_tx_tgas: %llu, used_deposit: %u, lock_tgas: %u, status: %d",
         digest_hex().c_str(),
         tx_deposit,
         target_used_tgas,
         used_deposit,
         lock_tgas,
         status);

    if (lock_tgas > 0) {
        auto cur_lock_tgas = contract_state()->lock_tgas();
        assert(cur_lock_tgas > lock_tgas);
        contract_state()->lock_tgas(cur_lock_tgas - lock_tgas);
    }

    if (tx_deposit > 0) {
        contract_state()->transfer_internal(data::XPROPERTY_BALANCE_LOCK, data::XPROPERTY_BALANCE_AVAILABLE, tx_deposit);
    }

    std::error_code ec;
    calc_resource(target_used_tgas, tx_deposit, target_used_tgas, ec);
    top::error::throw_error(ec);
    {
        auto ret = fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_tgas, target_used_tgas));
        assert(ret.second == true);
        ret = fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_deposit, used_deposit));
        assert(ret.second == true);
    }
}

void xtop_contract_execution_context::calc_resource(uint32_t & tgas, uint32_t deposit, uint32_t & used_deposit, std::error_code & ec) {
    auto used_tgas = calc_decayed_tgas();
    auto available_tgas = calc_available_tgas();
    uint64_t set_used_tgas{0};
    xdbg("tgas_disk used_tgas: %llu, available_tgas: %llu, tgas: %llu, deposit: %u, used_deposit: %u", used_tgas, available_tgas, tgas, deposit, used_deposit);
    if (tgas > (available_tgas + (deposit - used_deposit) / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio))) {
        xdbg("tgas_disk xtransaction_not_enough_pledge_token_tgas");
        set_used_tgas = available_tgas + used_tgas;
        tgas = available_tgas - used_tgas;
        used_deposit = deposit;
        ec = error::xenum_errc::tx_not_enough_pledge_token_tgas;;
    } else if (tgas > available_tgas) {
        used_deposit += (tgas - available_tgas) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
        set_used_tgas = available_tgas + used_tgas;
    } else {
        set_used_tgas = used_tgas + tgas;
    }
    contract_state()->used_tgas(set_used_tgas, ec);
    contract_state()->last_tx_hour(contract_state()->time(), ec);

    if (used_deposit > 0) {
        xdbg("xaccount_context_t::calc_resource balance withdraw used_deposit=%u", used_deposit);
        contract_state()->transfer_internal(data::XPROPERTY_BALANCE_AVAILABLE, data::XPROPERTY_BALANCE_BURN, used_deposit);
    }
}

void xtop_contract_execution_context::update_tgas_disk_sender(bool is_contract, contract_common::xcontract_execution_fee_t & fee_change, std::error_code & ec) {
    auto const tx_deposit = deposit();
    xdbg("tgas_disk deposit: %d, is_contract: %d", tx_deposit, is_contract);

    if (!data::is_sys_contract_address(sender())) {
        if (tx_deposit < XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit)) {
            xdbg("not_enough_deposit, source addr: %s, target addr: %s, deposit: %d", sender().value().c_str(), recver().value().c_str(), tx_deposit);
            return;  // xtransaction_not_enough_deposit;
        }
    }

    auto tgas_usage = calc_cost_tgas(is_contract);
    auto disk_usage = calc_cost_disk(is_contract);

    // step1: used tgas
    uint64_t tgas_deposit_usage{0};
    calc_used_tgas(tx_deposit, tgas_usage, tgas_deposit_usage, ec);
    if (ec) {
        return;
    }
    incr_used_tgas(tgas_usage, ec);
    if (ec) {
        return;
    }
    {
        auto ret = fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_tgas, tgas_usage - tgas_deposit_usage / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio)));
        assert(ret.second == true);
        ret = fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_deposit, tgas_deposit_usage));
        assert(ret.second == true);
    }
    xdbg("tgas_disk m_used_tgas %d, %d, %s, %s", contract_state()->used_tgas(), tgas_deposit_usage, sender().value().c_str(), digest_hex().c_str());

    // step2: disk
    contract_state()->disk(disk_usage);
    {
        auto ret = fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::used_disk, disk_usage));
        assert(ret.second == true);
    }
    xdbg("tgas_disk m_used_disk %d, %s, %s", used_disk(), sender().value().c_str(), digest_hex().c_str());

    // step3: frozen tgas
    auto available_tgas = calc_available_tgas();
    uint64_t frozen_tgas =
        std::min((tx_deposit - tgas_deposit_usage) / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio), available_tgas - contract_state()->lock_tgas());
    xdbg("tgas_disk sender frozen_tgas: %llu, available_tgas: %llu, lock_tgas: %llu", frozen_tgas, available_tgas, contract_state()->lock_tgas());

    if(is_contract && frozen_tgas > 0){
        auto ret = fee_change.insert(std::make_pair(contract_common::xcontract_execution_fee_option_t::send_tx_lock_tgas, frozen_tgas));
        assert(ret.second == true);
        contract_state()->lock_tgas(frozen_tgas);
        xdbg("tgas_disk tx hash: %s, frozen_tgas: %u", digest_hex().c_str(), frozen_tgas);
    }

    // step4: tranfer deposit
    if (tx_deposit > 0) {
        contract_state()->transfer_internal(data::XPROPERTY_BALANCE_AVAILABLE, data::XPROPERTY_BALANCE_LOCK, tx_deposit);
    }
    xdbg("tgas_disk tx hash: %s, deposit: %u", digest_hex().c_str(), tx_deposit);
}

void xtop_contract_execution_context::calc_used_tgas(uint32_t deposit, uint32_t & cur_tgas_usage, uint64_t & deposit_usage, std::error_code & ec) const {
    auto last_hour = contract_state()->last_tx_hour();
    // xdbg("tgas_disk last_hour: %d, m_timer_height: %d, no decay used_tgas: %d, used_tgas: %d, pledge_token: %d, token_price: %u, total_tgas: %d, tgas_usage: %d, deposit: %d",
    //       last_hour, m_timer_height, exectx->used_tgas(), calc_decayed_tgas(), m_account->tgas_balance(), get_token_price(), get_total_tgas(), cur_tgas_usage, deposit);

    auto available_tgas = calc_available_tgas();
    xdbg("tgas_disk account: %s, total tgas usage adding this tx : %d", contract_state()->state_account_address().to_string().c_str(), cur_tgas_usage);
    if (cur_tgas_usage > (available_tgas + deposit / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio))) {
        xdbg("tgas_disk xtransaction_not_enough_pledge_token_tgas");
        deposit_usage = deposit;
        cur_tgas_usage = available_tgas;
        ec = error::xenum_errc::tx_not_enough_pledge_token_tgas;
    } else if (cur_tgas_usage > available_tgas) {
        deposit_usage = (cur_tgas_usage - available_tgas) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
        cur_tgas_usage = available_tgas;
        xdbg("tgas_disk tx deposit_usage: %d", deposit_usage);
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
    if (total_asset >= XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset)) {
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
    if (height < decay_time + last_hour) {
        decayed_tgas = (decay_time - (height - last_hour)) * contract_state()->used_tgas() / decay_time;
    }
    return decayed_tgas;
}

uint32_t xtop_contract_execution_context::calc_cost_tgas(bool is_contract) const {
    #ifdef ENABLE_SCALE
        uint16_t amplify = 5;
    #else
        uint16_t amplify = 1;
    #endif
    if(is_contract) {
        amplify = 1;
    }
    uint32_t multiple = (action_stage() == data::xconsensus_action_stage_t::self) ? 1 : 3;
    return multiple * amplify * size();
}

uint32_t xtop_contract_execution_context::calc_cost_disk(bool is_contract) const {
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
    contract_state()->used_tgas(num + calc_decayed_tgas(), ec);
    contract_state()->last_tx_hour(contract_state()->time(), ec);
}

uint64_t xtop_contract_execution_context::calc_token_price() const {
    auto initial_total_pledge_token = XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token);
    auto total_lock_tgas = contract_state()->system_lock_tgas();
    auto total_pledge_token = total_lock_tgas + initial_total_pledge_token;
    auto token_price = XGET_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard) * XGET_CONFIG(validator_group_count) * TOP_UNIT / total_pledge_token;
    xdbg("tgas_disk get total pledge token from beacon: %llu + %llu = %llu, price: %llu", initial_total_pledge_token, total_lock_tgas, total_pledge_token, token_price);
    return token_price;
}

NS_END2
