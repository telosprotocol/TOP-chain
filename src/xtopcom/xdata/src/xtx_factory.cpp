// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xtx_factory.h"

#include "xchain_fork/xutility.h"
#include "xcrypto/xckey.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v3.h"

namespace top { namespace data {

xtransaction_ptr_t xtx_factory::create_tx(const enum_xtransaction_version tx_version) {
    switch (tx_version)
    {
    case xtransaction_version_1:
        return make_object_ptr<xtransaction_v1_t>();
        break;
    case xtransaction_version_2:
        return make_object_ptr<xtransaction_v2_t>();
        break;
    case xtransaction_version_3:
        return make_object_ptr<xtransaction_v3_t>();
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

xtransaction_ptr_t xtx_factory::create_contract_subtx_transfer(const std::string & sender, 
                                                                const std::string & receiver,
                                                                uint64_t latest_sendtx_nonce,
                                                                uint64_t amount,
                                                                uint64_t timestamp) {
    xtransaction_ptr_t tx = data::xtx_factory::create_tx(data::xtransaction_version_2);
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_last_nonce(latest_sendtx_nonce);
    tx->set_different_source_target_address(sender, receiver);
    tx->set_fire_timestamp(timestamp);
    tx->set_expire_duration(0);
    tx->set_deposit(0);
    tx->set_digest();
    tx->set_len();
    return tx;
}

xtransaction_ptr_t xtx_factory::create_contract_subtx_call_contract(const std::string & sender, 
                                                                    const std::string & receiver,
                                                                    uint64_t latest_sendtx_nonce,
                                                                    const std::string& func_name, 
                                                                    const std::string& func_param,
                                                                    uint64_t timestamp) {
    xtransaction_ptr_t tx = data::xtx_factory::create_tx(data::xtransaction_version_2);
    data::xproperty_asset asset(0);
    tx->make_tx_run_contract(asset, func_name, func_param);
    tx->set_last_nonce(latest_sendtx_nonce);
    tx->set_different_source_target_address(sender, receiver);
    tx->set_fire_timestamp(timestamp);
    tx->set_expire_duration(0);
    tx->set_deposit(0);
    tx->set_digest();
    tx->set_len();
    return tx;
}

xtransaction_ptr_t xtx_factory::create_sys_contract_call_self_tx(const std::string & account, 
                                                                 const uint64_t latest_sendtx_nonce,
                                                                 const std::string& func_name, 
                                                                 const std::string& func_param,
                                                                 const uint64_t timestamp,
                                                                 const uint16_t expire_duration) {
    xtransaction_ptr_t tx = data::xtx_factory::create_tx(data::xtransaction_version_2);
    data::xproperty_asset asset(0);
    tx->make_tx_run_contract(asset, func_name, func_param);
    tx->set_last_nonce(latest_sendtx_nonce);
    tx->set_different_source_target_address(account, account);
    tx->set_fire_timestamp(timestamp);
    tx->set_expire_duration(expire_duration);
    tx->set_deposit(0);
    tx->set_digest();
    tx->set_len();
    return tx;
}

xtransaction_ptr_t xtx_factory::create_nodejoin_tx(const std::string & sender, 
                                                   const uint64_t & latest_sendtx_nonce,
                                                   const std::string & func_param,
                                                   const uint32_t deposit) {
    xtransaction_ptr_t tx = data::xtx_factory::create_tx(data::xtransaction_version_2);
    tx->make_tx_run_contract("nodeJoinNetwork2", func_param);
    tx->set_last_nonce(latest_sendtx_nonce);
    tx->set_different_source_target_address(sender, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    tx->set_deposit(deposit);
    tx->set_digest();

    return tx;
}

xtransaction_ptr_t xtx_factory::create_ethcall_v3_tx(const std::string & from, const std::string & to, const std::string & data, const top::evm_common::u256 & value, 
                                                     const top::evm_common::u256 & gas, evm_common::u256 const& _maxGasPrice, evm_common::u256 const& _maxPriorityFee) {
    std::error_code ec;
    common::xaccount_address_t _top_from_addr(from);
    common::xeth_address_t _eth_from_addr = common::xeth_address_t::build_from(_top_from_addr, ec);

    common::xeth_address_t _eth_to_addr;
    if (!to.empty()) {
        common::xaccount_address_t _top_to_addr(to);
        _eth_to_addr = common::xeth_address_t::build_from(_top_to_addr, ec);
    }

    xbytes_t _data_bs = top::to_bytes(data);

    xeth_transaction_t ethtx(_eth_from_addr, _eth_to_addr, _data_bs, value, gas, _maxGasPrice, _maxPriorityFee);
    return create_v3_tx(ethtx);
}

xtransaction_ptr_t xtx_factory::create_v3_tx(xeth_transaction_t const& ethtx) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v3_t>(ethtx);
    return tx;
}

xtransaction_ptr_t xtx_factory::create_v2_run_contract_tx(common::xaccount_address_t const & address,
                                                        const uint64_t & latest_sendtx_nonce,
                                                        std::string const & action_name,
                                                        std::string const & action_params,
                                                        const uint64_t fire_timestamp) {
    auto tx = make_object_ptr<data::xtransaction_v2_t>();
    tx->make_tx_run_contract(action_name, action_params);
    tx->set_same_source_target_address(address.to_string());
    tx->set_last_nonce(latest_sendtx_nonce);
    tx->set_fire_timestamp(fire_timestamp);
    tx->set_expire_duration(300);
    tx->set_digest();
    tx->set_len();
    return tx;
}

xtransaction_ptr_t xtx_factory::create_v2_run_contract_tx(common::xaccount_address_t const & source_address,
                                                        common::xaccount_address_t const & target_address,
                                                        const uint64_t & latest_sendtx_nonce,
                                                        std::string const & action_name,
                                                        std::string const & action_params,
                                                        const uint64_t fire_timestamp) {
    auto tx = make_object_ptr<data::xtransaction_v2_t>();
    tx->make_tx_run_contract(action_name, action_params);
    tx->set_different_source_target_address(source_address.to_string(), target_address.to_string());
    tx->set_last_nonce(latest_sendtx_nonce);
    tx->set_fire_timestamp(fire_timestamp);
    tx->set_expire_duration(300);
    tx->set_digest();
    tx->set_len();
    return tx;
}

}  // namespace data
}  // namespace top
