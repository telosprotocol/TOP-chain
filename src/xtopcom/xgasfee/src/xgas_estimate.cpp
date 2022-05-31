// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgasfee/xgas_estimate.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"

#include <stdint.h>

namespace top {
namespace gasfee {

static evm_common::u256 base_price() {
    return evm_common::u256{40000000000UL};
}

static evm_common::u256 flexible_price(const xobject_ptr_t<data::xcons_transaction_t> tx, const evm_common::u256 evm_gas) {
    xtop_gas_tx_operator op{tx};
    evm_common::u256 total_tgas = op.tx_bandwith_tgas() + op.tx_disk_tgas() + evm_gas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_gas_to_tgas_exchange_ratio);
    evm_common::u256 total_utop = total_tgas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
    evm_common::u256 total_wei = op.utop_to_wei(total_utop);
    evm_common::u256 flexible_price = total_wei / evm_gas;
    return flexible_price;
}

static evm_common::u256 estimate_used_gas(const xobject_ptr_t<data::xcons_transaction_t> tx, const evm_common::u256 evm_gas) {
    xtop_gas_tx_operator op{tx};
    evm_common::u256 total_tgas = op.tx_bandwith_tgas() + op.tx_disk_tgas() + evm_gas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_gas_to_tgas_exchange_ratio);
    evm_common::u256 total_utop = total_tgas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
    evm_common::u256 total_wei = op.utop_to_wei(total_utop);
    evm_common::u256 estimate_used_gas = total_wei / base_price();
    return estimate_used_gas;
}

}  // namespace gasfee
}  // namespace top