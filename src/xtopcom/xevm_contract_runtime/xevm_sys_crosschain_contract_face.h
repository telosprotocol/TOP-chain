// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xcrosschain_whitelist.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

#if defined(XCXX20)
#include <fifo_map.hpp>
#else
#include <nlohmann/fifo_map.hpp>
#endif
#include <nlohmann/json.hpp>

#ifdef ETH_BRIDGE_TEST
#include <fstream>
#endif

NS_BEG3(top, contract_runtime, evm)

using evm_common::bigint;
using evm_common::h128;
using evm_common::h256;
using evm_common::h512;
using evm_common::u256;
using evm_common::u64;
using evm_common::xeth_header_t;
using state_ptr = std::shared_ptr<data::xunit_bstate_t>;

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

enum class xheader_status_t : uint8_t {
    header_overdue = 0,
    header_confirmed = 1,
    header_not_confirmed = 2,
};

template <typename T>
class xtop_evm_crosschain_syscontract_face : public xtop_evm_syscontract_face {
public:
    xtop_evm_crosschain_syscontract_face() {
        m_whitelist = load_whitelist();
        if (m_whitelist.empty()) {
            xwarn("[xtop_evm_heco_client_contract] whitelist empty!");
        }
    }
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

    std::set<std::string> load_whitelist();

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
    evm_common::bigint query_height(const evm_common::u256 height, state_ptr state) const {
        return static_cast<const T*>(this)->query_height(height, state);
    }
    bool reset(state_ptr state) {
        return static_cast<T*>(this)->reset(state);
    }
    bool disable_reset(state_ptr state) {
        return static_cast<T*>(this)->disable_reset(state);
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
    // query_height(uint256)                 => 12fdf338
    // is_known(uint256,bytes32)             => 6d571daf
    // is_confirmed(uint256,bytes32)         => d398572f
    // reset()                               => d826f88f
    // disable_reset()                       => b5a61069
    //--------------------------------------------------
    constexpr uint32_t method_id_init{0x6158600d};
    constexpr uint32_t method_id_sync{0x7eefcfa2};
    constexpr uint32_t method_id_get_height{0xb15ad2e8};
    constexpr uint32_t method_id_query_height{0x12fdf338};
    constexpr uint32_t method_id_is_known{0x6d571daf};
    constexpr uint32_t method_id_is_confirmed{0xd398572f};
    constexpr uint32_t method_id_reset{0xd826f88f};
    constexpr uint32_t method_id_disable_reset{0xb5a61069};

    // check param
    assert(state_ctx);
    auto state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.address, base::enum_vaccount_addr_type_secp256k1_evm_user_account));
    if (state == nullptr) {
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] state nullptr");
        return false;
    }
    if (input.empty()) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] invalid input");
        return false;
    }
    std::error_code ec;
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(xbytes_t{std::begin(input), std::end(input)}, ec);
    if (ec) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] illegal input data");
        return false;
    }
    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::fatal;
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
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
        if (!m_whitelist.count(context.caller.to_hex_string())) {
#else
        if (!m_whitelist.empty() && !m_whitelist.count(context.caller.to_hex_string())) {
#endif
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] init is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        if (!init(headers_rlp, state)) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] init headers error");
            return false;
        }
        xh256s_t topics;
        topics.push_back(xh256_t(context.caller.to_h256()));
        evm_common::xevm_log_t log(context.address, topics, top::to_bytes(evm_common::u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        return true;
    }
    case method_id_sync: {
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
        if (!m_whitelist.count(context.caller.to_hex_string())) {
#else
        if (!m_whitelist.empty() && !m_whitelist.count(context.caller.to_hex_string())) {
#endif
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] sync is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract error");
            return false;
        }
        if (!sync(headers_rlp, state)) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] sync headers error");
            return false;
        }
        xh256s_t topics;
        topics.push_back(xh256_t(context.caller.to_h256()));
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
    case method_id_query_height: {
        auto height = abi_decoder.extract<u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(query_height(height, state)));
        return true;
    }
    case method_id_is_known: {
        auto height = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
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
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
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
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
        if (!m_whitelist.count(context.caller.to_hex_string())) {
#else
        if (!m_whitelist.empty() && !m_whitelist.count(context.caller.to_hex_string())) {
#endif
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (!reset(state)) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] reset error");
            return false;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(0));
        return true;
    }
    case method_id_disable_reset: {
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
        if (!m_whitelist.count(context.caller.to_hex_string())) {
#else
        if (!m_whitelist.empty() && !m_whitelist.count(context.caller.to_hex_string())) {
#endif
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            err.cost = 50000;
            xwarn("[xtop_evm_crosschain_syscontract_face::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        disable_reset(state);
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(0));
        return true;
    }
    default: {
        xwarn("[xtop_evm_crosschain_syscontract_face::execute] not found method id: 0x%x", function_selector.method_id);
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);
        return false;
    }
    }

    return true;
}

template <typename T>
inline std::set<std::string> xtop_evm_crosschain_syscontract_face<T>::load_whitelist() {
    json j;
#ifdef ETH_BRIDGE_TEST
#    define WHITELIST_FILE "eth_bridge_whitelist.json"
    xinfo("[xtop_evm_crosschain_syscontract_face::load_whitelist] load from file %s", WHITELIST_FILE);
    std::ifstream data_file(WHITELIST_FILE);
    if (data_file.good()) {
        data_file >> j;
        data_file.close();
    } else {
        xwarn("[xtop_evm_crosschain_syscontract_face::load_whitelist] file %s open error", WHITELIST_FILE);
        return {};
    }
#else
    xinfo("[xtop_evm_crosschain_syscontract_face::load_whitelist] load from code");
    j = json::parse(data::crosschain_whitelist());
#endif

    if (!j.count("whitelist")) {
        xwarn("[xtop_evm_crosschain_syscontract_face::load_whitelist] load whitelist failed");
        return {};
    }
    std::set<std::string> ret;
    auto const & list = j["whitelist"];
    for (auto const & item : list) {
        ret.insert(item.get<std::string>());
    }
    return ret;
}

NS_END3
