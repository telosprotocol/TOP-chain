// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xconsensus_action.h"

NS_BEG2(top, data)

xtop_consensus_action<xtop_action_type_t::evm>::xtop_consensus_action(common::xaccount_address_t src_address,
                                                                      common::xaccount_address_t dst_address,
                                                                      evm_common::u256 value,
                                                                      xbytes_t data,
                                                                      uint64_t gaslimit)
  : xtop_action_t<xtop_action_type_t::evm>{nullptr, common::xjudgement_day}
  , m_sender{std::move(src_address)}
  , m_recver{std::move(dst_address)}
  , m_value{std::move(value)}
  , m_input_data{std::move(data)}
  , m_evm_action_type{(m_recver.empty() || m_recver == eth_zero_address) ? xevm_action_type_t::deploy_contract : xevm_action_type_t::call_contract}
  , m_gaslimit{gaslimit} {
}

xtop_consensus_action<xtop_action_type_t::evm>::xtop_consensus_action(xobject_ptr_t<data::xcons_transaction_t> const & tx)
  : xtop_action_t<xtop_action_type_t::evm> {
        tx,
        tx->is_send_tx() ? static_cast<common::xlogic_time_t>((tx->get_transaction()->get_fire_timestamp() + tx->get_transaction()->get_expire_duration() + XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance)) / XGLOBAL_TIMER_INTERVAL_IN_SECONDS)
                         : common::xjudgement_day
    } {
    m_sender = common::xaccount_address_t{tx->get_source_addr()};
    if (m_sender.empty()) {
        m_sender = eth_zero_address;
    }
    m_recver = common::xaccount_address_t{tx->get_target_addr()};
    m_value = tx->get_transaction()->get_amount_256();

    if (m_recver.empty() || m_recver == eth_zero_address) {
        m_evm_action_type = xtop_evm_action_type::deploy_contract;
    } else {
        m_evm_action_type = xtop_evm_action_type::call_contract;
    }
    m_input_data = top::to_bytes(tx->get_transaction()->get_data());
    m_gaslimit = (uint64_t)tx->get_transaction()->get_gaslimit();
}

xtop_evm_action_type xtop_consensus_action<xtop_action_type_t::evm>::evm_action_type() const noexcept {
    return m_evm_action_type;
}

common::xaccount_address_t const & xtop_consensus_action<xtop_action_type_t::evm>::sender() const noexcept {
    return m_sender;
}

common::xaccount_address_t const & xtop_consensus_action<xtop_action_type_t::evm>::recver() const noexcept {
    return m_recver;
}

xbytes_t const & xtop_consensus_action<xtop_action_type_t::evm>::data() const noexcept {
    return m_input_data;
}

evm_common::u256 const & xtop_consensus_action<xtop_action_type_t::evm>::value() const noexcept {
    return m_value;
}

uint64_t xtop_consensus_action<xtop_action_type_t::evm>::gas_limit() const noexcept {
    return m_gaslimit;
}

NS_END2
