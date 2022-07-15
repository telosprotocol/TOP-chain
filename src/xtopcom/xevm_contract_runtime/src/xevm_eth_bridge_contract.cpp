// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_eth_bridge_contract.h"

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xbasic/endianness.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xdata/xdata_common.h"
#include "xdata/xeth_bridge_whitelist.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xeth/xeth_config.h"
#include "xevm_common/xeth/xeth_eip1559.h"
#include "xevm_common/xeth/xeth_gaslimit.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_common/xeth/xethash.h"

#ifdef ETH_BRIDGE_TEST
#include <fstream>
#endif

// TODO
// 1 make sure context.address


NS_BEG4(top, contract_runtime, evm, sys_contract)

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

using namespace top::evm_common;

constexpr uint64_t MinGasLimit = 5000;
constexpr uint64_t MaxGasLimit = 0x7fffffffffffffff;
constexpr uint64_t MaximumExtraDataSize = 32;
constexpr uint64_t ConfirmHeight = 25;
constexpr uint64_t HashReserveNum = 40000;
constexpr uint64_t BlockReserveNum = 500;

static std::set<std::string> load_whitelist() {
    json j;
#ifdef ETH_BRIDGE_TEST
#    define WHITELIST_FILE "eth_bridge_whitelist.json"
    xinfo("[xtop_evm_eth_bridge_contract::load_whitelist] load from file %s", WHITELIST_FILE);
    std::ifstream data_file(WHITELIST_FILE);
    if (data_file.good()) {
        data_file >> j;
        data_file.close();
    } else {
        xwarn("[xtop_evm_eth_bridge_contract::load_whitelist] file %s open error", WHITELIST_FILE);
        return {};
    }
#else
    xinfo("[xtop_chain_checkpoint::load] load from code");
    j = json::parse(data::eth_bridge_whitelist());
#endif

    if (!j.count("whitelist")) {
        xwarn("[xtop_evm_eth_bridge_contract::load_whitelist] load whitelist failed");
        return {};
    }
    std::set<std::string> ret;
    auto const & list = j["whitelist"];
    for (auto const item : list) {
        ret.insert(item.get<std::string>());
    }
    return ret;
}

xtop_evm_eth_bridge_contract::xtop_evm_eth_bridge_contract() {
    m_whitelist = load_whitelist();
    if (m_whitelist.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::xtop_evm_eth_bridge_contract] m_whitelist empty!");
    }
}

bool xtop_evm_eth_bridge_contract::execute(xbytes_t input,
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
    m_contract_state = state_ctx->load_unit_state(m_contract_address.vaccount());
    if (m_contract_state == nullptr) {
        xwarn("[xtop_evm_eth_bridge_contract::execute] state nullptr");
        return false;
    }
    if (input.empty()) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth_bridge_contract::execute] invalid input");
        return false;
    }
    std::error_code ec;
    xabi_decoder_t abi_decoder = xabi_decoder_t::build_from(xbytes_t{std::begin(input), std::end(input)}, ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth_bridge_contract::execute] illegal input data");
        return false;
    }
    auto function_selector = abi_decoder.extract<xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth_bridge_contract::execute] illegal input function selector");
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::execute] caller: %s, address: %s, method_id: 0x%x, input size: %zu",
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
            xwarn("[xtop_evm_eth_bridge_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth_bridge_contract::execute] init is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        if (!init(headers_rlp)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth_bridge_contract::execute] init headers error");
            return false;
        }
        xh256s_t topics;
        topics.push_back(xh256_t(context.caller.to_h256()));
        xevm_log_t log(context.address, topics, top::to_bytes(u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        return true;
    }
    case method_id_sync: {
        if (is_static) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth_bridge_contract::execute] sync is not allowed in static context");
            return false;
        }
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract error");
            return false;
        }
        if (!sync(headers_rlp)) {
            err.fail_status = precompile_error::Revert;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitRevert::Reverted);
            xwarn("[xtop_evm_eth_bridge_contract::execute] sync headers error");
            return false;
        }
        xh256s_t topics;
        topics.push_back(xh256_t(context.caller.to_h256()));
        xevm_log_t log(context.address, topics, top::to_bytes(u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        return true;
    }
    case method_id_get_height: {
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(get_height()));
        return true;
    }
    case method_id_is_known: {
        auto height = abi_decoder.extract<u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        uint32_t known{0};
        if (is_known(height, hash_bytes)) {
            known = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(known));
        return true;
    }
    case method_id_is_confirmed: {
        auto height = abi_decoder.extract<u256>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        auto hash_bytes = abi_decoder.decode_bytes(32, ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        uint32_t confirmed{0};
        if (is_confirmed(height, hash_bytes)) {
            confirmed = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(confirmed));
        return true;
    }
    case method_id_reset: {
        if (!m_whitelist.count(context.caller.to_hex_string())) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            err.cost = 50000;
            xwarn("[xtop_evm_eth_bridge_contract::execute] caller %s not in the list", context.caller.to_hex_string().c_str());
            return false;
        }
        reset();
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(0));
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

