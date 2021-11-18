// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xtransaction_v1.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtx_factory.h"

namespace top { namespace data {

xtransaction_ptr_t xtx_factory::create_tx(const enum_xtransaction_version tx_version) {
    switch (tx_version)
    {
    case xtransaction_version_1:
        return make_object_ptr<xtransaction_v1_t>();
        break;
    
    default:
        return make_object_ptr<xtransaction_v2_t>();
        break;
    }
}

xtransaction_ptr_t xtx_factory::create_genesis_tx_with_balance(const std::string & account, int64_t top_balance) {
    // genesis tx should use v1 tx
    xtransaction_ptr_t tx = xtx_factory::create_tx(xtransaction_version_1);
    data::xproperty_asset asset(top_balance);
    tx->make_tx_transfer(asset);
    tx->set_same_source_target_address(account);
    tx->set_fire_timestamp(0);
    tx->set_expire_duration(0);
    tx->set_deposit(0);
    tx->set_digest();
    tx->set_len();
    return tx;
}

// XTODO xtop_contract_manager::setup_chain use this api to create genesis tx
xtransaction_ptr_t xtx_factory::create_genesis_tx_with_sys_contract(const std::string & account) {
    // genesis tx should use v1 tx
    xtransaction_ptr_t tx = xtx_factory::create_tx(xtransaction_version_1);
    data::xproperty_asset asset_out{0};
    tx->make_tx_run_contract(asset_out, "setup", "");
    tx->set_same_source_target_address(account);
    tx->set_digest();
    tx->set_len();
    return tx;
}



}  // namespace data
}  // namespace top
