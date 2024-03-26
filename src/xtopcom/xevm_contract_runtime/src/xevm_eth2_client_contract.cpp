// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_eth2_client_contract.h"

#include "xcommon/common_data.h"
#include "xcommon/xeth_address.h"
#include "xdata/xdata_error.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/xabi_decoder.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)
using namespace evm_common::eth2;


constexpr static uint32_t floorlog2(uint32_t x) {
    return x == 0 ? 0 : 31 - __builtin_clz(x);
}

constexpr static uint32_t get_subtree_index(uint32_t generalized_index) {
    return generalized_index % (1 << floorlog2(generalized_index));
}


constexpr static uint64_t MIN_SYNC_COMMITTEE_PARTICIPANTS{1};

constexpr static uint32_t FINALIZED_ROOT_INDEX{105};
constexpr static uint32_t NEXT_SYNC_COMMITTEE_INDEX{55};
constexpr static uint32_t FINALITY_TREE_DEPTH{floorlog2(FINALIZED_ROOT_INDEX)};
constexpr static uint32_t FINALITY_TREE_INDEX{get_subtree_index(FINALIZED_ROOT_INDEX)};
constexpr static uint32_t SYNC_COMMITTEE_TREE_DEPTH{floorlog2(NEXT_SYNC_COMMITTEE_INDEX)};
constexpr static uint32_t SYNC_COMMITTEE_TREE_INDEX{get_subtree_index(NEXT_SYNC_COMMITTEE_INDEX)};

constexpr uint64_t hashes_gc_threshold = 51000;

//static std::string covert_committee_bits_to_bin_str(xbytes_t const & bits) {
//    assert(bits.size() % 2 == 0);
//    auto bin_str = data::hex_str_to_bin_str({bits.begin(), bits.end()});
//    assert(bin_str.size() % 8 == 0);
//    std::string bin_str_lsb;
//    for (auto it = bin_str.begin(); it != bin_str.end(); it += 8) {
//        for (auto i = 0; i < 8; i++) {
//            bin_str_lsb.push_back(*(it + 7 - i));
//        }
//    }
//    return bin_str_lsb;
//}

static std::vector<xbytes48_t> get_participant_pubkeys(std::vector<xbytes48_t> const & public_keys,
                                                       xbitset_t<evm_common::eth2::SYNC_COMMITTEE_BITS_SIZE> const & sync_committee_bits) {
    std::vector<xbytes48_t> ret;
    for (size_t i = 0; i < sync_committee_bits.size(); ++i) {
        if (sync_committee_bits[i]) {
            ret.push_back(public_keys[i]);
        }
    }
    return ret;
}

xtop_evm_eth2_client_contract::xtop_evm_eth2_client_contract() : m_network{evm_common::eth2::xnetwork_id_t::mainnet} {
    m_whitelist = load_whitelist();
    if (m_whitelist.empty()) {
        xwarn("[xtop_evm_eth2_client_contract] whitelist empty!");
    }
    xinfo("xtop_evm_eth2_client_contract network: %u", m_network);
}

