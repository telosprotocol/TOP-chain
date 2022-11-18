// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_eth2_client_contract.h"

#include "xbasic/endianness.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xdata/xdata_common.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xabi_decoder.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

using namespace evm_common::eth2;

#define EPOCHS_PER_SYNC_COMMITTEE_PERIOD 256U
#define SLOTS_PER_EPOCH 32U
#define MIN_SYNC_COMMITTEE_PARTICIPANTS 1U
#define FINALITY_TREE_DEPTH 6U
#define FINALITY_TREE_INDEX 41U
#define SYNC_COMMITTEE_TREE_DEPTH 5U
#define SYNC_COMMITTEE_TREE_INDEX 23U

constexpr uint64_t hashes_gc_threshold = 51000;

static uint64_t compute_epoch_at_slot(uint64_t const slot) {
    return slot / SLOTS_PER_EPOCH;
}

static uint64_t compute_sync_committee_period(uint64_t const slot) {
    return compute_epoch_at_slot(slot) / EPOCHS_PER_SYNC_COMMITTEE_PERIOD;
}

static std::string covert_committee_bits_to_bin_str(xbytes_t const & bits) {
    assert(bits.size() % 2 == 0);
    auto bin_str = data::hex_str_to_bin_str({bits.begin(), bits.end()});
    assert(bin_str.size() % 8 == 0);
    std::string bin_str_lsb;
    for (auto it = bin_str.begin(); it != bin_str.end(); it += 8) {
        for (auto i = 0; i < 8; i++) {
            bin_str_lsb.push_back(*(it + 7 - i));
        }
    }
    return bin_str_lsb;
}

static std::vector<xbytes_t> get_participant_pubkeys(std::vector<xbytes_t> const & public_keys, std::string const & sync_committee_bits) {
    std::vector<xbytes_t> ret;
    for (size_t i = 0; i < sync_committee_bits.size(); ++i) {
        if (sync_committee_bits[i] == '1') {
            ret.emplace_back(public_keys[i]);
        }
    }
    return ret;
}

xtop_evm_eth2_client_contract::xtop_evm_eth2_client_contract() : m_network(xeth2_client_net_t::eth2_net_mainnet) {
    m_whitelist = load_whitelist();
    if (m_whitelist.empty()) {
        xwarn("[xtop_evm_eth2_client_contract] whitelist empty!");
    }
}

xtop_evm_eth2_client_contract::xtop_evm_eth2_client_contract(xeth2_client_net_t version) : m_network(version) {
    m_whitelist = load_whitelist();
    if (m_whitelist.empty()) {
        xwarn("[xtop_evm_eth2_client_contract] whitelist empty!");
    }
}

std::set<std::string> xtop_evm_eth2_client_contract::load_whitelist() {
    xinfo("xtop_evm_eth2_client_contract::load_whitelist load from code");
    auto j = json::parse(data::crosschain_whitelist());

    if (!j.count("whitelist")) {
        xwarn("xtop_evm_eth2_client_contract::load_whitelist load whitelist failed");
        return {};
    }
    std::set<std::string> ret;
    auto const & list = j["whitelist"];
    for (auto const item : list) {
        ret.insert(item.get<std::string>());
    }
    return ret;
}

