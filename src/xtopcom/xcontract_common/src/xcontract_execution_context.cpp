// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xcontract_execution_context.h"

#include "xbasic/xutility.h"
#include "xcontract_common/xerror/xerror.h"

NS_BEG2(top, contract_common)

//xtop_contract_execution_context::xtop_contract_execution_context(xobject_ptr_t<data::xtransaction_t> tx, observer_ptr<xcontract_state_t> s) noexcept
//  : m_contract_state{s}, m_tx{std::move(tx)} {
//}

xtop_contract_execution_context::xtop_contract_execution_context(std::unique_ptr<data::xbasic_top_action_t const> action,
                                                                 observer_ptr<xcontract_state_t> s) noexcept
  : m_contract_state{s}, m_action{std::move(action)} {
}

xtop_contract_execution_context::xtop_contract_execution_context(std::unique_ptr<data::xbasic_top_action_t const> action,
                                                                 observer_ptr<xcontract_state_t> s,
                                                                 xcontract_execution_param_t param) noexcept
  : m_contract_state{s}, m_action{std::move(action)} {
}

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

xcontract_execution_result_t xtop_contract_execution_context::execution_result() const noexcept {
    return m_execution_result;
}

void xtop_contract_execution_context::add_followup_transaction(data::xcons_transaction_ptr_t && tx, xfollowup_transaction_schedule_type_t type) {
    m_execution_result.output.followup_transaction_data.emplace_back(std::move(tx), type);
}

std::vector<xfollowup_transaction_datum_t> xtop_contract_execution_context::followup_transaction() {
    return m_execution_result.output.followup_transaction_data;
}

void xtop_contract_execution_context::receipt_data(std::map<std::string, xbyte_buffer_t> receipt_data) {
    m_receipt_data = std::move(receipt_data);
}

std::map<std::string, xbyte_buffer_t> & xtop_contract_execution_context::receipt_data() noexcept {
    return m_execution_result.output.receipt_data;
}

xbyte_buffer_t const & xtop_contract_execution_context::receipt_data(std::string const & key, std::error_code & ec) const noexcept {
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
    std::string ret;
    switch (m_action->type()) {
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->action_name();
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->action_name();
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
    xbyte_buffer_t ret;
    switch (m_action->type()) {
        case data::xtop_action_type_t::system:{
            ret = static_cast<data::xsystem_consensus_action_t const *>(m_action.get())->action_data();
            break;
        }
        case data::xtop_action_type_t::user:{
            ret = static_cast<data::xuser_consensus_action_t const *>(m_action.get())->action_data();
            break;
        }
        default:
            break;
    }
    return ret;
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

common::xlogic_time_t xtop_contract_execution_context::time() const {
    return contract_state()->time();
}

common::xlogic_time_t xtop_contract_execution_context::timestamp() const {
    return contract_state()->timestamp();
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
    if (contract_state()->latest_sendtx_nonce() != last_nonce) {
        ec = error::xerrc_t::nonce_mismatch;
        return false;
    }

    contract_state()->latest_sendtx_nonce(nonce);
    contract_state()->latest_sendtx_hash(hash);
    contract_state()->latest_followup_tx_nonce(nonce);
    contract_state()->latest_followup_tx_hash(hash);

    return true;
}

NS_END2