xtop_evm_eth2_client_contract::xtop_evm_eth2_client_contract(evm_common::eth2::xnetwork_id_t const version) : m_network(version) {
    m_whitelist = load_whitelist();
    if (m_whitelist.empty()) {
        xwarn("[xtop_evm_eth2_client_contract] whitelist empty!");
    }
    xinfo("xtop_evm_eth2_client_contract network: %u", m_network);
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
    for (auto const & item : list) {
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
    // "0x4ddf47d4": "init(bytes)",
    // "0x158ef93e": "initialized()",
    // "0x3bcdaaab": "block_hash_safe(uint64)",
    // "0x55b39f6e": "finalized_beacon_block_header()",
    // "0x4b469132": "finalized_beacon_block_root()",
    // "0x074b1681": "finalized_beacon_block_slot()",
    // "0x3ae8d743": "get_light_client_state()",
    // "0xd398572f": "is_confirmed(uint256,bytes32)",
    // "0x7120345a": "is_known_execution_header(uint64)",
    // "0x1eeaebb2": "last_block_number()",
    // "0x2e139f0c": "submit_beacon_chain_light_client_update(bytes)",
    // "0xe9211822": "submit_execution_headers(bytes)",
    // "0xd826f88f": "reset()",
    // "0xb5a61069": "disable_reset()",
    // "0x0968a773": "get_client_mode()",
    // "0xf6d1f443": "get_unfinalized_tail_block_number()"
    //--------------------------------------------------------------

    constexpr uint32_t method_id_init{0x4ddf47d4};
    constexpr uint32_t method_id_initialized{0x158ef93e};
    constexpr uint32_t method_id_block_hash_safe{0x3bcdaaab};
    constexpr uint32_t method_id_finalized_beacon_block_header{0x55b39f6e};
    constexpr uint32_t method_id_finalized_beacon_block_root{0x4b469132};
    constexpr uint32_t method_id_finalized_beacon_block_slot{0x074b1681};
    constexpr uint32_t method_id_get_light_client_state{0x3ae8d743};
    constexpr uint32_t method_id_is_confirmed{0xd398572f};
    constexpr uint32_t method_id_is_known_execution_header{0x7120345a};
    constexpr uint32_t method_id_last_block_number{0x1eeaebb2};
    constexpr uint32_t method_id_submit_beacon_chain_light_client_update{0x2e139f0c};
    constexpr uint32_t method_id_submit_execution_headers{0xe9211822};
    constexpr uint32_t method_id_reset{0xd826f88f};
    constexpr uint32_t method_id_disable_reset{0xb5a61069};
    constexpr uint32_t method_id_get_client_mode{0x0968a773};
    constexpr uint32_t method_id_get_unfinalized_tail_block_number{0xf6d1f443};

    // check param
    assert(state_ctx);
    auto state = state_ctx->load_unit_state(common::xaccount_address_t::build_from(context.address, base::enum_vaccount_addr_type_secp256k1_evm_user_account));
    if (state == nullptr) {
        xwarn("[xtop_evm_eth2_client_contract::execute] state nullptr");
        return false;
    }
    if (input.empty()) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth2_client_contract::execute] invalid input");
        return false;
    }
    std::error_code ec;
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(input, ec);
    if (ec) {
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth2_client_contract::execute] illegal input data");
        return false;
    }
    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::fatal;
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
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] init is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        xinit_input_t init_put;
        if (false == init_put.decode_rlp(headers_rlp)) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] init_put decode error");
            return false;
        }
        if (!init(state, init_put, context.caller)) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] init headers error");
            return false;
        }
        xh256s_t topics;
        topics.emplace_back();
        context.caller.to_h256(topics.back());
        // topics.push_back(xh256_t(context.caller.to_h256()));
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
            err.fail_status = precompile_error::revert;
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
        auto const height = abi_decoder.extract<uint64_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
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
        auto const block_height = abi_decoder.extract<uint64_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        uint32_t is_known{0};
        if (is_known_execution_header(state, block_height)) {
            is_known = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<evm_common::u256>(is_known));
        return true;
    }
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
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] method_id_submit_beacon_chain_light_client_update is not allowed in static context");
            return false;
        }
        auto update_bytes = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        xlight_client_update_t update;
        update.decode_rlp(update_bytes, ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("xtop_evm_eth2_client_contract::execute, submit_beacon_chain_light_client_update failed, msg %s", ec.message().c_str());
            return false;
        }
        if (!submit_beacon_chain_light_client_update(state, update)) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] submit_beacon_chain_light_client_update error");
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
    case method_id_submit_execution_headers: {
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
        if (!m_whitelist.count(context.caller.to_hex_string())) {
#else
        if (!m_whitelist.empty() && !m_whitelist.count(context.caller.to_hex_string())) {
#endif
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] method_id_submit_execution_header is not allowed in static context");
            return false;
        }

        auto bytes = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        // evm_common::xeth_header_t header;
        auto left_bytes = std::move(bytes);
        size_t count = 0;
        while (!left_bytes.empty()) {
            evm_common::RLP::DecodedItem item = evm_common::RLP::decode(left_bytes);

            if (item.decoded.size() != 5) {
                xwarn("xtop_evm_eth2_client_contract::execute, decode error, decoding xeth_header_t at %zu, decoded item count %zu", count, item.decoded.size());
                err.fail_status = precompile_error::revert;
                err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
                return false;
            }
            ++count;
            left_bytes = std::move(item.remainder);

            xeth_header_t header;
            {
                auto const & item_header = evm_common::RLP::decode_once(item.decoded[0]);
                if (item_header.decoded.size() != 1) {
                    xwarn("xtop_evm_eth2_client_contract::execute, decode_once error, size %zu", item_header.decoded.size());
                    err.fail_status = precompile_error::revert;
                    err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
                    return false;
                }

                auto header_bytes = item_header.decoded[0];
                header.decode_rlp(header_bytes, ec);
                if (ec) {
                    err.fail_status = precompile_error::revert;
                    err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
                    xwarn("xtop_evm_eth2_client_contract::execute, xeth_header_t::decode_rlp failed, msg %s", ec.message().c_str());
                    return false;
                }
            }
            if (!submit_execution_header(state, header, context.caller)) {
                err.fail_status = precompile_error::revert;
                err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
                xwarn("[xtop_evm_eth2_client_contract::execute] submit_execution_header error");
                return false;
            }
        }
        xh256s_t topics;
        topics.push_back(xh256_t(context.caller.to_h256()));
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
            err.fail_status = precompile_error::revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth2_client_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (!reset(state)) {
            err.fail_status = precompile_error::revert;
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
            err.fail_status = precompile_error::revert;
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
    case method_id_get_client_mode: {
        auto const mode = get_client_mode(state);

        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(static_cast<uint8_t>(mode)));

        return true;
    }

    case method_id_get_unfinalized_tail_block_number: {
        auto const header = get_unfinalized_tail_block_number(state);

        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(header));

        return true;
    }

    default: {
        xwarn("[xtop_evm_eth2_client_contract::execute] not found method id: 0x%x", function_selector.method_id);
        err.fail_status = precompile_error::fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);
        return false;
    }
    }

    return true;
}