bool xtop_evm_eth2_client_contract::execute(xbytes_t input,
                                            uint64_t target_gas,
                                            sys_contract_context const & context,
                                            bool is_static,
                                            observer_ptr<statectx::xstatectx_face_t> state_ctx,
                                            sys_contract_precompile_output & output,
                                            sys_contract_precompile_error & err) {
    // method ids:
    //--------------------------------------------------------------
    // "4ddf47d4": "init(bytes)",
    // "158ef93e": "initialized()",
    // "3bcdaaab": "block_hash_safe(uint64)",
    // "55b39f6e": "finalized_beacon_block_header()",
    // "4b469132": "finalized_beacon_block_root()",
    // "074b1681": "finalized_beacon_block_slot()",
    // "3ae8d743": "get_light_client_state()",
    // "d398572f": "is_confirmed(uint256,bytes32)",
    // "43b1378b": "is_known_execution_header(bytes32)",
    // "b15ad2e8": "get_height()"
    // "1eeaebb2": "last_block_number()",
    // "2e139f0c": "submit_beacon_chain_light_client_update(bytes)",
    // "3c1a38b6": "submit_execution_header(bytes)",
    // "d826f88f": "reset()",
    // "b5a61069": "disable_reset()",
    //--------------------------------------------------------------

    constexpr uint32_t method_id_init{0x4ddf47d4};
    constexpr uint32_t method_id_initialized{0x158ef93e};
    constexpr uint32_t method_id_block_hash_safe{0x3bcdaaab};
    constexpr uint32_t method_id_finalized_beacon_block_header{0x55b39f6e};
    constexpr uint32_t method_id_finalized_beacon_block_root{0x4b469132};
    constexpr uint32_t method_id_finalized_beacon_block_slot{0x074b1681};
    constexpr uint32_t method_id_get_light_client_state{0x3ae8d743};
    constexpr uint32_t method_id_is_confirmed{0xd398572f};
    constexpr uint32_t method_id_is_known_execution_header{0x43b1378b};
    constexpr uint32_t method_id_get_height{0xb15ad2e8};
    constexpr uint32_t method_id_last_block_number{0x1eeaebb2};
    constexpr uint32_t method_id_submit_beacon_chain_light_client_update{0x2e139f0c};
    constexpr uint32_t method_id_submit_execution_header{0x3c1a38b6};
    constexpr uint32_t method_id_reset{0xd826f88f};
    constexpr uint32_t method_id_disable_reset{0xb5a61069};

    // check param
    assert(state_ctx);
    auto state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.address, base::enum_vaccount_addr_type_secp256k1_evm_user_account).vaccount());
    if (state == nullptr) {
        xwarn("[xtop_evm_eth2_client_contract::execute] state nullptr");
        return false;
    }
    if (input.empty()) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth2_client_contract::execute] invalid input");
        return false;
    }
    std::error_code ec;
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(xbytes_t{std::begin(input), std::end(input)}, ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth2_client_contract::execute] illegal input data");
        return false;
    }
    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth2_client_contract::execute] illegal input function selector");
        return false;
    }
    xinfo("[xtop_evm_eth2_client_contract::execute] caller: %s, address: %s, method_id: 0x%x, input size: %zu",
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
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] init is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        xinit_input_t init_put;
        if (false == init_put.decode_rlp(headers_rlp)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] init_put decode error");
            return false;
        }
        if (!init(state, init_put)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] init headers error");
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
    case method_id_initialized: {
        uint32_t inited{0};
        if (initialized(state)) {
            inited = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(inited));
        return true;
    }
    case method_id_block_hash_safe: {
        auto number = abi_decoder.extract<uint64_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash = block_hash_safe(state, number);
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(hash));
        return true;
    }
    case method_id_finalized_beacon_block_header: {
        auto header = finalized_beacon_block_header(state);
        output.exit_status = Returned;
        output.cost = 0;
        output.output = header.encode_rlp();
        return true;
    }
    case method_id_finalized_beacon_block_root: {
        auto hash = finalized_beacon_block_root(state);
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(hash));
        return true;
    }
    case method_id_finalized_beacon_block_slot: {
        auto slot = finalized_beacon_block_slot(state);
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(slot));
        return true;
    }
    case method_id_get_light_client_state: {
        auto client_state = get_light_client_state(state);
        output.exit_status = Returned;
        output.cost = 0;
        output.output = client_state.encode_rlp();
        return true;
    }
    case method_id_is_confirmed: {
        u256 height = abi_decoder.extract<evm_common::u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        uint32_t confirmed{0};
        if (is_confirmed(state, height, static_cast<h256>(hash_bytes))) {
            confirmed = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(confirmed));
        return true;
    }
    case method_id_is_known_execution_header: {
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        uint32_t is_known{0};
        if (is_known_execution_header(state, static_cast<h256>(hash_bytes))) {
            is_known = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(is_known));
        return true;
    }
    case method_id_get_height:
    case method_id_last_block_number: {
        auto number = last_block_number(state);
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(number));
        return true;
    }
    case method_id_submit_beacon_chain_light_client_update: {
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
        if (!m_whitelist.count(context.caller.to_hex_string())) {
#else
        if (!m_whitelist.empty() && !m_whitelist.count(context.caller.to_hex_string())) {
#endif
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] method_id_submit_beacon_chain_light_client_update is not allowed in static context");
            return false;
        }
        auto update_bytes = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        xlight_client_update_t update;
        if (false == update.decode_rlp(update_bytes)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] update decode error");
            return false;
        }
        if (!submit_beacon_chain_light_client_update(state, update)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] submit_beacon_chain_light_client_update error");
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
    case method_id_submit_execution_header: {
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
        if (!m_whitelist.count(context.caller.to_hex_string())) {
#else
        if (!m_whitelist.empty() && !m_whitelist.count(context.caller.to_hex_string())) {
#endif
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] method_id_submit_execution_header is not allowed in static context");
            return false;
        }
        auto bytes = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        evm_common::xeth_header_t header;
        auto left_bytes = std::move(bytes);
        while (left_bytes.size() != 0) {
            evm_common::RLP::DecodedItem item = evm_common::RLP::decode(left_bytes);
            left_bytes = std::move(item.remainder);
            evm_common::xeth_header_t header;
            {
                auto item_header = evm_common::RLP::decode_once(item.decoded[0]);
                auto header_bytes = item_header.decoded[0];
                if (header.decode_rlp(header_bytes) == false) {
                    xwarn("[xtop_evm_eth2_client_contract::execute] decode header error");
                    return false;
                }
            }
            if (!submit_execution_header(state, header)) {
                err.fail_status = precompile_error::Revert;
                err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
                xwarn("[xtop_evm_eth2_client_contract::execute] submit_execution_header error");
                return false;
            }
        }
        evm_common::xh256s_t topics;
        topics.push_back(evm_common::xh256_t(context.caller.to_h256()));
        evm_common::xevm_log_t log(context.address, topics, top::to_bytes(evm_common::u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        return true;
    }
    case method_id_reset: {
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
        if (!m_whitelist.count(context.caller.to_hex_string())) {
#else
        if (!m_whitelist.empty() && !m_whitelist.count(context.caller.to_hex_string())) {
#endif
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (!reset(state)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] reset error");
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
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        disable_reset(state);
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(0));
        return true;
    }
    default: {
        xwarn("[xtop_evm_eth2_client_contract::execute] not found method id: 0x%x", function_selector.method_id);
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);
        return false;
    }
    }

    return true;
}

