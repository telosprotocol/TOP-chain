// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcommon/xeth_address.h"
#include "xbase/xmem.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xbloom9.h"
#include "xevm_common/xfixed_hash.h"

#include <string>
#include <vector>

NS_BEG2(top, evm_common)

class xevm_log_t {
public:
    xevm_log_t() = default;
    xevm_log_t(xevm_log_t const &) = default;
    xevm_log_t & operator=(xevm_log_t const &) = default;
    xevm_log_t(xevm_log_t &&) = default;
    xevm_log_t & operator=(xevm_log_t &&) = default;
    ~xevm_log_t() = default;

    xevm_log_t(common::xeth_address_t const& _address, xh256s_t const& topics, xbytes_t const& data);
    xevm_log_t(common::xeth_address_t const & address, xh256s_t topics);

    int32_t do_write(base::xstream_t & stream);
    int32_t do_read(base::xstream_t & stream);
    void streamRLP(RLPStream& _s) const;
    void decodeRLP(RLP const& _r, std::error_code & ec);
    xbloom9_t   bloom() const;

    common::xeth_address_t  address;
    xh256s_t                topics;
    xbytes_t                data;
};

using xevm_logs_t = std::vector<xevm_log_t>;

/// same as TransactionStatus in `evm_engine_rs/engine/src/parameters.rs`
// 23.02.22 future update to: `xtvm_engine_rs/tvm-engine/src/types.rs`
enum xevm_transaction_status_t : uint32_t {
    Success = 0,
    Revert = 1,
    /// Execution runs out of gas (runtime).
    OutOfGas = 2,
    /// Not enough fund to start the execution (runtime).
    OutOfFund = 3,
    /// An opcode accesses external information, but the request is off offset limit (runtime).
    OutOfOffset = 4,
    OtherExecuteError = 5,

    Invalid = 32,
    FAILED = UINT32_MAX,
};

class xevm_transaction_result_t {
 public:
    xevm_transaction_result_t(){}
    xevm_transaction_result_t(const xevm_transaction_result_t & evm_transaction_result) {
        used_gas = evm_transaction_result.used_gas;
        status = evm_transaction_result.status;
        extra_msg = evm_transaction_result.extra_msg;
        logs = evm_transaction_result.logs;
    }

    xevm_transaction_result_t & operator = (const xevm_transaction_result_t & evm_transaction_result) {
        used_gas = evm_transaction_result.used_gas;
        status = evm_transaction_result.status;
        extra_msg = evm_transaction_result.extra_msg;
        logs = evm_transaction_result.logs;
        return *this;
    }

public:
    uint64_t used_gas{0}; // todo: calculate used gas to expense
    xevm_transaction_status_t status{Invalid};
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