bool xtop_evm_eth2_client_contract::init(state_ptr const & state, xinit_input_t const & init_input, common::xeth_address_t const & sender) {
    assert(!sender.is_zero());

    if (initialized(state)) {
        xwarn("xtop_evm_eth2_client_contract::init already");
        return false;
    }
    xh256_t hash;
    init_input.finalized_execution_header.calc_hash(hash);
    if (hash != init_input.finalized_beacon_header.execution_block_hash) {
        xwarn("xtop_evm_eth2_client_contract::init hash mismatch %s, %s", hash.hex().c_str(), init_input.finalized_beacon_header.execution_block_hash.hex().c_str());
        return false;
    }

    auto const & finalized_execution_header_info =
        xexecution_header_info_t{init_input.finalized_execution_header.parent_hash, init_input.finalized_execution_header.number, sender};
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
    if (false == client_mode(state, xclient_mode_t::submit_light_client_update)) {
        xwarn("xtop_evm_eth2_client_contract::init set client mode error");
        return false;
    }

    if (false == add_finalized_execution_blocks(state, init_input.finalized_execution_header.number, hash)) {
        xwarn("xtop_evm_eth2_client_contract::init set finalized execution header number => hash failed");
        return false;
    }

    xinfo("xtop_evm_eth2_client_contract::init success");
    return true;
}

bool xtop_evm_eth2_client_contract::initialized(state_ptr const & state) const {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    return !finalized_beacon_header.empty();
}

bool xtop_evm_eth2_client_contract::reset(state_ptr state) {
    if (get_flag(state) != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::reset] reset already disabled");
        return false;
    }

    state->map_clear(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS);

    if (create_unfinalized_head_execution_header_property_if_necessary(state)) {
        xwarn("[xtop_evm_eth_bridge_contract::reset] create_unfinalized_head_execution_header_property_if_necessary failed");
        return false;
    }
    state->string_set(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER, std::string{});

    if (create_unfinalized_tail_execution_header_property_if_necessary(state)) {
        xwarn("[xtop_evm_eth_bridge_contract::reset] create_unfinalized_tail_execution_header_property_if_necessary failed");
        return false;
    }
    state->string_set(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER, std::string{});

    xextended_beacon_block_header_t beacon_block;
    auto beacon_header_bytes = beacon_block.encode_rlp();
    state->string_set(data::system_contract::XPROPERTY_FINALIZED_BEACON_HEADER, {beacon_header_bytes.begin(), beacon_header_bytes.end()});
    xexecution_header_info_t execution_header;
    auto execution_header_bytes = execution_header.encode_rlp();
    state->string_set(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_HEADER, {execution_header_bytes.begin(), execution_header_bytes.end()});
    if (create_client_mode_property_if_necessary(state)) {
        xwarn("[xtop_evm_eth_bridge_contract::reset] create_client_mode_property_if_necessary failed");
        return false;
    }
    state->uint64_set(data::system_contract::XPROPERTY_CLIENT_MODE, static_cast<uint64_t>(xclient_mode_t::invalid));
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

xh256_t xtop_evm_eth2_client_contract::block_hash_safe(state_ptr const & state, uint64_t const block_number) const {
    auto const finalized_execution_header_block_number = last_block_number(state);
    if (block_number > finalized_execution_header_block_number) {
        xwarn("[xtop_evm_eth2_client_contract::block_hash_safe] block_number %" PRIu64 " > finalized_execution_header_block_number %" PRIu64, block_number, finalized_execution_header_block_number);
        return xh256_t{};
    }

    return get_finalized_execution_blocks(state, block_number);
}

bool xtop_evm_eth2_client_contract::is_known_execution_header(state_ptr const & state, uint64_t const height) const {
    auto const & header = get_finalized_execution_blocks(state, height);
    return !header.empty();
}