bool xtop_evm_eth2_client_contract::init(state_ptr const & state, xinit_input_t const & init_input) {
    if (initialized(state)) {
        xwarn("xtop_evm_eth2_client_contract::init already");
        return false;
    }
    auto const & hash = init_input.finalized_execution_header.hash();
    if (hash != init_input.finalized_beacon_header.execution_block_hash) {
        xwarn("xtop_evm_eth2_client_contract::init hash mismatch %s, %s", hash.hex().c_str(), init_input.finalized_beacon_header.execution_block_hash.hex().c_str());
        return false;
    }
    auto const & finalized_execution_header_info =
        xexecution_header_info_t{init_input.finalized_execution_header.parent_hash, static_cast<uint64_t>(init_input.finalized_execution_header.number)};
    if (false == set_finalized_beacon_header(state, init_input.finalized_beacon_header)) {
        xwarn("xtop_evm_eth2_client_contract::init set_finalized_beacon_header error");
        return false;
    }
    if (false == set_finalized_execution_header(state, finalized_execution_header_info)) {
        xwarn("xtop_evm_eth2_client_contract::init set_finalized_execution_header error");
        return false;
    }
    if (false == set_current_sync_committee(state, init_input.current_sync_committee)) {
        xwarn("xtop_evm_eth2_client_contract::init set_current_sync_committee error");
        return false;
    }
    if (false == set_next_sync_committee(state, init_input.next_sync_committee)) {
        xwarn("xtop_evm_eth2_client_contract::init set_next_sync_committee error");
        return false;
    }
    xinfo("xtop_evm_eth2_client_contract::init success");
    return true;
}

bool xtop_evm_eth2_client_contract::initialized(state_ptr const & state) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    return !finalized_beacon_header.empty();
}

bool xtop_evm_eth2_client_contract::reset(state_ptr state) {
    if (get_flag(state) != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::reset] reset already disabled");
        return false;
    }
    state->map_clear(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS);
    state->map_clear(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS);
    xextended_beacon_block_header_t beacon_block;
    auto beacon_header_bytes = beacon_block.encode_rlp();
    state->string_set(data::system_contract::XPROPERTY_FINALIZED_BEACON_HEADER, {beacon_header_bytes.begin(), beacon_header_bytes.end()});
    xexecution_header_info_t execution_header;
    auto execution_header_bytes = execution_header.encode_rlp();
    state->string_set(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_HEADER, {execution_header_bytes.begin(), execution_header_bytes.end()});
    xinfo("[xtop_evm_eth2_client_contract::reset] reset success");
    return true;
}

