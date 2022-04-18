// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xevm_common/xevm_transaction_result.h"

#include <string>
#include <system_error>
#include <vector>

NS_BEG3(top, contract_runtime, evm)

struct xtop_evm_output_status {
    std::error_code ec{error::xerrc_t::ok};
    std::string extra_msg;

    // dump
    std::string dump_info() {
        return "[status: " + ec.message() + ", extra_msg: " + extra_msg + "]";
    }
};
using xevm_output_status_t = xtop_evm_output_status;

struct xtop_evm_output {
    xevm_output_status_t status;
    uint64_t used_gas;

    // only if status(ec == ok) tx_result has meaning.
    // else meaning something we have't consider well
    evm_common::xevm_transaction_result_t tx_result;

    // debug
    std::string dump_info() {
        return status.dump_info() + ",[used_gas: " + std::to_string(used_gas) + "]," + tx_result.dump_info();
    }
};
using xevm_output_t = xtop_evm_output;

NS_END3
