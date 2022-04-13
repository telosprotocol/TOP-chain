// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_action.h"

NS_BEG2(top, data)

xtop_consensus_action<xtop_action_type_t::evm>::xtop_consensus_action(xobject_ptr_t<data::xcons_transaction_t> const & tx) noexcept
  : xtop_action_t<xtop_action_type_t::evm>{tx,
                                           tx->is_send_tx() ?
                                               static_cast<common::xlogic_time_t>((tx->get_transaction()->get_fire_timestamp() + tx->get_transaction()->get_expire_duration() +
                                                                                   XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance)) /
                                                                                  XGLOBAL_TIMER_INTERVAL_IN_SECONDS) :
                                               common::xjudgement_day} {
    // todo
    // get action_type/sender/recever/gas/value/data.... from tx

    common::xaccount_address_t const target_address{tx->get_transaction()->get_target_address()};
    if (target_address.empty()) {
        // deploy_contract
        m_action_type = xtop_evm_action_type::deploy_contract;
    } else {
        m_action_type = xtop_evm_action_type::call_contract;
    }

    // m_data tx->get_transaction()->get_ext()
}

NS_END2