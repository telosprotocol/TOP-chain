// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcommon/xeth_address.h"
#include "xdata/xnative_contract_address.h"
#include "xcommon/common.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xevm_transaction_result.h"
#if defined(XCXX20)
#include "xevm_runner/proto/ubuntu/proto_precompile.pb.h"
#else
#include "xevm_runner/proto/centos/proto_precompile.pb.h"
#endif

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xstatectx/xstatectx_face.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include <cstdint>
#include <vector>

NS_BEG3(top, contract_runtime, evm)

struct sys_contract_context {
    common::xeth_address_t address;
    common::xeth_address_t caller;
    evm_common::u256 apparent_value;

    sys_contract_context() = default;
    sys_contract_context(evm_engine::precompile::ContractContext const & proto_context)
      : address{common::xeth_address_t::build_from(top::to_bytes(proto_context.address().value()))}
      , caller{common::xeth_address_t::build_from(top::to_bytes(proto_context.caller().value()))} {
        apparent_value = evm_common::fromBigEndian<evm_common::u256>(proto_context.apparent_value().data());
    }
};

enum precompile_output_ExitSucceed : uint32_t {
    Stopped = 1,
    Returned = 2,
    Suicided = 3,
};

enum class precompile_error : uint32_t {
    error = 1,
    revert = 2,
    fatal = 3,
};

// ref: ~.cargo/git/checkouts/evm-31951a45719dc0d6/07ae445/core/src/error.rs:105
// match: engine-types/src/precompiles.rs
enum class precompile_error_ExitError : uint32_t {
    StackUnderflow = 0,
    StackOverflow = 1,
    InvalidJump = 2,
    InvalidRange = 3,
    DesignatedInvalid = 4,
    CallTooDeep = 5,
    CreateCollision = 6,
    CreateContractLimit = 7,
    InvalidCode = 8,
    OutOfOffset = 9,
    OutOfGas = 10,
    OutOfFund = 11,
    PCUnderflow = 12,
    CreateEmpty = 13,
    Other = 14,
};

// ref: ~.cargo/git/checkouts/evm-31951a45719dc0d6/07ae445/core/src/error.rs:87
// match: engine-types/src/precompiles.rs
enum class precompile_error_ExitRevert : uint32_t {
    Reverted = 1,  // only one
};

// ref: ~.cargo/git/checkouts/evm-31951a45719dc0d6/07ae445/core/src/error.rs:157
// match: engine-types/src/precompiles.rs
enum class precompile_error_ExitFatal : uint32_t {
    NotSupported = 1,  // todo add more
    UnhandledInterrupt = 2,
    CallErrorAsFatal = 3,
    Other = 4
};

struct sys_contract_precompile_output {
    precompile_output_ExitSucceed exit_status;  // uint32_t exit_status;
    uint64_t cost{0};
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
                         observer_ptr<statectx::xstatectx_face_t> state_ctx,
                         sys_contract_precompile_output & output,
                         sys_contract_precompile_error & err) = 0;
};
using xevm_syscontract_face_t = xtop_evm_syscontract_face;

NS_END3