bool xtop_evm_eth2_client_contract::disable_reset(state_ptr state) {
    xinfo("[xtop_evm_eth2_client_contract::disable_reset] disable_reset");
    return set_flag(state);
}

uint64_t xtop_evm_eth2_client_contract::last_block_number(state_ptr const & state) const {
    return get_finalized_execution_header(state).block_number;
}

h256 xtop_evm_eth2_client_contract::block_hash_safe(state_ptr const & state, u256 const block_number) const {
    return get_finalized_execution_blocks(state, static_cast<uint64_t>(block_number));
}

bool xtop_evm_eth2_client_contract::is_known_execution_header(state_ptr const & state, h256 const & hash) const {
    auto const & header = get_unfinalized_headers(state, hash);
    if (header.empty()) {
        return false;
    }
    return true;
}

bool xtop_evm_eth2_client_contract::is_confirmed(state_ptr state, u256 const height, h256 const & hash_bytes) const {
    h256 hash = static_cast<h256>(hash_bytes);
    h256 origin_hash = get_finalized_execution_blocks(state, static_cast<uint64_t>(height));
    if (hash != origin_hash) {
        xwarn("[xtop_evm_eth2_client_contract::is_confirmed] hash mismatch: %s, %s", hash.hex().c_str(), origin_hash.hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth2_client_contract::is_confirmed] is_confirmed: %s", hash.hex().c_str());
    return true;
}

h256 xtop_evm_eth2_client_contract::finalized_beacon_block_root(state_ptr const & state) const {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::finalized_beacon_block_root get_finalized_beacon_header empty");
        return {};
    }
    return finalized_beacon_header.beacon_block_root;
}

uint64_t xtop_evm_eth2_client_contract::finalized_beacon_block_slot(state_ptr const & state) const {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::finalized_beacon_block_slot get_finalized_beacon_header empty");
        return 0;
    }
    return finalized_beacon_header.header.slot;
}

xextended_beacon_block_header_t xtop_evm_eth2_client_contract::finalized_beacon_block_header(state_ptr const & state) const {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::finalized_beacon_block_header get_finalized_beacon_header empty");
        return {};
    }
    return finalized_beacon_header;
}

xlight_client_state_t xtop_evm_eth2_client_contract::get_light_client_state(state_ptr const & state) const {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_light_client_state get_finalized_beacon_header empty");
        return {};
    }
    auto const & current_sync_committee = get_current_sync_committee(state);
    if (current_sync_committee.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_light_client_state current_sync_committee empty");
        return {};
    }
    auto const & next_sync_committee = get_next_sync_committee(state);
    if (next_sync_committee.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_light_client_state next_sync_committee empty");
        return {};
    }
    return {finalized_beacon_header, current_sync_committee, next_sync_committee};
}

bool xtop_evm_eth2_client_contract::submit_beacon_chain_light_client_update(state_ptr const & state, xlight_client_update_t const & update) {
    if (false == validate_light_client_update(state, update)) {
        xwarn("xtop_evm_eth2_client_contract::submit_beacon_chain_light_client_update validate_light_client_update error");
        return false;
    }
    if (false == commit_light_client_update(state, update)) {
        xwarn("xtop_evm_eth2_client_contract::submit_beacon_chain_light_client_update commit_light_client_update error");
        return false;
    }
    xinfo("xtop_evm_eth2_client_contract::submit_beacon_chain_light_client_update success");
    return true;
}

bool xtop_evm_eth2_client_contract::submit_execution_header(state_ptr const & state, evm_common::xeth_header_t const & block_header) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch get_finalized_beacon_header empty");
        return false;
    }
    if (finalized_beacon_header.execution_block_hash != block_header.parent_hash) {
        auto unfinalized_header = get_unfinalized_headers(state, block_header.parent_hash);
        if (unfinalized_header.empty()) {
            xwarn("xtop_evm_eth2_client_contract::submit_execution_header parent %s not submitted", block_header.parent_hash.hex().c_str());
            return false;
        }
    }
    auto const & block_hash = block_header.hash();
    auto const & block_info = xexecution_header_info_t{block_header.parent_hash, static_cast<uint64_t>(block_header.number)};
    if (false == set_unfinalized_headers(state, block_hash, block_info)) {
        xwarn("xtop_evm_eth2_client_contract::submit_execution_header set_unfinalized_headers error: %s", block_hash.hex().c_str());
        return false;
    }
    xinfo("xtop_evm_eth2_client_contract::submit_execution_header header %s set succss", block_hash.hex().c_str());
    return true;
}

