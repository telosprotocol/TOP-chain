// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xgasfee/xgas_state_operator.h"
#include "xgasfee/xgas_tx_operator.h"

#include <iostream>

namespace top {
namespace gasfee {

class xtop_gasfee : public xgas_state_operator_t, public xgas_tx_operator_t {
public:
    xtop_gasfee(std::shared_ptr<data::xunit_bstate_t> const & state, xobject_ptr_t<data::xcons_transaction_t> const & tx, uint64_t time, uint64_t onchain_tgas_deposit);

public:
    void preprocess(std::error_code & ec);
    void postprocess(const uint64_t supplement_gas, std::error_code & ec);

private:
    void preprocess_one_stage(std::error_code & ec);
    void preprocess_send_stage(std::error_code & ec);
    void preprocess_recv_stage(std::error_code & ec);
    void preprocess_confirm_stage(std::error_code & ec);

    void postprocess_one_stage(const uint64_t supplement_gas, std::error_code & ec);
    void postprocess_send_stage(const uint64_t supplement_gas, std::error_code & ec);
    void postprocess_recv_stage(const uint64_t supplement_gas, std::error_code & ec);
    void postprocess_confirm_stage(const uint64_t supplement_gas, std::error_code & ec);

    void store_in_one_stage(std::error_code & ec);
    void store_in_send_stage(std::error_code & ec);
    void store_in_recv_stage(std::error_code & ec);
    void store_in_confirm_stage(std::error_code & ec);

    void init(std::error_code & ec);
    void add(const uint64_t tgas, std::error_code & ec);
    void calculate(std::error_code & ec);

    void process_fixed_tgas(std::error_code & ec);
    void process_bandwith_tgas(std::error_code & ec);
    void process_disk_tgas(std::error_code & ec);
    void process_calculation_tgas(const uint64_t supplement_gas, std::error_code & ec);

    uint64_t balance_to_tgas(const uint64_t balance) const;
    uint64_t tgas_to_balance(const uint64_t tgas) const;

    // tgas related param
    uint64_t m_free_tgas{0};
    uint64_t m_free_tgas_usage{0};
    uint64_t m_deducted_free_tgas_usage{0};
    uint64_t m_converted_tgas{0};
    uint64_t m_converted_tgas_usage{0};
    uint64_t m_deducted_converted_tgas_usage{0};

    // onchain related param
    uint64_t m_time{0};
    uint64_t m_onchain_tgas_deposit{0};
};

using xgasfee_t = xtop_gasfee;

}  // namespace gasfee
}  // namespace top
