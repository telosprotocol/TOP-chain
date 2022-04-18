// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"

#include <string>
#include <vector>

NS_BEG2(top, evm_common)

class xevm_transaction_result_t {
public:
    struct xevm_log_t {
        std::string address;
        std::vector<xbytes_t> topics;
        xbytes_t data;
    };
    /// same as TransactionStatus in `evm_engine_rs/engine/src/parameters.rs`
    enum xevm_transaction_status_t : uint32_t {
        Success = 0,
        Revert = 1,
        /// Execution runs out of gas (runtime).
        OutOfGas = 2,
        /// Not enough fund to start the execution (runtime).
        OutOfFund = 3,
        /// An opcode accesses external information, but the request is off offset limit (runtime).
        OutOfOffset = 4,
    };

    uint64_t used_gas;
    xevm_transaction_status_t status;
    std::string extra_msg;
    std::vector<xevm_log_t> logs;

    void set_status(uint32_t input) {
        status = static_cast<xevm_transaction_status_t>(input);
    }

    // debug
    std::string dump_info() {
        return "transaction_result[status:" + std::to_string(static_cast<std::underlying_type<xevm_transaction_status_t>::type>(status)) + ", extra_msg:" + extra_msg +
               "], logs.size():" + std::to_string(logs.size());
    }
};

NS_END2