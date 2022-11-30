// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xdata/xunit_bstate.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xtransaction_v2.h"

NS_BEG2(top, data)

// XTODO for test
class xtransaction_maker {
 public:
    static xtransaction_ptr_t make_transfer_tx(const std::string & from, uint64_t last_nonce, const std::string & to,
        uint64_t amount, uint64_t firestamp, uint16_t duration, uint32_t deposit) {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
        data::xproperty_asset asset(amount);
        tx->make_tx_transfer(asset);
        tx->set_last_nonce(last_nonce);
        tx->set_different_source_target_address(from, to);
        tx->set_fire_timestamp(firestamp);
        tx->set_expire_duration(duration);
        tx->set_deposit(deposit);
        tx->set_digest();
        tx->set_len();
        return tx;
    } 

    static xtransaction_ptr_t make_transfer_tx_v1(const std::string & from, uint64_t last_nonce, const std::string & to,
        uint64_t amount, uint64_t firestamp, uint16_t duration, uint32_t deposit) {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
        data::xproperty_asset asset(amount);
        tx->make_tx_transfer(asset);
        tx->set_last_nonce(last_nonce);
        tx->set_different_source_target_address(from, to);
        tx->set_fire_timestamp(firestamp);
        tx->set_expire_duration(duration);
        tx->set_deposit(deposit);
        tx->set_digest();
        tx->set_len();
        return tx;
    } 

    static xtransaction_ptr_t make_run_contract_tx(const std::string & from, uint64_t last_nonce, const std::string & to,
        const std::string& func_name, const std::string& func_param, uint64_t amount, uint64_t firestamp,
        uint16_t duration, uint32_t deposit) {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
        data::xproperty_asset asset(amount);
        tx->make_tx_run_contract(asset, func_name, func_param);
        tx->set_last_nonce(last_nonce);
        tx->set_different_source_target_address(from, to);
        tx->set_fire_timestamp(firestamp);
        tx->set_expire_duration(duration);
        tx->set_deposit(deposit);
        tx->set_digest();
        tx->set_len();
        return tx;
    }
};

NS_END2
