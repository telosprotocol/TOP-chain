// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgasfee/xgas_tx_operator.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xgasfee/xerror/xerror.h"
#include "xconfig/xutility.h"
#include "xchain_fork/xutility.h"

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

std::string xtop_gas_tx_operator::sender_str() const {
    return m_tx->get_source_addr();
}

std::string xtop_gas_tx_operator::recver_str() const {
    return m_tx->get_target_addr();
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

uint64_t xtop_gas_tx_operator::tx_last_action_used_deposit() const {
    return m_tx->get_last_action_used_deposit();
}

data::enum_xtransaction_version xtop_gas_tx_operator::tx_version() const {
    return static_cast<data::enum_xtransaction_version>(m_tx->get_transaction()->get_tx_version());
}

evm_common::u256 xtop_gas_tx_operator::tx_eth_gas_limit() const {
    return m_tx->get_transaction()->get_gaslimit();
}

evm_common::u256 xtop_gas_tx_operator::tx_eth_fee_per_gas() const {
    return m_tx->get_transaction()->get_max_fee_per_gas();
}

evm_common::u256 xtop_gas_tx_operator::tx_eth_priority_fee_per_gas() const {
    return m_tx->get_transaction()->get_max_priority_fee_per_gas();
}

evm_common::u256 xtop_gas_tx_operator::tx_eth_limited_gasfee(uint64_t forked_time) const {
    // 1Gwei = (ratio / 10^3)Utop
    // 1Utop = (10^3 / ratio)Gwei
    evm_common::u256 limit = tx_eth_gas_limit();
    evm_common::u256 price = tx_eth_fee_per_gas();
    evm_common::u256 wei_gasfee = limit * price;
    evm_common::u256 utop_gasfee = wei_to_utop(wei_gasfee, forked_time);
    xdbg("[xtop_gas_tx_operator::tx_eth_limited_gasfee] eth_gas_price: %s, eth_gas_limit: %s, wei_gasfee: %s, utop_gasfee: %s",
         price.str().c_str(),
         limit.str().c_str(),
         wei_gasfee.str().c_str(),
         utop_gasfee.str().c_str());
    return utop_gasfee;
}

evm_common::u256 xtop_gas_tx_operator::wei_to_utop(const evm_common::u256 wei, uint64_t forked_time) {
    evm_common::u256 utop{0};

    if (chain_fork::xutility_t::is_forked(fork_points::v1_10_priority_fee_update_point, forked_time)) {
        utop = wei * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_to_top_exchange_ratio) / evm_common::u256(1000000000000ULL);
    } else {
        evm_common::u256 gwei = wei / evm_common::u256(1000000000ULL);
        evm_common::u256 mtop = gwei * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_to_top_exchange_ratio);
        utop = mtop / 1000U;
    }

    xdbg("[xtop_gas_tx_operator::wei_to_utop] exchange ratio: %lu, wei: %s, utop: %s",
        XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_to_top_exchange_ratio),
        wei.str().c_str(),
        utop.str().c_str());
    return utop;
}

evm_common::u256 xtop_gas_tx_operator::utop_to_wei(const evm_common::u256 utop) {
    auto wei = utop * evm_common::u256(1000000000000ULL) / XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_to_top_exchange_ratio);
    xdbg("[xtop_gas_tx_operator::utop_to_wei] exchange ratio: %lu, utop: %s, wei: %s",
         XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_to_top_exchange_ratio),
         utop.str().c_str(),
         wei.str().c_str());
    return wei;
}

evm_common::u256 xtop_gas_tx_operator::tx_fixed_tgas() const {
    evm_common::u256 fixed_tgas{0};
#ifndef XENABLE_MOCK_ZEC_STAKE
    if (recver_str().empty()) {
        return 0;
    }
    if (!data::is_sys_contract_address(sender()) && data::is_beacon_contract_address(recver())) {
        fixed_tgas = balance_to_tgas(XGET_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee));
    }
#endif
    return fixed_tgas;
}

evm_common::u256 xtop_gas_tx_operator::tx_bandwith_tgas() const {
#ifdef ENABLE_SCALE
    uint16_t amplify = 5;
#else
    uint16_t amplify = 1;
#endif
    if (tx_type() != data::xtransaction_type_transfer) {
        amplify = 1;
    }
    evm_common::u256 multiple{3};
    evm_common::u256 bandwith_tgas = multiple * amplify * m_tx->get_transaction()->get_tx_len();
    return bandwith_tgas;
}

evm_common::u256 xtop_gas_tx_operator::tx_disk_tgas() const {
    if (tx_type() == data::xtransaction_type_transfer) {
        return 0;
    }
    evm_common::u256 multiple{1};
    // evm deploy tx
    if (recver_str().empty() || recver() == eth_zero_address) {
        multiple = 1200000UL;
    }
    evm_common::u256 disk_tgas = multiple * m_tx->get_transaction()->get_tx_len();
    return disk_tgas;
}

bool xtop_gas_tx_operator::is_one_stage_tx() const {
    return (m_tx->is_self_tx() || m_tx->get_inner_table_flag());
}

evm_common::u256 xtop_gas_tx_operator::balance_to_tgas(const evm_common::u256 balance) {
    xassert(XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio) > 0);
    return balance / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
}

evm_common::u256 xtop_gas_tx_operator::tgas_to_balance(const evm_common::u256 tgas) {
    return tgas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
}

}  // namespace gasfee
}  // namespace top
