// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xcommon/xaccount_address.h"
#include "xevm_common/common.h"
#include "xevm_common/xborsh.hpp"
#include "xevm_common/xevm_transaction_result.h"
#include "xevm_contract_runtime/xevm_variant_bytes.h"
#include "xevm_runner/proto/proto_precompile.pb.h"

#include <cstdint>
#include <vector>

NS_BEG3(top, contract_runtime, evm)

struct sys_contract_context {
    common::xaccount_address_t address;
    common::xaccount_address_t caller;
    evm_common::u256 apparent_value;

    sys_contract_context(evm_engine::precompile::ContractContext const & proto_context) {
        address = common::xaccount_address_t{xvariant_bytes{proto_context.address().value(), false}.to_hex_string("T60004")};
        caller = common::xaccount_address_t{xvariant_bytes{proto_context.caller().value(), false}.to_hex_string("T60004")};
        evm_common::xBorshDecoder decoder;
        decoder.getInteger(top::to_bytes(proto_context.apparent_value().data()), apparent_value);
    }
};

enum precompile_output_ExitSucceed : uint32_t {
    Stopped = 1,
    Returned = 2,
    Suicided = 3,
};

enum precompile_error : uint32_t {
    Error = 1,
    Revert = 2,
    Fatal = 3,
};

// ref: ~.cargo/git/checkouts/evm-31951a45719dc0d6/07ae445/core/src/error.rs:105
// match: engine-types/src/precompiles.rs
enum precompile_error_ExitError : uint32_t {
    OutOfGas = 1,  // todo add more
};

// ref: ~.cargo/git/checkouts/evm-31951a45719dc0d6/07ae445/core/src/error.rs:87
// match: engine-types/src/precompiles.rs
enum precompile_error_ExitRevert : uint32_t {
    Reverted = 1,  // only one
};

// ref: ~.cargo/git/checkouts/evm-31951a45719dc0d6/07ae445/core/src/error.rs:157
// match: engine-types/src/precompiles.rs
enum precompile_error_ExitFatal : uint32_t {
    NotSupported = 1,  // todo add more
};

struct sys_contract_precompile_output {
    precompile_output_ExitSucceed exit_status;  // uint32_t exit_status;
    uint64_t cost;
    xbytes_t output;
    std::vector<evm_common::xevm_log_t> logs;
};

struct sys_contract_precompile_error {
    precompile_error fail_status;  // uint32_t fail_status;
    uint32_t minor_status{0};
    xbytes_t output;
    uint64_t cost{0};
};

class xtop_evm_syscontract_face {
public:
    xtop_evm_syscontract_face() = default;
    xtop_evm_syscontract_face(xtop_evm_syscontract_face const &) = delete;
    xtop_evm_syscontract_face & operator=(xtop_evm_syscontract_face const &) = delete;
    xtop_evm_syscontract_face(xtop_evm_syscontract_face &&) = default;
    xtop_evm_syscontract_face & operator=(xtop_evm_syscontract_face &&) = default;
    virtual ~xtop_evm_syscontract_face() = default;

    virtual bool execute(xbytes_t input,
                         uint64_t target_gas,
                         sys_contract_context const & context,
                         bool is_static,
                         sys_contract_precompile_output & output,
                         sys_contract_precompile_error & err) = 0;
};
using xevm_syscontract_face_t = xtop_evm_syscontract_face;

NS_END3