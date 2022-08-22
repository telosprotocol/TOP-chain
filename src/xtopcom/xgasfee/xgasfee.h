// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xgasfee/xgas_state_operator.h"
#include "xgasfee/xgas_tx_operator.h"
#include "xtxexecutor/xvm_face.h"

#include <iostream>

namespace top {
namespace gasfee {

class xtop_gasfee : public xgas_state_operator_t, public xgas_tx_operator_t {
public:
    xtop_gasfee(std::shared_ptr<data::xunit_bstate_t> const & state, xobject_ptr_t<data::xcons_transaction_t> const & tx, uint64_t time, uint64_t onchain_tgas_deposit);

public:
    void preprocess(std::error_code & ec);
    void postprocess(const evm_common::u256 supplement_gas, std::error_code & ec);

    txexecutor::xvm_gasfee_detail_t gasfee_detail() const;

private:
    void preprocess_one_stage(std::error_code & ec);
    void preprocess_send_stage(std::error_code & ec);
    void preprocess_recv_stage(std::error_code & ec);
    void preprocess_confirm_stage(std::error_code & ec);

    void postprocess_one_stage(const evm_common::u256 supplement_gas, std::error_code & ec);
    void postprocess_send_stage(const evm_common::u256 supplement_gas, std::error_code & ec);
    void postprocess_recv_stage(const evm_common::u256 supplement_gas, std::error_code & ec);
    void postprocess_confirm_stage(const evm_common::u256 supplement_gas, std::error_code & ec);

    void store_in_one_stage();
    void store_in_send_stage();
    void store_in_recv_stage();
    void store_in_confirm_stage();

    void check(std::error_code & ec);
    void init(std::error_code & ec);
    void add(const evm_common::u256 tgas, std::error_code & ec);
    void calculate(const evm_common::u256 supplement_gas, std::error_code & ec);

    void process_fixed_tgas(std::error_code & ec);
    void process_bandwith_tgas(std::error_code & ec);
    void process_disk_tgas(std::error_code & ec);
    void process_calculation_tgas(const evm_common::u256 supplement_gas, std::error_code & ec);

    // tgas related param
    evm_common::u256 m_free_tgas{0};
    evm_common::u256 m_free_tgas_usage{0};
    evm_common::u256 m_converted_tgas{0};
    evm_common::u256 m_converted_tgas_usage{0};

    // onchain related param
    uint64_t m_time{0};
    uint64_t m_onchain_tgas_deposit{0};

    // output
    txexecutor::xvm_gasfee_detail_t m_detail;
};

using xgasfee_t = xtop_gasfee;

}  // namespace gasfee
}  // namespace top