bool xtop_evm_eth2_client_contract::is_confirmed(state_ptr state, uint64_t const number, h256 const & hash_bytes) const {
    auto const & hash = hash_bytes;
    auto const & origin_hash = get_finalized_execution_blocks(state, number);
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
    if (!is_light_client_update_allowed(state)) {
        return false;
    }

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

bool xtop_evm_eth2_client_contract::submit_execution_header(state_ptr const & state, evm_common::xeth_header_t const & block_header, common::xeth_address_t const & sender) {
    assert(!sender.is_zero());
    if (get_client_mode(state) != xclient_mode_t::submit_header) {
        xwarn("xtop_evm_eth2_client_contract::submit_execution_header client_mode error");
        return false;
    }

    auto const & block_hash = block_header.calc_hash();
    auto const & unfinalized_tail_execution_header_info = get_unfinalized_tail_execution_header_info(state);
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (unfinalized_tail_execution_header_info.empty() && finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::submit_execution_header get_finalized_beacon_header empty");
        return false;
    }
    auto const & excepted_block_hash =
        !unfinalized_tail_execution_header_info.empty() ? unfinalized_tail_execution_header_info.parent_hash : finalized_beacon_header.execution_block_hash;
    if (block_hash != excepted_block_hash) {
        xwarn("xtop_evm_eth2_client_contract::submit_execution_header block_hash %s != excepted_block_hash %s", block_hash.hex().c_str(), excepted_block_hash.hex().c_str());
        return false;
    }

    if (!add_finalized_execution_blocks(state, block_header.number, block_hash)) {
        xwarn("xtop_evm_eth2_client_contract::submit_execution_header set_finalized_execution_blocks error: %s", block_hash.hex().c_str());
        return false;
    }

    auto const & finalized_execution_header = get_finalized_execution_header(state);
    if (finalized_execution_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::submit_execution_header get_finalized_execution_header empty");
        return false;
    }

    // Apply GC
    auto const diff_between_unfinalized_head_and_tail = get_diff_between_unfinalized_head_and_tail(state);
    if (diff_between_unfinalized_head_and_tail != 0) {
        auto const some_block_height = finalized_execution_header.block_number + diff_between_unfinalized_head_and_tail;
        if (some_block_height > hashes_gc_threshold) {
            auto const header_number_to_remove = some_block_height - hashes_gc_threshold;
            if (header_number_to_remove >= finalized_execution_header.block_number) {
                xwarn("xtop_evm_eth2_client_contract::submit_execution_header header_number_to_remove %" PRIu64 " >= finalized_execution_header.block_number %" PRIu64,
                      header_number_to_remove,
                      finalized_execution_header.block_number);
                return false;
            }

            if (header_number_to_remove > 0) {
                gc_finalized_execution_blocks(state, header_number_to_remove);
            }
        }
    }

    if (block_header.number == finalized_execution_header.block_number + 1) {
        auto const & finalized_execution_header_hash = get_finalized_execution_blocks(state, finalized_execution_header.block_number);
        if (block_header.parent_hash != finalized_execution_header_hash) {
             xwarn("xtop_evm_eth2_client_contract::submit_execution_header block_header.parent_hash %s != finalized_execution_header_hash %s",
                   block_header.parent_hash.hex().c_str(),
                   finalized_execution_header_hash.hex().c_str());
             return false;
        }

        if (create_unfinalized_head_execution_header_property_if_necessary(state)) {
            xwarn("xtop_evm_eth2_client_contract::submit_execution_header create_unfinalized_head_execution_header_property_if_necessary error");
            return false;
        }
        set_finalized_execution_header_bytes(state, state->string_get(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER));
        reset_unfinalized_tail_execution_header(state);
        reset_unfinalized_head_execution_header(state);

        client_mode(state, xclient_mode_t::submit_light_client_update);
    } else {
        xexecution_header_info_t block_info;
        block_info.parent_hash = block_header.parent_hash;
        block_info.block_number = block_header.number;
        block_info.submitter = sender;

        auto const & unfinalized_head_execution_header_info = get_unfinalized_head_execution_header_info(state);
        if (unfinalized_head_execution_header_info.empty()) {
             set_unfinalized_head_execution_header_info(state, block_info);
        }
        set_unfinalized_tail_execution_header_info(state, block_info);
    }

    xinfo("xtop_evm_eth2_client_contract::submit_execution_header header %s set succss", block_hash.hex().c_str());
    return true;
}

bool xtop_evm_eth2_client_contract::validate_light_client_update(state_ptr const & state, xlight_client_update_t const & update) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::validate_light_client_update get_finalized_beacon_header empty");
        return false;
    }

    auto const finalized_period = compute_sync_committee_period(finalized_beacon_header.header.slot);
    if (false == verify_finality_branch(state, update, finalized_period)) {
        xwarn("xtop_evm_eth2_client_contract::validate_light_client_update verify_finality_branch error");
        return false;
    }

    // Verify sync committee has sufficient participants
    auto const & sync_committee_bits = update.sync_aggregate.sync_committee_bits;
    assert(sync_committee_bits.size() == SYNC_COMMITTEE_BITS_SIZE);

    size_t const sync_committee_bits_sum{ sync_committee_bits.count()};
    if (sync_committee_bits_sum < MIN_SYNC_COMMITTEE_PARTICIPANTS) {
        xwarn("xtop_evm_eth2_client_contract::validate_light_client_update error sync_committee_bits_sum: %zu", sync_committee_bits_sum);
        return false;
    }
    if (sync_committee_bits_sum * 3 < (sync_committee_bits.size() * 2)) {
        xwarn("xtop_evm_eth2_client_contract::validate_light_client_update committee bits sum is less than 2/3 threshold, %zu, %zu",
              sync_committee_bits_sum,
              sync_committee_bits.size());
        return false;
    }

    if (false == verify_bls_signatures(state, update, sync_committee_bits, finalized_period)) {
        xwarn("xtop_evm_eth2_client_contract::validate_light_client_update verify_bls_signatures error");
        return false;
    }
    xinfo("xtop_evm_eth2_client_contract::validate_light_client_update validate update success");
    return true;
}