bool xtop_evm_eth_bridge_contract::init(const xbytes_t & rlp_bytes) {
    // step 1: check init
    if (get_last_hash() != h256()) {
        xwarn("[xtop_evm_eth_bridge_contract::init] init already");
        return false;
    }
    // step 2: decode
    auto item = RLP::decode_once(rlp_bytes);
    xassert(item.decoded.size() == 1);
    xeth_header_t header;
    if (header.decode_rlp(item.decoded[0]) == false) {
        xwarn("[xtop_evm_eth_bridge_contract::init] decode header error");
        return false;
    }
    // step 3: store with no check
    xinfo("[xtop_evm_eth_bridge_contract::init] header dump: %s", header.dump().c_str());
    if (!set_last_hash(header.hash())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_last_hash failed, hash: %s", header.hash().hex().c_str());
        return false;
    }
    if (!set_effective_hash(header.number, header.hash())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_effective_hash failed, height: %s, hash: %s", header.number.str().c_str(), header.hash().hex().c_str());
        return false;
    }
    if (!set_hashes(header.number, {header.hash()})) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_hash failed, height: %s, hash: %s", header.number.str().c_str(), header.hash().hex().c_str());
        return false;
    }
    if (!set_header(header.hash(), header)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_header failed, height: %s, hash: %s", header.number.str().c_str(), header.hash().hex().c_str());
        return false;
    }
    xeth_header_info_t header_info{header.difficulty, header.parent_hash, header.number};
    if (!set_header_info(header.hash(), header_info)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_header_info failed, height: %s, hash: %s", header.number.str().c_str(), header.hash().hex().c_str());
        return false;
    }

    xinfo("[xtop_evm_eth_bridge_contract::init] init success");
    return true;
}

bool xtop_evm_eth_bridge_contract::sync(const xbytes_t & rlp_bytes) {
    // step 1: check init
    if (get_last_hash() == h256()) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] not init yet");
        return false;
    }

    auto left_bytes = std::move(rlp_bytes);
    while (left_bytes.size() != 0) {
        // step 2: decode
        RLP::DecodedItem item = RLP::decode(left_bytes);
        left_bytes = std::move(item.remainder);
        xeth_header_t header;
        {
            auto item_header = RLP::decode_once(item.decoded[0]);
            auto header_bytes = item_header.decoded[0];
            if (header.decode_rlp(header_bytes) == false) {
                xwarn("[xtop_evm_eth_bridge_contract::sync] decode header error");
                return false;
            }
        }
        xinfo("[xtop_evm_eth_bridge_contract::sync] header dump: %s", header.dump().c_str());
        auto proofs_per_node = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[item.decoded.size() - 1]));
        std::vector<ethash::double_node_with_merkle_proof> nodes;
        uint32_t nodes_size{64};
        if (proofs_per_node * nodes_size + 2 * nodes_size + 3 != item.decoded.size()) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] param error");
            return false;
        }
        uint32_t nodes_start_index{2};
        uint32_t proofs_start_index{2 + nodes_size * 2};
        for (size_t i = 0; i < nodes_size; ++i) {
            ethash::double_node_with_merkle_proof node;
            node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i]));
            node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i + 1]));
            for (size_t j = 0; j < proofs_per_node; ++j) {
                node.proof.emplace_back(static_cast<h128>(item.decoded[proofs_start_index + proofs_per_node * i + j]));
            }
            nodes.emplace_back(node);
        }
        xeth_header_t parent_header;
        if (!get_header(header.parent_hash, parent_header)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] get parent header failed, hash: %s", header.parent_hash.hex().c_str());
            return false;
        }
        // step 5: verify header
        if (!verify(parent_header, header, nodes)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] verify header failed");
            return false;
        }
        // step 6: record
        if (!record(header)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] record header failed");
            return false;
        }
    }
    xinfo("[xtop_evm_eth_bridge_contract::sync] sync success");
    return true;
}