bool xtop_evm_eth2_client_contract::validate_light_client_update(state_ptr const & state, xlight_client_update_t const & update) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch get_finalized_beacon_header empty");
        return false;
    }
    auto finalized_period = compute_sync_committee_period(finalized_beacon_header.header.slot);
    if (false == verify_finality_branch(state, update, finalized_period)) {
        xwarn("xtop_evm_eth2_client_contract::validate_light_client_update verify_finality_branch error");
        return false;
    }
    auto const & sync_committee_bits = update.sync_aggregate.sync_committee_bits;
    auto const & bits_str = covert_committee_bits_to_bin_str({sync_committee_bits.begin(), sync_committee_bits.end()});

    uint64_t sync_committee_bits_sum{0};
    for (auto const & b : bits_str) {
        sync_committee_bits_sum += (b == '1' ? 1 : 0);
    }
    if (sync_committee_bits_sum < MIN_SYNC_COMMITTEE_PARTICIPANTS) {
        xwarn("xtop_evm_eth2_client_contract::validate_light_client_update error sync_committee_bits_sum: %lu", sync_committee_bits_sum);
        return false;
    }
    if (sync_committee_bits_sum * 3 < (sync_committee_bits.size() * 2)) {
        xwarn("xtop_evm_eth2_client_contract::validate_light_client_update committee bits sum is less than 2/3 threshold, %lu, %zu",
              sync_committee_bits_sum,
              sync_committee_bits.size());
        return false;
    }
    if (false == verify_bls_signatures(state, update, bits_str, finalized_period)) {
        xwarn("xtop_evm_eth2_client_contract::validate_light_client_update verify_bls_signatures error");
        return false;
    }
    xinfo("xtop_evm_eth2_client_contract::validate_light_client_update validate update success");
    return true;
}

bool xtop_evm_eth2_client_contract::verify_finality_branch(state_ptr const & state, xlight_client_update_t const & update, uint64_t const finalized_period) {
    auto const & active_header = update.finality_update.header_update.beacon_header;
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch get_finalized_beacon_header empty");
        return false;
    }
    if (active_header.slot <= finalized_beacon_header.header.slot) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch slot mismatch, %lu, %lu", active_header.slot, finalized_beacon_header.header.slot);
        return false;
    }
    if (update.attested_beacon_header.slot < update.finality_update.header_update.beacon_header.slot) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch slot mismatch, %lu, %lu", active_header.slot, finalized_beacon_header.header.slot);
        return false;
    }
    if (update.signature_slot <= update.attested_beacon_header.slot) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch slot mismatch, %lu, %lu", update.signature_slot, update.attested_beacon_header.slot);
        return false;
    }
    auto update_period = compute_sync_committee_period(active_header.slot);
    if (update_period != finalized_period && update_period != finalized_period + 1) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch period mismatch, %lu, %lu", update_period, finalized_period);
        return false;
    }
    xbytes_t branch_data;
    for (auto const & b : update.finality_update.finality_branch) {
        branch_data.insert(branch_data.end(), b.begin(), b.end());
    }
    xbytes_t merkle_root(32);
    if (false == unsafe_merkle_proof(update.finality_update.header_update.beacon_header.tree_hash_root().data(),
                                     branch_data.data(),
                                     branch_data.size(),
                                     FINALITY_TREE_DEPTH,
                                     FINALITY_TREE_INDEX,
                                     merkle_root.data())) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch header unsafe_merkle_proof error");
        return false;
    }
    if (merkle_root != update.attested_beacon_header.state_root.asBytes()) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch header root error: %s, %s",
              to_hex(merkle_root).c_str(),
              to_hex(update.attested_beacon_header.state_root).c_str());
        return false;
    }
    if (update_period != finalized_period) {
        xbytes_t branch_data;
        for (auto const & b : update.sync_committee_update.next_sync_committee_branch) {
            branch_data.insert(branch_data.end(), b.begin(), b.end());
        }
        xbytes_t merkle_root(32);
        if (false == unsafe_merkle_proof(update.sync_committee_update.next_sync_committee.tree_hash_root().data(),
                                         branch_data.data(),
                                         branch_data.size(),
                                         SYNC_COMMITTEE_TREE_DEPTH,
                                         SYNC_COMMITTEE_TREE_INDEX,
                                         merkle_root.data())) {
            xwarn("xtop_evm_eth2_client_contract::verify_finality_branch committee unsafe_merkle_proof error");
            return false;
        }
        if ((merkle_root != active_header.state_root.asBytes()) && (merkle_root != update.attested_beacon_header.state_root.asBytes())) {
            xwarn("xtop_evm_eth2_client_contract::verify_finality_branch committee root error: %s, %s, %s",
                  to_hex(merkle_root).c_str(),
                  to_hex(update.attested_beacon_header.state_root).c_str(),
                  to_hex(update.attested_beacon_header.state_root).c_str());
            return false;
        }
    }
    return true;
}