bool xtop_evm_eth2_client_contract::verify_finality_branch(state_ptr const & state, xlight_client_update_t const & update, uint64_t const finalized_period) const {
    auto const & active_header = update.finality_update.header_update.beacon_header;
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch get_finalized_beacon_header empty");
        return false;
    }
    if (active_header.slot <= finalized_beacon_header.header.slot) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch slot mismatch, %" PRIu64 ", %" PRIu64, active_header.slot, finalized_beacon_header.header.slot);
        return false;
    }
    if (update.attested_beacon_header.slot < update.finality_update.header_update.beacon_header.slot) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch slot mismatch, %" PRIu64 ", %" PRIu64, active_header.slot, finalized_beacon_header.header.slot);
        return false;
    }
    if (update.signature_slot <= update.attested_beacon_header.slot) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch slot mismatch, %" PRIu64 ", %" PRIu64, update.signature_slot, update.attested_beacon_header.slot);
        return false;
    }
    auto const update_period = compute_sync_committee_period(active_header.slot);
    if (update_period != finalized_period && update_period != finalized_period + 1) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch period mismatch, acceptable periods are '%" PRIu64 "' and '%" PRIu64 "' but got %" PRIu64,
              finalized_period,
              finalized_period + 1,
              update_period);
        return false;
    }

    // Verify that the `finality_branch`, confirms `finalized_header`
    // to match the finalized checkpoint root saved in the state of `attested_header`.
    xbytes_t branch_data;
    for (auto const & b : update.finality_update.finality_branch) {
        branch_data.insert(branch_data.end(), b.begin(), b.end());
    }
    xh256_t merkle_root;
    if (false == unsafe_merkle_proof(update.finality_update.header_update.beacon_header.tree_hash_root().data(),
                                     branch_data.data(),
                                     branch_data.size(),
                                     FINALITY_TREE_DEPTH,
                                     FINALITY_TREE_INDEX,
                                     merkle_root.data())) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch header unsafe_merkle_proof error");
        return false;
    }
    if (merkle_root != update.attested_beacon_header.state_root) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch header root error: %s, %s",
              to_hex(merkle_root).c_str(),
              to_hex(update.attested_beacon_header.state_root).c_str());
        return false;
    }

    xnetwork_config_t const config{m_network};
    if (false == validate_beacon_block_header_update(config, update.finality_update.header_update)) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch validate_beacon_block_header_update error");
        return false;
    }

    // Verify that the `next_sync_committee`, if present, actually is the next sync committee saved in the
    // state of the `active_header`
    if (update_period != finalized_period) {
        branch_data.clear();
        for (auto const & b : update.sync_committee_update.next_sync_committee_branch) {
            branch_data.insert(branch_data.end(), b.begin(), b.end());
        }
        merkle_root.clear();
        if (false == unsafe_merkle_proof(update.sync_committee_update.next_sync_committee.tree_hash_root().data(),
                                         branch_data.data(),
                                         branch_data.size(),
                                         SYNC_COMMITTEE_TREE_DEPTH,
                                         SYNC_COMMITTEE_TREE_INDEX,
                                         merkle_root.data())) {
            xwarn("xtop_evm_eth2_client_contract::verify_finality_branch committee unsafe_merkle_proof error");
            return false;
        }

        xdbg("next_sync_committee.state_root %s, beacon_header.state_root %s, attested_beacon_header.state_root %s",
             to_hex(merkle_root).c_str(),
             to_hex(active_header.state_root).c_str(),
             to_hex(update.attested_beacon_header.state_root).c_str());

        if (merkle_root != update.attested_beacon_header.state_root) {
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
                                                          xbitset_t<evm_common::eth2::SYNC_COMMITTEE_BITS_SIZE> const & sync_committee_bits,
                                                          uint64_t const finalized_period) {
    auto const signature_period = compute_sync_committee_period(update.signature_slot);
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
    auto const & participant_pubkeys = get_participant_pubkeys(sync_committee.pubkeys(), sync_committee_bits);
    xbytes_t signing_root(32);
    unsafe_compute_committee_signing_root(update.attested_beacon_header.tree_hash_root().data(), uint8_t(m_network), update.signature_slot, signing_root.data());
    xbytes_t pubkeys_data;
    for (auto const & pubkey : participant_pubkeys) {
        pubkeys_data.insert(pubkeys_data.begin(), pubkey.begin(), pubkey.end());
    }
    return unsafe_verify_bls_signatures(update.sync_aggregate.sync_committee_signature.data(), pubkeys_data.data(), pubkeys_data.size(), signing_root.data());
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
    if (false == set_finalized_beacon_header(state, header)) {
        xwarn("xtop_evm_eth2_client_contract::commit_light_client_update set_finalized_beacon_header error");
        return false;
    }
    if (false == client_mode(state, xclient_mode_t::submit_header)) {
        xwarn("xtop_evm_eth2_client_contract::commit_light_client_update client_mode error");
        return false;
    }
    return true;
}

void xtop_evm_eth2_client_contract::gc_finalized_execution_blocks(state_ptr const & state, uint64_t number) {
    for (;;) {
        if (del_finalized_execution_blocks(state, number)) {
            if (number == 0) {
                break;
            }

            number -= 1;
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

bool xtop_evm_eth2_client_contract::add_finalized_execution_blocks(state_ptr const & state, uint64_t const height, h256 const & hash) {
    auto v = hash.asBytes();
    if (0 != state->map_set(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, std::to_string(height), {v.begin(), v.end()})) {
        xwarn("xtop_evm_eth2_client_contract::set_finalized_execution_blocks map_set error, height: %" PRIu64, height);
        return false;
    }
    return true;
}

bool xtop_evm_eth2_client_contract::del_finalized_execution_blocks(state_ptr const & state, uint64_t const height) {
    return !state->map_remove(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, std::to_string(height));
}

xextended_beacon_block_header_t xtop_evm_eth2_client_contract::get_finalized_beacon_header(state_ptr const & state) {
    auto const & v = state->string_get(data::system_contract::XPROPERTY_FINALIZED_BEACON_HEADER);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_finalized_beacon_header empty");
        return {};
    }
    xextended_beacon_block_header_t beacon;
    if (beacon.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_finalized_beacon_header decode error");
        return {};
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
        return {};
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

bool xtop_evm_eth2_client_contract::set_finalized_execution_header_bytes(state_ptr const & state, std::string const & bytes) {
    assert(!bytes.empty());
    if (state->string_set(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_HEADER, bytes) != 0) {
        xwarn("xtop_evm_eth2_client_contract::set_current_execution_header string_set error");
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

xclient_mode_t xtop_evm_eth2_client_contract::get_client_mode(state_ptr state) const {
    if (!state->property_exist(data::system_contract::XPROPERTY_CLIENT_MODE)) {
        xwarn("xtop_evm_eth2_client_contract::get_client_mode, data::system_contract::XPROPERTY_CLIENT_MODE property not exist");
        return xclient_mode_t::invalid;
    }

    auto const mode = static_cast<evm_common::eth2::xclient_mode_t>(state->uint64_property_get(data::system_contract::XPROPERTY_CLIENT_MODE));
    if (mode == xclient_mode_t::invalid) {
        xwarn("xtop_evm_eth2_client_contract::get_client_mode failed");
    }

    return mode;
}

bool xtop_evm_eth2_client_contract::client_mode(state_ptr state, evm_common::eth2::xclient_mode_t const mode) {
    assert(state != nullptr);
    if (create_client_mode_property_if_necessary(state)) {
        xwarn("xtop_evm_eth2_client_contract::client_mode create_client_mode_property_if_necessary failed");
        return false;
    }

    auto const ec = state->uint64_set(data::system_contract::XPROPERTY_CLIENT_MODE, static_cast<uint64_t>(mode));
    if (ec) {
        xwarn("xtop_evm_eth2_client_contract::client_mode set mode failed");
        return false;
    }

    return true;
}

uint64_t xtop_evm_eth2_client_contract::get_unfinalized_tail_block_number(state_ptr state) const {
    if (!state->property_exist(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER)) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_tail_block_number, data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER property not exist");
        return 0;
    }

    auto const & v = state->string_get(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_tail_block_number empty");
        return 0;
    }

    xexecution_header_info_t info;
    if (false == info.decode_rlp({v.begin(), v.end()})) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_tail_block_number decode error");
        return 0;
    }

    return info.block_number;
}

xexecution_header_info_t xtop_evm_eth2_client_contract::get_unfinalized_tail_execution_header_info(state_ptr const & state) const {
    if (!state->property_exist(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER)) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_tail_block_number, data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER property not exist");
        return {};
    }

    auto const & v = state->string_get(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_tail_execution_header_info empty");
        return {};
    }
    xexecution_header_info_t info;
    if (info.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_tail_execution_header_info decode error");
        return {};
    }
    return info;
}

evm_common::eth2::xexecution_header_info_t xtop_evm_eth2_client_contract::get_unfinalized_head_execution_header_info(state_ptr const & state) const {
    if (!state->property_exist(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER)) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_head_execution_header_info, data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER property not exist");
        return {};
    }

    auto const & v = state->string_get(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_head_execution_header_info empty");
        return {};
    }
    xexecution_header_info_t info;
    if (info.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_head_execution_header_info decode error");
        return {};
    }
    return info;
}

uint64_t xtop_evm_eth2_client_contract::get_diff_between_unfinalized_head_and_tail(state_ptr state) const {
    assert(state != nullptr);
    if (!state->property_exist(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER)) {
        xwarn("xtop_evm_eth2_client_contract::get_diff_between_unfinalized_head_and_tail, data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER property not exist");
        return 0;
    }

    if (!state->property_exist(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER)) {
        xwarn("xtop_evm_eth2_client_contract::get_diff_between_unfinalized_head_and_tail, data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER property not exist");
        return 0;
    }

    auto const & header_data = state->string_get(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER);
    if (header_data.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_diff_between_unfinalized_head_and_tail empty");
        return 0;
    }
    auto const & tail_data = state->string_get(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER);
    if (tail_data.empty()) {
        xwarn("xtop_evm_eth2_client_contract::get_unfinalized_tail_execution_header_number empty");
        return 0;
    }

    xexecution_header_info_t header;
    if (header.decode_rlp({header_data.begin(), header_data.end()}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_diff_between_unfinalized_head_and_tail decode header error");
        return 0;
    }

    xexecution_header_info_t tail;
    if (tail.decode_rlp({tail_data.begin(), tail_data.end()}) == false) {
        xwarn("xtop_evm_eth2_client_contract::get_diff_between_unfinalized_head_and_tail decode tail error");
        return 0;
    }
    assert(header.block_number >= tail.block_number);

    return header.block_number - tail.block_number;
}

bool xtop_evm_eth2_client_contract::reset_unfinalized_head_execution_header(state_ptr const & state) {
    if (create_unfinalized_head_execution_header_property_if_necessary(state)) {
        xwarn("xtop_evm_eth2_client_contract::reset_unfinalized_head_execution_header create_unfinalized_head_execution_header_property_if_necessary error");
        return false;
    }

    if (state->string_set(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER, {}) != 0) {
        xwarn("xtop_evm_eth2_client_contract::set_unfinalized_tail_execution_header_info string_set error");
        return false;
    }

    return true;
}

bool xtop_evm_eth2_client_contract::reset_unfinalized_tail_execution_header(state_ptr const & state) {
    if (create_unfinalized_tail_execution_header_property_if_necessary(state)) {
        xwarn("xtop_evm_eth2_client_contract::reset_unfinalized_tail_execution_header create_unfinalized_tail_execution_header_property_if_necessary error");
        return false;
    }

    if (state->string_set(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER, {}) != 0) {
        xwarn("xtop_evm_eth2_client_contract::set_unfinalized_tail_execution_header_info string_set error");
        return false;
    }
    return true;
}

bool xtop_evm_eth2_client_contract::set_unfinalized_head_execution_header_info(state_ptr const & state, xexecution_header_info_t const & info) {
    if (create_unfinalized_head_execution_header_property_if_necessary(state)) {
        xwarn("xtop_evm_eth2_client_contract::set_unfinalized_head_execution_header_info create_unfinalized_head_execution_header_property_if_necessary error");
        return false;
    }

    auto bytes = info.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER, {bytes.begin(), bytes.end()}) != 0) {
        xwarn("xtop_evm_eth2_client_contract::set_unfinalized_head_execution_header_info string_set error");
        return false;
    }
    return true;
}

bool xtop_evm_eth2_client_contract::set_unfinalized_tail_execution_header_info(state_ptr const & state, xexecution_header_info_t const & info) {
    if (create_unfinalized_tail_execution_header_property_if_necessary(state)) {
        xwarn("xtop_evm_eth2_client_contract::set_unfinalized_tail_execution_header_info create_unfinalized_tail_execution_header_property_if_necessary error");
        return false;
    }

    auto bytes = info.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER, {bytes.begin(), bytes.end()}) != 0) {
        xwarn("xtop_evm_eth2_client_contract::set_unfinalized_tail_execution_header_info string_set error");
        return false;
    }
    return true;
}

