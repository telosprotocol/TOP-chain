// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcommon/xeth_address.h"
#include "xdata/xnative_contract_address.h"
#include "xevm_common/common.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xevm_transaction_result.h"
#include "xevm_runner/proto/proto_precompile.pb.h"

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

enum precompile_error : uint32_t {
    Error = 1,
    Revert = 2,
    Fatal = 3,
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

template <typename T>
class xtop_evm_crosschain_syscontract_face : public xtop_evm_syscontract_face {
public:
    using state_ptr = std::shared_ptr<data::xunit_bstate_t>;

    xtop_evm_crosschain_syscontract_face() = default;
    xtop_evm_crosschain_syscontract_face(xtop_evm_crosschain_syscontract_face const &) = delete;
    xtop_evm_crosschain_syscontract_face & operator=(xtop_evm_crosschain_syscontract_face const &) = delete;
    xtop_evm_crosschain_syscontract_face(xtop_evm_crosschain_syscontract_face &&) = default;
    xtop_evm_crosschain_syscontract_face & operator=(xtop_evm_crosschain_syscontract_face &&) = default;
    ~xtop_evm_crosschain_syscontract_face() override = default;
    
    bool execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 observer_ptr<statectx::xstatectx_face_t> state_ctx,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) override;

    bool init(const xbytes_t & rlp_bytes, state_ptr state) {
        return static_cast<T*>(this)->init(rlp_bytes, state);
    }
    bool sync(const xbytes_t & rlp_bytes, state_ptr state) {
        return static_cast<T*>(this)->sync(rlp_bytes, state);
    }
    bool is_known(const evm_common::u256 height, const xbytes_t & hash_bytes, state_ptr state) const {
        return static_cast<const T*>(this)->is_known(height, hash_bytes, state);
    }
    bool is_confirmed(const evm_common::u256 height, const xbytes_t & hash_bytes, state_ptr state) const {
        return static_cast<const T*>(this)->is_confirmed(height, hash_bytes, state);
    }
    evm_common::bigint get_height(state_ptr state) const {
        return static_cast<const T*>(this)->get_height(state);
    }
    void reset(state_ptr state) {
        return static_cast<T*>(this)->reset(state);
    }

    std::set<std::string> m_whitelist;
};

template <typename T>
inline bool xtop_evm_crosschain_syscontract_face<T>::execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 observer_ptr<statectx::xstatectx_face_t> state_ctx,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) {
    // method ids:
    //--------------------------------------------------
    // init(bytes,string)                    => 6158600d
    // sync(bytes)                           => 7eefcfa2
    // get_height()                          => b15ad2e8
    // is_known(uint256,bytes32)             => 6d571daf
    // is_confirmed(uint256,bytes32)         => d398572f
    // reset()                               => d826f88f
    //--------------------------------------------------
    constexpr uint32_t method_id_init{0x6158600d};
    constexpr uint32_t method_id_sync{0x7eefcfa2};
    constexpr uint32_t method_id_get_height{0xb15ad2e8};
    constexpr uint32_t method_id_is_known{0x6d571daf};
    constexpr uint32_t method_id_is_confirmed{0xd398572f};
    constexpr uint32_t method_id_reset{0xd826f88f};

    // check param
    assert(state_ctx);
    auto state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.address, base::enum_vaccount_addr_type_secp256k1_evm_user_account).vaccount());
    if (state == nullptr) {
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] state nullptr");
        return false;
    }
    if (input.empty()) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] invalid input");
        return false;
    }
    std::error_code ec;
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(xbytes_t{std::begin(input), std::end(input)}, ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] illegal input data");
        return false;
    }
    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] illegal input function selector");
        return false;
    }
    xinfo("[xtop_evm_crosschain_syscontract_face::execute] caller: %s, address: %s, method_id: 0x%x, input size: %zu",
          context.caller.to_hex_string().c_str(),
          context.address.to_hex_string().c_str(),
          function_selector.method_id,
          input.size());

    switch (function_selector.method_id) {
    case method_id_init: {
        if (!m_whitelist.count(context.caller.to_hex_string())) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] init is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        if (!init(headers_rlp, state)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] init headers error");
            return false;
        }
        evm_common::xh256s_t topics;
        topics.push_back(evm_common::xh256_t(context.caller.to_h256()));
        evm_common::xevm_log_t log(context.address, topics, top::to_bytes(evm_common::u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        return true;
    }
    case method_id_sync: {
        if (!m_whitelist.count(context.caller.to_hex_string())) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] sync is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract error");
            return false;
        }
        if (!sync(headers_rlp, state)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] sync headers error");
            return false;
        }
        evm_common::xh256s_t topics;
        topics.push_back(evm_common::xh256_t(context.caller.to_h256()));
        evm_common::xevm_log_t log(context.address, topics, top::to_bytes(evm_common::u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        return true;
    }
    case method_id_get_height: {
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(get_height(state)));
        return true;
    }
    case method_id_is_known: {
        auto height = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        uint32_t known{0};
        if (is_known(height, hash_bytes, state)) {
            known = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(known));
        return true;
    }
    case method_id_is_confirmed: {
        auto height = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        uint32_t confirmed{0};
        if (is_confirmed(height, hash_bytes, state)) {
            confirmed = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(confirmed));
        return true;
    }
    case method_id_reset: {
        if (!m_whitelist.count(context.caller.to_hex_string())) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        reset(state);
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(0));
        return true;
    }
    default: {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);
        return false;
    }
    }

    return true;
}

NS_END3