bool xtop_evm_eth2_client_contract::verify_bls_signatures(state_ptr const & state,
                                                          xlight_client_update_t const & update,
                                                          std::string const & sync_committee_bits,
                                                          uint64_t const finalized_period) {
    auto signature_period = compute_sync_committee_period(update.signature_slot);
    if (signature_period != finalized_period && signature_period != finalized_period + 1) {
        xwarn("xtop_evm_eth2_client_contract::verify_bls_signatures signature_period mismatch: %lu, %lu", signature_period, finalized_period);
        return false;
    }
    xsync_committee_t sync_committee;
    if (signature_period == finalized_period) {
        sync_committee = get_current_sync_committee(state);
        if (sync_committee.empty()) {
            xwarn("xtop_evm_eth2_client_contract::verify_bls_signatures get_current_sync_committee empty");
            return false;
        }
    } else {
        sync_committee = get_next_sync_committee(state);
        if (sync_committee.empty()) {
            xwarn("xtop_evm_eth2_client_contract::verify_bls_signatures get_next_sync_committee empty");
            return false;
        }
    }
    auto const & participant_pubkeys = get_participant_pubkeys(sync_committee.pubkeys, sync_committee_bits);
    xbytes_t signing_root(32);
    unsafe_compute_bellatrix_committee_signing_root(update.attested_beacon_header.tree_hash_root().data(), uint8_t(m_network), signing_root.data());
    xbytes_t pubkeys_data;
    for (auto const & pubkey : participant_pubkeys) {
        pubkeys_data.insert(pubkeys_data.begin(), pubkey.begin(), pubkey.end());
    }
    return unsafe_verify_bls_signatures(update.sync_aggregate.sync_committee_signature.data(), pubkeys_data.data(), pubkeys_data.size(), signing_root.data());
}

bool xtop_evm_eth2_client_contract::update_finalized_header(state_ptr const & state, xextended_beacon_block_header_t const & finalized_header) {
    //  finalized_execution_header_info
    auto const & finalized_execution_header_info = get_unfinalized_headers(state, finalized_header.execution_block_hash);
    if (finalized_execution_header_info.empty()) {
        xwarn("xtop_evm_eth2_client_contract::update_finalized_header get_unfinalized_headers %s empty", finalized_header.execution_block_hash.hex().c_str());
        return false;
    }
    auto cursor_header = finalized_execution_header_info;
    auto cursor_header_hash = finalized_header.execution_block_hash;

    for (;;) {
        del_unfinalized_headers(state, cursor_header_hash);
        if (false == set_finalized_execution_blocks(state, cursor_header.block_number, cursor_header_hash)) {
            xwarn("xtop_evm_eth2_client_contract::update_finalized_header set_finalized_execution_blocks error, %lu, %s",
                  cursor_header.block_number,
                  cursor_header_hash.hex().c_str());
            return false;
        }
        auto const & finalized_beacon_header = get_finalized_beacon_header(state);
        if (finalized_beacon_header.empty()) {
            xwarn("xtop_evm_eth2_client_contract::update_finalized_header get_finalized_beacon_header empty");
            return false;
        }
        if (cursor_header.parent_hash == finalized_beacon_header.execution_block_hash) {
            break;
        }
        cursor_header_hash = cursor_header.parent_hash;
        cursor_header = get_unfinalized_headers(state, cursor_header_hash);
        if (cursor_header.empty()) {
            xwarn("xtop_evm_eth2_client_contract::update_finalized_header get_unfinalized_headers %s empty", cursor_header_hash.hex().c_str());
            return false;
        }
    }
    if (false == set_finalized_beacon_header(state, finalized_header)) {
        xwarn("xtop_evm_eth2_client_contract::update_finalized_header set_finalized_beacon_header %s error", finalized_header.beacon_block_root.hex().c_str());
        return false;
    }
    if (false == set_finalized_execution_header(state, finalized_execution_header_info)) {
        xwarn("xtop_evm_eth2_client_contract::update_finalized_header set_finalized_execution_header %lu error", finalized_execution_header_info.block_number);
        return false;
    }
    if (finalized_execution_header_info.block_number > hashes_gc_threshold) {
        release_finalized_execution_blocks(state, finalized_execution_header_info.block_number - hashes_gc_threshold);
    }
    xinfo("xtop_evm_eth2_client_contract::update_finalized_header %s success", finalized_header.beacon_block_root.hex().c_str());
    return true;
}

