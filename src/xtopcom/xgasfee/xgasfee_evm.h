// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xgasfee/xgas_state_operator.h"
#include "xgasfee/xgas_tx_operator.h"
#include "xtxexecutor/xvm_face.h"

namespace top {
namespace gasfee {

class xtop_gasfee_evm
  : public xgas_state_operator_t
  , public xgas_tx_operator_t {
public:
    xtop_gasfee_evm(std::shared_ptr<data::xunit_bstate_t> const & state, xobject_ptr_t<data::xcons_transaction_t> const & tx, uint64_t time);

public:
    void preprocess(std::error_code & ec);
    void postprocess(const evm_common::u256& gas_used, std::error_code & ec);
    txexecutor::xvm_gasfee_detail_t gasfee_detail() const;

private:
    void init(std::error_code & ec);
    void store_in_one_stage();
    void calculate_min_priority();
    void calculate_gas_fee(const evm_common::u256& gas_used, std::error_code & ec);

private:
    evm_common::u256 m_max_converted_utop{0};
    evm_common::u256 m_priority_fee_price{0};
    uint64_t m_time{0};
    // output
    txexecutor::xvm_gasfee_detail_t m_detail;
};

using xgasfee_evm_t = xtop_gasfee_evm;

}  // namespace gasfee
}  // namespace top
