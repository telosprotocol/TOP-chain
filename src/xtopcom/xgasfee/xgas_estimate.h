// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xvmethod.h"
#include "xdata/xunit_bstate.h"
#include "xgasfee/xgas_tx_operator.h"

namespace top {
namespace gasfee {

class xgas_estimate {
public:
    static evm_common::u256 base_price();
    static evm_common::u256 flexible_price(const xobject_ptr_t<data::xcons_transaction_t> tx, const evm_common::u256 evm_gas);
    static evm_common::u256 estimate_used_gas(const xobject_ptr_t<data::xcons_transaction_t> tx, const evm_common::u256 evm_gas);
};

}  // namespace gasfee
}  // namespace top