bool xtop_evm_eth2_client_contract::commit_light_client_update(state_ptr const & state, xlight_client_update_t const & update) {
    auto const & finalized_header_update = update.finality_update.header_update;
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::commit_light_client_update get_finalized_beacon_header empty");
        return false;
    }
    auto const & finalized_period = compute_sync_committee_period(finalized_beacon_header.header.slot);
    auto const & update_period = compute_sync_committee_period(finalized_header_update.beacon_header.slot);
    if (update_period == finalized_period + 1) {
        auto const & next_sync_committee = get_next_sync_committee(state);
        if (next_sync_committee.empty()) {
            xwarn("xtop_evm_eth2_client_contract::commit_light_client_update get_next_sync_committee error");
            return false;
        }
        if (false == set_current_sync_committee(state, next_sync_committee)) {
            xwarn("xtop_evm_eth2_client_contract::commit_light_client_update set_current_sync_committee error");
            return false;
        }
        if (false == set_next_sync_committee(state, update.sync_committee_update.next_sync_committee)) {
            xwarn("xtop_evm_eth2_client_contract::commit_light_client_update set_next_sync_committee error");
            return false;
        }
    }
    xextended_beacon_block_header_t header;
    header.header = finalized_header_update.beacon_header;
    header.beacon_block_root = finalized_header_update.beacon_header.tree_hash_root();
    header.execution_block_hash = finalized_header_update.execution_block_hash;
    if (false == update_finalized_header(state, header)) {
        xwarn("xtop_evm_eth2_client_contract::commit_light_client_update update_finalized_header error");
        return false;
    }
    return true;
}

void xtop_evm_eth2_client_contract::release_finalized_execution_blocks(state_ptr const & state, uint64_t number) {
    for (;;) {
        if (del_finalized_execution_blocks(state, number) == true) {
            if (number == 0) {
                break;
            } else {
                number -= 1;
            }
        } else {
            break;
        }
    }
}

h256 xtop_evm_eth2_client_contract::get_finalized_execution_blocks(state_ptr const & state, uint64_t const height) const {
    auto str = state->map_get(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, std::to_string(height));
    if (str.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_finalized_execution_blocks map_get empty, height: %lu", height);
        return {};
    }
    return static_cast<h256>(xbytes_t{std::begin(str), std::end(str)});
}

bool xtop_evm_eth2_client_contract::set_finalized_execution_blocks(state_ptr const & state, uint64_t const height, h256 const & hash) {
    auto v = hash.asBytes();
    if (0 != state->map_set(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, std::to_string(height), {v.begin(), v.end()})) {
        xwarn("xtop_evm_eth2_client_contract::set_finalized_execution_blocks map_set error, height: %lu", height);
        return false;
    }
    return true;
}

bool xtop_evm_eth2_client_contract::del_finalized_execution_blocks(state_ptr const & state, uint64_t const height) {
    if (0 != state->map_remove(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, std::to_string(height))) {
        return false;
    }
    return true;
}