void xtop_evm_eth_bridge_contract::reset() {
    m_contract_state->map_clear(data::system_contract::XPROPERTY_EFFECTIVE_HASHES);
    m_contract_state->map_clear(data::system_contract::XPROPERTY_ALL_HASHES);
    m_contract_state->map_clear(data::system_contract::XPROPERTY_HEADERS);
    m_contract_state->map_clear(data::system_contract::XPROPERTY_HEADERS_SUMMARY);
    set_last_hash(h256());
}

bigint xtop_evm_eth_bridge_contract::get_height() const {
    h256 last_hash = get_last_hash();
    xeth_header_info_t info;
    if (!get_header_info(last_hash, info)) {
        xwarn("[xtop_evm_eth_bridge_contract::get_height] get header info failed, hash: %s", last_hash.hex().c_str());
        return 0;
    }
    xinfo("[xtop_evm_eth_bridge_contract::get_height] height: %s", info.number.str().c_str());
    return info.number;
}

bool xtop_evm_eth_bridge_contract::is_known(const u256 height, const xbytes_t & hash_bytes) const {
    h256 hash = static_cast<h256>(hash_bytes);
    auto hashes = get_hashes(height);
    if (!hashes.count(hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::is_known] not found, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::is_known] is known, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::is_confirmed(const u256 height, const xbytes_t & hash_bytes) const {
    h256 hash = static_cast<h256>(hash_bytes);
    h256 origin_hash = get_effective_hash(height);
    if (hash != origin_hash) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] hash mismatch: %s, %s", hash.hex().c_str(), origin_hash.hex().c_str());
        return false;
    }
    h256 last_hash = get_last_hash();
    xeth_header_info_t info;
    if (!get_header_info(last_hash, info)) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] get header info failed, hash: %s", last_hash.hex().c_str());
        return false;
    }
    if (height + ConfirmHeight > info.number) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] height not confirmed: %s, %s, limit: %d", info.number.str().c_str(), height.str().c_str(), ConfirmHeight);
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::is_confirmed] is_confirmed: %s", hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::verify(const xeth_header_t & prev_header, const xeth_header_t & new_header, const std::vector<ethash::double_node_with_merkle_proof> & nodes) const {
    if (new_header.number != prev_header.number + 1) {
        xwarn("[xtop_evm_eth_bridge_contract::verify] height mismatch, new: %s, old: %s", new_header.number.str().c_str(), prev_header.number.str().c_str());
        return false;
    }
    if (new_header.extra.size() > MaximumExtraDataSize) {
        xwarn("[xtop_evm_eth_bridge_contract::verify] extra size too big: %zu > 32", new_header.extra.size());
        return false;
    }
    if (new_header.time <= prev_header.time) {
        xwarn("[xtop_evm_eth_bridge_contract::verify] time mismatch, new: %lu, old: %lu", new_header.time, prev_header.time);
        return false;
    }
    if (new_header.gas_limit > MaxGasLimit) {
        xwarn("[xtop_evm_eth_bridge_contract::verify] gaslimit too big: %lu > 0x7fffffffffffffff", new_header.gas_limit);
        return false;
    }
    if (new_header.gas_used > new_header.gas_limit) {
        xwarn("[xtop_evm_eth_bridge_contract::verify] gasUsed: %lu > gasLimit: %lu", new_header.gas_used, new_header.gas_limit);
        return false;
    }
    if ((new_header.gas_limit >= prev_header.gas_limit * 1025 / 1024) || (new_header.gas_limit <= prev_header.gas_limit * 1023 / 1024)) {
        xwarn("[xtop_evm_eth_bridge_contract::verify] gaslimit mismatch, new: %lu, old: %lu", new_header.gas_limit, prev_header.gas_limit);
        return false;
    }
    if (!eth::config::is_london(new_header.number)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] not london fork");
        return false;
    }
    if (!eth::verify_eip1559_header(prev_header, new_header)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] verifyEip1559Header failed, new: %lu, old: %lu", new_header.gas_limit, prev_header.gas_limit);
        return false;
    }
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
    // step 6: verify difficulty
    bigint diff = ethash::xethash_t::instance().calc_difficulty(new_header.time, prev_header);
    if (diff != new_header.difficulty) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] difficulty check mismatch");
        return false;
    }
    // step 7: verify ethash
    if (!ethash::xethash_t::instance().verify_seal(new_header, nodes)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] ethash verify failed, header: %s", new_header.hash().hex().c_str());
        return false;
    }
