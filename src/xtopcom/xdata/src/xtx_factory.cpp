// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcrypto/xckey.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xdata/xnative_contract_address.h"
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

xtransaction_ptr_t xtx_factory::create_contract_subtx_transfer(const std::string & sender, 
                                                                const std::string & receiver,
                                                                uint64_t latest_sendtx_nonce,
                                                                const uint256_t & latest_sendtx_hash,
                                                                uint64_t amount,
                                                                uint64_t timestamp) {
    xtransaction_ptr_t tx;
    if (top::chain_fork::xtop_chain_fork_config_center::is_tx_forked_by_timestamp(timestamp)) {
        tx = data::xtx_factory::create_tx(data::xtransaction_version_2);
    } else {
        tx = data::xtx_factory::create_tx(data::xtransaction_version_1);
    }

    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_last_trans_hash_and_nonce(latest_sendtx_hash, latest_sendtx_nonce);
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
                                                                    const uint256_t & latest_sendtx_hash,
                                                                    const std::string& func_name, 
                                                                    const std::string& func_param,
                                                                    uint64_t timestamp) {
    xtransaction_ptr_t tx;
    if (top::chain_fork::xtop_chain_fork_config_center::is_tx_forked_by_timestamp(timestamp)) {
        tx = data::xtx_factory::create_tx(data::xtransaction_version_2);
    } else {
        tx = data::xtx_factory::create_tx(data::xtransaction_version_1);
    }
    data::xproperty_asset asset(0);
    tx->make_tx_run_contract(asset, func_name, func_param);
    tx->set_last_trans_hash_and_nonce(latest_sendtx_hash, latest_sendtx_nonce);
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
                                                                 const uint256_t & latest_sendtx_hash,
                                                                 const std::string& func_name, 
                                                                 const std::string& func_param,
                                                                 const uint64_t timestamp,
                                                                 const uint16_t expire_duration) {
    // XTODO if enable RPC_V2, should add fork time future
    xtransaction_ptr_t tx;
    if (top::chain_fork::xtop_chain_fork_config_center::is_tx_forked_by_timestamp(timestamp)) {
        tx = data::xtx_factory::create_tx(data::xtransaction_version_2);
    } else {
        tx = data::xtx_factory::create_tx(data::xtransaction_version_1);
    }
    data::xproperty_asset asset(0);
    tx->make_tx_run_contract(asset, func_name, func_param);
    tx->set_last_trans_hash_and_nonce(latest_sendtx_hash, latest_sendtx_nonce);
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
                                                   const uint64_t & latest_sendtx_hash,
                                                   const std::string & func_param,
                                                   const uint32_t deposit,
                                                   const std::string & sign_key) {
    xtransaction_ptr_t tx;
    auto fire_time = xtransaction_t::get_gmttime_s();
    if (top::chain_fork::xtop_chain_fork_config_center::is_tx_forked_by_timestamp(fire_time)) {
        tx = data::xtx_factory::create_tx(data::xtransaction_version_2);
    } else {
        tx = data::xtx_factory::create_tx(data::xtransaction_version_1);
    }

    tx->make_tx_run_contract("nodeJoinNetwork2", func_param);
    tx->set_last_nonce(latest_sendtx_nonce);
    tx->set_last_hash(latest_sendtx_hash);
    tx->set_different_source_target_address(sender, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    tx->set_deposit(deposit);
    tx->set_digest();

    utl::xecprikey_t pri_key_obj((uint8_t*)sign_key.data());
    utl::xecdsasig_t signature_obj = pri_key_obj.sign(tx->digest());
    auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
    tx->set_authorization(signature);
    tx->set_len();
    return tx;
}

}  // namespace data
}  // namespace top