xexecution_header_info_t xtop_evm_eth2_client_contract::get_unfinalized_headers(state_ptr const & state, h256 const & hash) const {
    auto k = hash.asBytes();
    auto str = state->map_get(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS, {k.begin(), k.end()});
    if (str.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_headers map_get empty, hash: %s", hash.hex().c_str());
        return {};
    }
    xexecution_header_info_t info;
    if (info.decode_rlp({std::begin(str), std::end(str)}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_headers decode error, hash: %s", hash.hex().c_str());
        return {};
    }
    return info;
}

bool xtop_evm_eth2_client_contract::set_unfinalized_headers(state_ptr const & state, h256 const & hash, xexecution_header_info_t const & info) {
    auto k = hash.asBytes();
    auto v = info.encode_rlp();
    if (0 != state->map_set(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        xwarn("xtop_evm_eth2_client_contract::set_unfinalized_headers map_set error, hash: %s", hash.hex().c_str());
        return false;
    }
    return true;
}

bool xtop_evm_eth2_client_contract::del_unfinalized_headers(state_ptr const & state, h256 const & hash) {
    auto k = hash.to_bytes();
    if (0 != state->map_remove(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

xextended_beacon_block_header_t xtop_evm_eth2_client_contract::get_finalized_beacon_header(state_ptr const & state) const {
    auto const & v = state->string_get(data::system_contract::XPROPERTY_FINALIZED_BEACON_HEADER);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_finalized_beacon_header empty");
        return {};
    }
    xextended_beacon_block_header_t beacon;
    if (beacon.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_finalized_beacon_header decode error");
    }
    return beacon;
}

bool xtop_evm_eth2_client_contract::set_finalized_beacon_header(state_ptr const & state, xextended_beacon_block_header_t const & beacon) {
    auto const & v = beacon.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_FINALIZED_BEACON_HEADER, {v.begin(), v.end()}) != 0) {
        xwarn("xtop_evm_eth2_client_contract::set_finalized_beacon_header string_set error");
        return false;
    }
    return true;
}

xexecution_header_info_t xtop_evm_eth2_client_contract::get_finalized_execution_header(state_ptr const & state) const {
    auto const & v = state->string_get(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_HEADER);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_finalized_execution_header empty");
        return {};
    }
    xexecution_header_info_t info;
    if (info.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_finalized_execution_header decode error");
    }
    return info;
}

bool xtop_evm_eth2_client_contract::set_finalized_execution_header(state_ptr const & state, xexecution_header_info_t const & info) {
    auto const & v = info.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_HEADER, {v.begin(), v.end()}) != 0) {
        xwarn("xtop_evm_eth2_client_contract::set_finalized_execution_header string_set error");
        return false;
    }
    return true;
}

xsync_committee_t xtop_evm_eth2_client_contract::get_current_sync_committee(state_ptr const & state) const {
    auto const & v = state->string_get(data::system_contract::XPROPERTY_CURRENT_SYNC_COMMITTEE);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_current_sync_committee empty");
        return {};
    }
    xsync_committee_t committee;
    if (committee.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_current_sync_committee decode error");
    }
    return committee;
}

bool xtop_evm_eth2_client_contract::set_current_sync_committee(state_ptr const & state, xsync_committee_t const & committee) {
    auto const & v = committee.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_CURRENT_SYNC_COMMITTEE, {v.begin(), v.end()}) != 0) {
        xwarn("xtop_evm_eth2_client_contract::set_current_sync_committee string_set error");
        return false;
    }
    return true;
}

xsync_committee_t xtop_evm_eth2_client_contract::get_next_sync_committee(state_ptr const & state) const {
    auto const & v = state->string_get(data::system_contract::XPROPERTY_NEXT_SYNC_COMMITTEE);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_next_sync_committee empty");
        return {};
    }
    xsync_committee_t committee;
    if (committee.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_next_sync_committee decode error");
    }
    return committee;
}

bool xtop_evm_eth2_client_contract::set_next_sync_committee(state_ptr const & state, xsync_committee_t const & committee) {
    auto const & v = committee.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_NEXT_SYNC_COMMITTEE, {v.begin(), v.end()}) != 0) {
        xwarn("xtop_evm_eth2_client_contract::set_next_sync_committee string_set error");
        return false;
    }
    return true;
}

int xtop_evm_eth2_client_contract::get_flag(state_ptr state) const {
    auto flag_str = state->string_get(data::system_contract::XPROPERTY_RESET_FLAG);
    if (flag_str.empty()) {
        return 0;
    }
    return from_string<int>(flag_str);
}

bool xtop_evm_eth2_client_contract::set_flag(state_ptr state) {
    if (0 != state->string_set(data::system_contract::XPROPERTY_RESET_FLAG, top::to_string(1))) {
        return false;
    }
    return true;
}

NS_END4