#endif
    return true;
}

bool xtop_evm_eth_bridge_contract::record(const xeth_header_t & header) {
    h256 header_hash = header.hash();
    h256 last_hash = get_last_hash();
    xeth_header_info_t last_info;
    if (!get_header_info(last_hash, last_info)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] get_header_info failed, hash: %s", last_hash.hex().c_str());
        return false;
    }
    if (header.number + BlockReserveNum < last_info.number) {
        xwarn("[xtop_evm_eth_bridge_contract::record] header is too old height: %s, hash: %s, now height: %s",
              header.number.str().c_str(),
              header_hash.hex().c_str(),
              last_info.number.str().c_str());
        return false;
    }
    xeth_header_info_t parent_info;
    if (!get_header_info(header.parent_hash, parent_info)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] get parent header_info failed, hash: %s", header.parent_hash.hex().c_str());
        return false;
    }
    auto all_hashes = get_hashes(header.number);
    if (all_hashes.count(header_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] header existed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    all_hashes.insert(header_hash);
    if (!set_hashes(header.number, all_hashes)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] set hashes failed, height: %s", header.number.str().c_str());
        return false;
    }
    if (!set_header(header_hash, header)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] set header failed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    xeth_header_info_t info{header.difficulty + parent_info.difficult_sum, header.parent_hash, header.number};
    if (!set_header_info(header.hash(), info)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] set_header_info failed, height: %s, hash: %s", header.number.str().c_str(), header.hash().hex().c_str());
        return false;
    }
    if (info.difficult_sum > last_info.difficult_sum || (info.difficult_sum == last_info.difficult_sum && header.difficulty % 2 == 0)) {
        if (!rebuild(header, last_info, info)) {
            xwarn("[xtop_evm_eth_bridge_contract::record] rebuild failed");
            return false;
        }
        release(header.number);
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::rebuild(const xeth_header_t & header, const xeth_header_info_t & last_info, const xeth_header_info_t & cur_info) {
    if (last_info.number > cur_info.number) {
        for (bigint i = cur_info.number + 1; i <= last_info.number; ++i) {
            remove_effective_hash(i);
        }
    }
    auto header_hash = header.hash();
    if (!set_last_hash(header_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] set_last_hash failed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    if (!set_effective_hash(header.number, header_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_effective_hash failed, height: %s, hash: %s", header.number.str().c_str(), header_hash.hex().c_str());
        return false;
    }
    bigint height = header.number - 1;
    auto current_hash = cur_info.parent_hash;
    for (;;) {
        if (height == 0) {
            break;
        }
        if (get_effective_hash(height) == current_hash) {
            break;
        }
        if (!set_effective_hash(height, current_hash)) {
            xwarn("[xtop_evm_eth_bridge_contract::init] set_effective_hash failed, height: %s, hash: %s", height.str().c_str(), current_hash.hex().c_str());
            return false;
        }
        xeth_header_info_t info;
        if (get_header_info(current_hash, info)) {
            current_hash = info.parent_hash;
        } else {
            break;
        }
        height -= 1;
    }
    return true;
}

void xtop_evm_eth_bridge_contract::release(const bigint number) {
    xeth_header_t header;
    bigint difficulty;
    h256 hash;
    if (number > BlockReserveNum) {
        bigint n = number - BlockReserveNum;
        for (;;) {
            auto hashes = get_hashes(n);
            if (hashes.empty()) {
                break;
            }
            for (auto h : hashes) {
                remove_header_info(h);
                remove_header(h);
            }
            remove_hashes(n);
            if (n == 0) {
                break;
            }
            n -= 1;
        }
    }
    if (number > HashReserveNum) {
        bigint n = number - HashReserveNum;
        for (;;) {
            if (get_effective_hash(n) == h256()) {
                break;
            }
            remove_effective_hash(n);
            if (n == 0) {
                break;
            }
            n -= 1;
        }
    }
}

bool xtop_evm_eth_bridge_contract::get_header(const h256 hash, xeth_header_t & header) const {
    auto k = hash.asBytes();
    auto header_str = m_contract_state->map_get(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()});
    if (header_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header] get_header not exist, hash: %s", hash.hex().c_str());
        return false;
    }
    if (header.decode_rlp({std::begin(header_str), std::end(header_str)}) == false) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header] decode_header failed, hash: %s", hash.hex().c_str());
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::set_header(const h256 hash, const xeth_header_t & header) {
    auto k = hash.asBytes();
    auto v = header.encode_rlp();
    if (0 != m_contract_state->map_set(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_header(const h256 hash) {
    auto k = hash.asBytes();
    if (0 != m_contract_state->map_remove(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::get_header_info(const h256 hash, xeth_header_info_t & header_info) const {
    auto k = hash.asBytes();
    auto info_str = m_contract_state->map_get(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()});
    if (info_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header_info] get_header not exist, hash: %s", hash.hex().c_str());
        return false;
    }
    if (header_info.decode_rlp({std::begin(info_str), std::end(info_str)}) == false) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header_info] decode_header failed, hash: %s", hash.hex().c_str());
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::set_header_info(const h256 hash, const xeth_header_info_t & header_info) {
    auto k = hash.asBytes();
    auto v = header_info.encode_rlp();
    if (0 != m_contract_state->map_set(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_header_info(const h256 hash) {
    auto k = hash.asBytes();
    if (0 != m_contract_state->map_remove(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

h256 xtop_evm_eth_bridge_contract::get_effective_hash(const bigint height) const {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto hash_str = m_contract_state->map_get(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()});
    if (hash_str.empty()) {
        return h256();
    }
    return static_cast<h256>(xbytes_t{std::begin(hash_str), std::end(hash_str)});
}

bool xtop_evm_eth_bridge_contract::set_effective_hash(const bigint height, const h256 hash) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto v = hash.asBytes();
    if (0 != m_contract_state->map_set(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_effective_hash(const bigint height) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != m_contract_state->map_remove(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

std::set<h256> xtop_evm_eth_bridge_contract::get_hashes(const bigint height) const {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto str = m_contract_state->map_get(data::system_contract::XPROPERTY_ALL_HASHES, {k.begin(), k.end()});
    if (str.empty()) {
        return {};
    }
    std::set<h256> hashes;
    auto item = RLP::decodeList({std::begin(str), std::end(str)});
    for (auto bytes : item.decoded) {
        hashes.insert(static_cast<h256>(bytes));
    }
    return hashes;
}

bool xtop_evm_eth_bridge_contract::set_hashes(const bigint height, const std::set<h256> & hashes) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    xbytes_t v;
    for (auto h : hashes) {
        auto bytes = RLP::encode(h.asBytes());
        v.insert(v.end(), bytes.begin(), bytes.end());
    }
    if (0 != m_contract_state->map_set(data::system_contract::XPROPERTY_ALL_HASHES, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_hashes(const bigint height) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != m_contract_state->map_remove(data::system_contract::XPROPERTY_ALL_HASHES, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

h256 xtop_evm_eth_bridge_contract::get_last_hash() const {
    auto hash_str = m_contract_state->string_get(data::system_contract::XPROPERTY_LAST_HASH);
    if (hash_str.empty()) {
        return h256();
    }
    return static_cast<h256>(xbytes_t{std::begin(hash_str), std::end(hash_str)});
}

bool xtop_evm_eth_bridge_contract::set_last_hash(const h256 hash) {
    auto bytes = hash.asBytes();
    if (0 != m_contract_state->string_set(data::system_contract::XPROPERTY_LAST_HASH, {bytes.begin(), bytes.end()})) {
        return false;
    }
    return true;
}

NS_END4
