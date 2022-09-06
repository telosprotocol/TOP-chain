// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xgasfee/xgasfee_interface.h"
#include <iostream>

namespace top {
namespace gasfee {

class xconsortium_gasfee : public xgasfee_interface {
public:
    xconsortium_gasfee(std::shared_ptr<data::xunit_bstate_t> const & state, xobject_ptr_t<data::xcons_transaction_t> const & tx, uint64_t time, uint64_t onchain_tgas_deposit)
      : xgasfee_interface(state, tx, time, onchain_tgas_deposit) {
    }

public:
    void preprocess(std::error_code & ec) {}
    void postprocess(const evm_common::u256 supplement_gas, std::error_code & ec) {}
    evm_common::u256 get_tx_eth_gas_limit() const { return 0;}
};

}  // namespace gasfee
}  // namespace top
