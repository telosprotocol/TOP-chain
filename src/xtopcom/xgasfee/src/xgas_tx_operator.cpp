// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgasfee/xgas_tx_operator.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xgenesis_data.h"
#include "xgasfee/xerror/xerror.h"

#include <stdint.h>

namespace top {
namespace gasfee {

xtop_gas_tx_operator::xtop_gas_tx_operator(xobject_ptr_t<data::xcons_transaction_t> const & tx) : m_tx(tx) {
}

common::xaccount_address_t xtop_gas_tx_operator::sender() const {
    return common::xaccount_address_t{m_tx->get_source_addr()};
}

common::xaccount_address_t xtop_gas_tx_operator::recver() const {
    return common::xaccount_address_t{m_tx->get_target_addr()};
}

uint64_t xtop_gas_tx_operator::deposit() const {
    return m_tx->get_transaction()->get_deposit();
}

data::enum_xtransaction_type xtop_gas_tx_operator::tx_type() const {
    return static_cast<data::enum_xtransaction_type>(m_tx->get_tx_type());
}

base::enum_transaction_subtype xtop_gas_tx_operator::tx_subtype() const {
    return m_tx->get_tx_subtype();
}

void xtop_gas_tx_operator::tx_set_used_tgas(const uint64_t tgas) {
    m_tx->set_current_used_tgas(tgas);
}

void xtop_gas_tx_operator::tx_set_used_deposit(const uint64_t deposit) {
    m_tx->set_current_used_deposit(deposit);
}

void xtop_gas_tx_operator::tx_set_current_recv_tx_use_send_tx_tgas(const uint64_t tgas) {
    m_tx->set_current_recv_tx_use_send_tx_tgas(tgas);
}

uint64_t xtop_gas_tx_operator::tx_used_tgas() const {
    return m_tx->get_current_used_tgas();
}

uint64_t xtop_gas_tx_operator::tx_last_action_used_deposit() const {
    return m_tx->get_last_action_used_deposit();
}

uint64_t xtop_gas_tx_operator::tx_last_action_recv_tx_use_send_tx_tgas() const {
    return m_tx->get_last_action_recv_tx_use_send_tx_tgas();
}

data::enum_xtransaction_version xtop_gas_tx_operator::tx_version() const {
    return static_cast<data::enum_xtransaction_version>(m_tx->get_transaction()->get_tx_version());
}

evm_common::u256 xtop_gas_tx_operator::tx_gas_limit() const {
    return m_tx->get_transaction()->get_gaslimit();
}

evm_common::u256 xtop_gas_tx_operator::tx_eth_fee_per_gas() const {
    return m_tx->get_transaction()->get_max_fee_per_gas();
}

evm_common::u256 xtop_gas_tx_operator::tx_top_fee_per_gas() const {
    // 1 Ether = 1* 10 ^18 Wei = 3*10^12 Utop; 1Gwei = 1* 10 ^9 Wei = 3*10^3 Utop
    return tx_eth_fee_per_gas() / static_cast<evm_common::u256>(std::pow(10, 6) * 3);
}

evm_common::u256 xtop_gas_tx_operator::tx_limited_tgas() const {
    return tx_top_fee_per_gas() * tx_gas_limit();
}

uint64_t xtop_gas_tx_operator::tx_fixed_tgas() const {
    uint64_t fixed_tgas{0};
#ifndef XENABLE_MOCK_ZEC_STAKE
    if (!data::is_sys_contract_address(sender()) && data::is_beacon_contract_address(recver())) {
        fixed_tgas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee);
    }
#endif
    return fixed_tgas;
}

uint64_t xtop_gas_tx_operator::tx_bandwith_tgas() const {
#ifdef ENABLE_SCALE
    uint16_t amplify = 5;
#else
    uint16_t amplify = 1;
#endif
    if (tx_type() != data::xtransaction_type_transfer) {
        amplify = 1;
    }
    uint32_t multiple = (m_tx->is_self_tx()) ? 1 : 3;
    return multiple * amplify * m_tx->get_transaction()->get_tx_len();
}

uint64_t xtop_gas_tx_operator::tx_disk_tgas() const {
    return 0;
}

}  // namespace gasfee
}  // namespace top