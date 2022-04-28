// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xgasfee/xgas_state_operator.h"
#include "xgasfee/xgas_tx_operator.h"

#include <iostream>

namespace top {
namespace gasfee {

class xtop_gas_operator : public xgas_state_operator_t, public xgas_tx_operator_t {
public:
    xtop_gas_operator(std::shared_ptr<data::xunit_bstate_t> const & state, xobject_ptr_t<data::xcons_transaction_t> const & tx, uint64_t time, uint64_t total_gas_deposit);

public:
    void preprocess_one_stage_tx(std::error_code & ec);
    void postprocess_one_stage_tx(const uint64_t supplement_gas, std::error_code & ec);

    template <base::enum_transaction_subtype Stage>
    void process_three_stage_tx(std::error_code & ec);

private:
    void init(std::error_code & ec);
    void add(const uint64_t tgas, std::error_code & ec);
    void calculate(std::error_code & ec);
    
    template <base::enum_transaction_subtype Stage>
    void store(std::error_code & ec);

    void process_fixed_tgas(std::error_code & ec);
    void process_bandwith_tgas(std::error_code & ec);
    void process_disk_tgas(std::error_code & ec);
    void process_calculation_tgas(const uint64_t supplement_gas, std::error_code & ec);

    bool need_postprocess() const;

    // tgas related param
    uint64_t m_available_tgas{0};
    uint64_t m_max_tgas{0};
    uint64_t m_tgas_usage{0};
    uint64_t m_deposit_usage{0};

    // onchain related param
    uint64_t m_time{0};
    uint64_t m_total_gas_deposit{0};
};

using xgas_operator_t = xtop_gas_operator;

template <>
void xtop_gas_operator::store<base::enum_transaction_subtype_self>(std::error_code & ec);

template <>
void xtop_gas_operator::store<base::enum_transaction_subtype_send>(std::error_code & ec);

template <>
void xtop_gas_operator::store<base::enum_transaction_subtype_recv>(std::error_code & ec);

template <>
void xtop_gas_operator::store<base::enum_transaction_subtype_confirm>(std::error_code & ec);

template <>
void xtop_gas_operator::process_three_stage_tx<base::enum_transaction_subtype_send>(std::error_code & ec);

template <>
void xtop_gas_operator::process_three_stage_tx<base::enum_transaction_subtype_recv>(std::error_code & ec);

template <>
void xtop_gas_operator::process_three_stage_tx<base::enum_transaction_subtype_confirm>(std::error_code & ec);

}  // namespace gasfee
}  // namespace top