bool xtop_evm_eth2_client_contract::is_light_client_update_allowed(state_ptr state) const {
    if (get_client_mode(state) != xclient_mode_t::submit_light_client_update) {
        xwarn("xtop_evm_eth2_client_contract::is_light_client_update_allowed client_mode error");
        return false;
    }

    return true;
}

bool xtop_evm_eth2_client_contract::validate_beacon_block_header_update(evm_common::eth2::xnetwork_config_t const & config, xheader_update_t const & header_update) const {
    std::vector<xh256_t> const & branch = header_update.execution_hash_branch;
    auto const & proof_size = config.compute_proof_size_by_slot(header_update.beacon_header.slot);

    //xinfo("xtop_evm_eth2_client_contract::validate_beacon_block_header_update. execution hash branch size:%zu execution proof size:%zu header update slot:%" PRIu64
    //      " header update epoch:%" PRIu64,
    //      branch.size(),
    //      proof_size.execution_proof_size,
    //      header_update.beacon_header.slot,
    //      compute_epoch_at_slot(header_update.beacon_header.slot));

    if (branch.size() != proof_size.execution_proof_size) {
        xwarn("xtop_evm_eth2_client_contract::validate_beacon_block_header_update execution_hash_branch size error. branch size: %zu, proof size:%zu", branch.size(), proof_size.execution_proof_size);
        return false;
    }

    xspan_t<xh256_t const> const l2_proof{&branch[0], &branch[proof_size.l2_execution_payload_proof_size]};
    xspan_t<xh256_t const> const l1_proof{&branch[proof_size.l2_execution_payload_proof_size], &branch[proof_size.execution_proof_size]};

    xh256_t execution_payload_hash;
    if (false == unsafe_merkle_proof(header_update.execution_block_hash.data(),
                                     l2_proof.data()->data(),
                                     l2_proof.size() * xh256_t::size(),
                                     proof_size.l2_execution_payload_proof_size,
                                     proof_size.l2_execution_payload_tree_execution_block_index,
                                     execution_payload_hash.data())) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch committee unsafe_merkle_proof error");
        return false;
    }

    xh256_t body_root;
    if (false == unsafe_merkle_proof(execution_payload_hash.data(),
                                     l1_proof.data()->data(),
                                     l1_proof.size() * xh256_t::size(),
                                     proof_size.beacon_block_body_tree_depth,
                                     proof_size.l1_beacon_block_body_tree_execution_payload_index,
                                     body_root.data())) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch committee unsafe_merkle_proof error");
        return false;
    }

    if (body_root != header_update.beacon_header.body_root) {
        xwarn("xtop_evm_eth2_client_contract::verify_finality_branch committee body_root error. calculated body root hash:%s, submitted body root hash:%s",
              to_hex(body_root).c_str(),
              to_hex(header_update.beacon_header.body_root).c_str());
        return false;
    }

    return true;
}

