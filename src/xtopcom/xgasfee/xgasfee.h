// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xgasfee/xgasfee_interface.h"
#include <iostream>

namespace top {
namespace gasfee {

class xtop_gasfee : public xgasfee_interface {
public:
    xtop_gasfee(std::shared_ptr<data::xunit_bstate_t> const & state, xobject_ptr_t<data::xcons_transaction_t> const & tx, uint64_t time, uint64_t onchain_tgas_deposit);

public:
    void preprocess(std::error_code & ec);
    void postprocess(const evm_common::u256 supplement_gas, std::error_code & ec);

    evm_common::u256 get_tx_eth_gas_limit() const { return tx_eth_gas_limit();}
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
    evm_common::u256 m_eth_converted_tgas{0};
    evm_common::u256 m_eth_converted_tgas_usage{0};

};

using xgasfee_t = xtop_gasfee;

}  // namespace gasfee
}  // namespace top
