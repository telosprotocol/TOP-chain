// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_runtime/xtransaction_execution_result.h"
#include "xevm/xerror/xerror.h"

#include <string>
#include <system_error>
#include <vector>

NS_BEG2(top, evm)

struct xtop_evm_execution_status {
    std::error_code ec{error::xerrc_t::ok};
    std::string extra_msg{};
};
using xevm_execution_status_t = xtop_evm_execution_status;

struct xtop_evm_execution_result {
    xevm_execution_status_t status;
    std::string binlog;
    std::string bincode;
    std::vector<contract_runtime::xtransaction_execution_result_t> transaction_results;
};
using xevm_execution_result_t = xtop_evm_execution_result;

struct xtop_evm_output {
    xevm_execution_status_t status;
    std::string binlog;
    std::string bincode;
    std::vector<data::xcons_transaction_ptr_t> success_tx_assemble;
    std::vector<data::xcons_transaction_ptr_t> failed_tx_assemble;
    std::vector<data::xcons_transaction_ptr_t> delay_tx_assemble;
};
using xevm_output_t = xtop_evm_output;

NS_END2