int32_t xtop_evm_eth2_client_contract::create_client_mode_property_if_necessary(state_ptr state) {
    assert(state != nullptr);
    auto const bstate = state->get_bstate();
    assert(bstate != nullptr);

    auto const ret = state->uint64_create(data::system_contract::XPROPERTY_CLIENT_MODE);
    if (ret && ret != data::xaccount_property_already_exist) {
        xwarn("xtop_evm_eth2_client_contract::create_client_mode_property_if_necessary uint64_create error");
        return ret;
    }

    return 0;
}

int32_t xtop_evm_eth2_client_contract::create_unfinalized_head_execution_header_property_if_necessary(state_ptr state) {
    assert(state != nullptr);
    auto const bstate = state->get_bstate();
    assert(bstate != nullptr);

    auto const ret = state->string_create(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER);
    if (ret && ret != data::xaccount_property_already_exist) {
        xwarn("xtop_evm_eth2_client_contract::create_unfinalized_head_execution_header_property_if_necessary uint64_create error");
        return ret;
    }

    return 0;
}

int32_t xtop_evm_eth2_client_contract::create_unfinalized_tail_execution_header_property_if_necessary(state_ptr state) {
    assert(state != nullptr);
    auto const bstate = state->get_bstate();
    assert(bstate != nullptr);

    auto const ret = state->string_create(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER);
    if (ret && ret != data::xaccount_property_already_exist) {
        xwarn("xtop_evm_eth2_client_contract::create_unfinalized_tail_execution_header_property_if_necessary uint64_create error");
        return ret;
    }

    return 0;
}


NS_END4
