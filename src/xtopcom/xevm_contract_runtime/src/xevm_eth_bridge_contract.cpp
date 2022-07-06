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

#include <fstream>

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

xtop_evm_eth_bridge_contract::xtop_evm_eth_bridge_contract() {
    m_whitelist = load_whitelist();
    if (m_whitelist.empty()) {
        xassert(false);
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
    // is_confirmed(bytes)                   => 627b0fd2
    // reset()                               => d826f88f
    //--------------------------------------------------
    constexpr uint32_t method_id_init{0x6158600d};
    constexpr uint32_t method_id_sync{0x7eefcfa2};
    constexpr uint32_t method_id_get_height{0xb15ad2e8};
    constexpr uint32_t method_id_is_confirmed{0x627b0fd2};
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
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(xbytes_t{std::begin(input), std::end(input)}, ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth_bridge_contract::execute] illegal input data");
        return false;
    }
    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    if (ec) {
        err.fail_status = precompile_error::Fatal;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
        xwarn("[xtop_evm_eth_bridge_contract::execute] illegal input function selector");
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::execute] caller: %s, method_id: 0x%x, input size: %zu", context.caller.to_hex_string().c_str(), function_selector.method_id, input.size());

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
        evm_common::xh256s_t topics;
        topics.push_back(evm_common::xh256_t(context.caller.to_h256()));
        evm_common::xevm_log_t log(context.address, topics, top::to_bytes(u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        xinfo("[xtop_evm_eth_bridge_contract::execute] init success");
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
        evm_common::xh256s_t topics;
        topics.push_back(evm_common::xh256_t(context.caller.to_h256()));
        evm_common::xevm_log_t log(context.address, topics, top::to_bytes(u256(0)));
        output.cost = 0;
        output.exit_status = Returned;
        output.logs.push_back(log);
        xinfo("[xtop_evm_eth_bridge_contract::execute] sync success");
        return true;
    }
    case method_id_get_height: {
        bigint height{0};
        if (!get_height(height)) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::CallErrorAsFatal);
            xwarn("[xtop_evm_eth_bridge_contract::execute] get_height failed, height");
            return false;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(height));
        xinfo("[xtop_evm_eth_bridge_contract::execute] get_height %lu", static_cast<uint64_t>(height));
        return true;
    }
    case method_id_is_confirmed: {
        uint32_t confirmed{0};
        auto hash_bytes = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        if (is_confirmed(hash_bytes)) {
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

std::set<std::string> xtop_evm_eth_bridge_contract::load_whitelist() {
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

bool xtop_evm_eth_bridge_contract::init(const xbytes_t & rlp_bytes) {
    // step 0: check init
    bigint height{0};
    if (!get_height(height)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] get_height error");
        return false;
    }
    if (height != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::init] init already");
        return false;
    }
    // step 2: decode
    auto item = RLP::decode_once(rlp_bytes);
    xassert(item.decoded.size() == 1);
    eth::xeth_block_header_t header;
    if (header.from_rlp(item.decoded[0]) < 0) {
        xwarn("[xtop_evm_eth_bridge_contract::init] decode header error");
        return false;
    }
    header.hash();
    // step 3: check exist
    h256 hash{0};
    if (get_hash(header.number(), hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] get_hash existed, height: %s, hash: %s", header.number().str().c_str(), hash.hex().c_str());
        return false;
    }
    // step 4: store block with no check
    xinfo("[xtop_evm_eth_bridge_contract::init] header dump: %s", header.dump().c_str());
    if (!set_header(header, header.difficulty())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_header failed, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::init] set_header success, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
    // step 5: store mainchain header
    if (!set_hash(header.number(), header.hash())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_hash failed, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::init] set_hash success, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
    // step 6: store height
    if (!set_height(header.number())) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_height failed, height: %s", header.number().str().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::init] set_height success, height: %s", header.number().str().c_str());
    
    return true;
}

bool xtop_evm_eth_bridge_contract::sync(const xbytes_t & rlp_bytes) {
    // step 1: check init
    bigint last_height{0};
    if (!get_height(last_height)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] get_height failed, height: %s", last_height.str().c_str());
        return false;
    }
    if (last_height == 0) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] not init yet");
        return false;
    }

    auto left_bytes = std::move(rlp_bytes);
    while (left_bytes.size() != 0) {
        // step 2: decode
        RLP::DecodedItem item = RLP::decode(left_bytes);
        left_bytes = std::move(item.remainder);
        eth::xeth_block_header_t header;
        {
            auto item_header = RLP::decode_once(item.decoded[0]);
            auto header_bytes = item_header.decoded[0];
            if (header.from_rlp(header_bytes) < 0) {
                xwarn("[xtop_evm_eth_bridge_contract::sync] decode header error");
                return false;
            }
        }
        auto proofs_per_node = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[item.decoded.size() - 1]));
        std::vector<ethash::double_node_with_merkle_proof> nodes;
        uint32_t nodes_size{64};
        if (proofs_per_node * nodes_size + 2 * nodes_size + 3 != item.decoded.size()) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] param error");
            return false;
        }
        uint32_t nodes_start_index{2};
        uint32_t proofs_start_index{2 + nodes_size * 2};
        for (size_t i = 0; i < nodes_size; i++) {
            ethash::double_node_with_merkle_proof node;
            node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i]));
            node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i + 1]));
            for (size_t j = 0; j < proofs_per_node; j++) {
                node.proof.emplace_back(static_cast<h128>(item.decoded[proofs_start_index + proofs_per_node * i + j]));
            }
            nodes.emplace_back(node);
        }
        // step 3: check exist
        h256 hash{0};
        if (get_hash(header.number(), hash)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] get_hash existed, height: %s, hash: %s", header.number().str().c_str(), hash.hex().c_str());
            return false;
        }
        xinfo("[xtop_evm_eth_bridge_contract::sync] header dump: %s", header.dump().c_str());
        if (!get_height(last_height)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] get last height failed, height: %s", last_height.str().c_str());
            return false;
        }
        if (header.number() + BlockReserveNum < last_height) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] header is too old height: %s, hash: %s, now height: %s",
                header.number().str().c_str(),
                header.hash().hex().c_str(),
                last_height.str().c_str());
            return false;
        }
        // step 4: get parent header
        eth::xeth_block_header_t parentHeader;
        bigint preSumOfDifficult{0};
        if (!get_header(header.parentHash(), parentHeader, preSumOfDifficult)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] get parent header failed, hash: %s", header.parentHash().hex().c_str());
            return false;
        }
        // step 5: verify header common
        if (!verify_common(parentHeader, header)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] verify header common failed, header: %s, parrent header: %s", header.hash().hex().c_str(), parentHeader.hash().hex().c_str());
            return false;
        }
        if (!eth::config::is_london(header.number())) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] not london fork");
            return false;
        }
        if (!eth::verify_eip1559_header(parentHeader, header)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] verifyEip1559Header failed, new: %lu, old: %lu", header.gasLimit(), parentHeader.gasLimit());
            return false;
        }
        // step 6: verify difficulty
        bigint diff = ethash::xethash_t::instance().calc_difficulty(header.time(), parentHeader);
        if (diff != header.difficulty()) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] difficulty check mismatch");
            return false;
        }
        // step 7: verify ethash
        if (!ethash::xethash_t::instance().verify_seal(header, nodes)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] ethash verify failed, header: %s", header.hash().hex().c_str());
            return false;
        }
        // step 8: set header
        bigint newSumOfDifficult = preSumOfDifficult + header.difficulty();
        if (!set_header(header, newSumOfDifficult)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] set_header failed, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
            return false;
        }
        xinfo("[xtop_evm_eth_bridge_contract::sync] set_header success, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
        // step 9: get last header
        h256 last_hash;
        if (!get_hash(last_height, last_hash)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] get_hash failed, height: %s", last_height.str().c_str());
            return false;
        }
        eth::xeth_block_header_t last_header;
        bigint last_difficulty{0};
        if (!get_header(last_hash, last_header, last_difficulty)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] get last header failed, hash: %s", last_hash.hex().c_str());
            return false;
        }
        // step 10: set or rebuild
        if (last_header.hash() == header.parentHash()) {
            if (!set_hash(header.number(), header.hash())) {
                xwarn("[xtop_evm_eth_bridge_contract::sync] set_hash failed, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
                return false;
            }
            xinfo("[xtop_evm_eth_bridge_contract::sync] set_hash success, height: %s, hash: %s", header.number().str().c_str(), header.hash().hex().c_str());
            if (!set_height(header.number())) {
                xwarn("[xtop_evm_eth_bridge_contract::sync] set_height failed, height: %s", header.number().str().c_str());
                return false;
            }
            xwarn("[xtop_evm_eth_bridge_contract::sync] set_height success, height: %s", header.number().str().c_str());
        } else {
            xinfo("[xtop_evm_eth_bridge_contract::sync] hash mismatch rebuid, %s, %s", last_header.hash().hex().c_str(), header.parentHash().hex().c_str());
            if (newSumOfDifficult > last_difficulty) {
                if (!rebuild(last_header, header)) {
                    xwarn("[xtop_evm_eth_bridge_contract::sync] rebuild failed");
                    return false;
                }
            }
        }
        // step 11: release
        release(header.number());
    }

    return true;
}

void xtop_evm_eth_bridge_contract::reset() {
    m_contract_state->map_clear(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER);
    m_contract_state->map_clear(data::system_contract::XPROPERTY_ETH_CHAINS_HASH);
    set_height(0);
}

bool xtop_evm_eth_bridge_contract::is_confirmed(const xbytes_t & hash_bytes) {
    h256 hash = static_cast<h256>(hash_bytes);
    eth::xeth_block_header_t header;
    bigint sumOfDifficult{0};
    if (!get_header(hash, header, sumOfDifficult)) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] get_header failed, hash: %s", hash.hex().c_str());
        return false;
    }
    bigint cur_height{0};
    if (!get_height(cur_height)) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] get current height failed, height");
        return false;
    }
    if (cur_height < header.number() + ConfirmHeight) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] height not confirmed: %s, %s, limit: %d", cur_height.str().c_str(), header.number().str().c_str(), ConfirmHeight);
        return false;
    }

    return true;
}

bool xtop_evm_eth_bridge_contract::verify_common(const eth::xeth_block_header_t & prev_header, const eth::xeth_block_header_t & new_header) const {
    if (new_header.number() != prev_header.number() + 1) {
        xwarn("[xtop_evm_eth_bridge_contract::verify_common] height mismatch, new: %s, old: %s", new_header.number().str().c_str(), prev_header.number().str().c_str());
        return false;
    }
    if (new_header.extra().size() > MaximumExtraDataSize) {
        xwarn("[xtop_evm_eth_bridge_contract::verify_common] extra size too big: %zu > 32", new_header.extra().size());
        return false;
    }
    if (new_header.time() <= prev_header.time()) {
        xwarn("[xtop_evm_eth_bridge_contract::verify_common] time mismatch, new: %lu, old: %lu", new_header.time(), prev_header.time());
        return false;
    }
    if (new_header.gasLimit() > MaxGasLimit) {
        xwarn("[xtop_evm_eth_bridge_contract::verify_common] gaslimit too big: %lu > 0x7fffffffffffffff", new_header.gasLimit());
        return false;
    }
    if (new_header.gasUsed() > new_header.gasLimit()) {
        xwarn("[xtop_evm_eth_bridge_contract::verify_common] gasUsed: %lu > gasLimit: %lu", new_header.gasUsed(), new_header.gasLimit());
        return false;
    }
    if ((new_header.gasLimit() >= prev_header.gasLimit() * 1025 / 1024) || (new_header.gasLimit() <= prev_header.gasLimit() * 1023 / 1024)) {
        xwarn("[xtop_evm_eth_bridge_contract::verify_common] gaslimit mismatch, new: %lu, old: %lu", new_header.gasLimit(), prev_header.gasLimit());
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::get_header(const h256 hash, eth::xeth_block_header_t & header, bigint & difficulty) const {
    auto k = hash.asBytes();
    auto header_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER, {k.begin(), k.end()});
    if (header_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header] get_header not exist, hash: %s", hash.hex().c_str());
        return false;
    }
    eth::xeth_block_header_with_difficulty_t header_with_difficulty;
    if (header_with_difficulty.from_string(header_str) < 0) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header] decode_header failed, hash: %s", hash.hex().c_str());
        return false;
    }
    header = header_with_difficulty.m_header;
    difficulty = header_with_difficulty.m_difficult_sum;
    return true;
}

bool xtop_evm_eth_bridge_contract::set_header(eth::xeth_block_header_t & header, bigint difficulty) {
    eth::xeth_block_header_with_difficulty_t header_with_difficulty{header, difficulty};
    auto k = header.hash().asBytes();
    auto v = header_with_difficulty.to_string();
    if (0 != m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_header(const evm_common::h256 hash) {
    auto k = hash.asBytes();
    if (0 != m_contract_state->map_remove(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::get_hash(const bigint height, h256 & hash) const {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto hash_str = m_contract_state->map_get(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, {k.begin(), k.end()});
    if (hash_str.empty()) {
        return false;
    }
    xbytes_t v{std::begin(hash_str), std::end(hash_str)};
    hash = static_cast<h256>(v);
    return true;
}

bool xtop_evm_eth_bridge_contract::set_hash(const bigint height, const h256 hash) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto v = hash.asBytes();
    if (0 != m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_hash(const bigint height) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != m_contract_state->map_remove(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::get_height(bigint & height) const {
    auto height_str = m_contract_state->string_get(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT);
    if (height_str.empty()) {
        return false;
    }
    height = static_cast<bigint>(evm_common::fromBigEndian<u256>(height_str));
    return true;
}

bool xtop_evm_eth_bridge_contract::set_height(const bigint height) {
    auto bytes = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != m_contract_state->string_set(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT, {bytes.begin(), bytes.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::rebuild(eth::xeth_block_header_t & current_header, eth::xeth_block_header_t & new_header) {
    const bigint current_height = current_header.number();
    bigint new_height = new_header.number();
    h256 new_hash = new_header.hash();

    std::map<xbytes_t, xbytes_t> add;
    bigint remove_cnt{0};

    if (new_height < current_height) {
        for (bigint h = new_height + 1; h <= current_height; ++h) {
            if (!remove_hash(h)) {
                xwarn("[xtop_evm_eth_bridge_contract::rebuild] remove hash error, height: %s", h.str().c_str());
            }
        }
    } else if (new_height > current_height) {
        while (new_height > current_height) {
            eth::xeth_block_header_with_difficulty_t h;
            bigint difficult_sum;
            if (!get_header(new_hash, h.m_header, difficult_sum)) {
                xwarn("[xtop_evm_eth_bridge_contract::rebuild] get_header failed, hash: %s", new_hash.hex().c_str());
                return false;
            }
            if (h.m_header.number() != new_height) {
                xwarn("[xtop_evm_eth_bridge_contract::rebuild] header height mismatch, stored height: %s, new_height: %s",
                      h.m_header.number().str().c_str(),
                      new_height.str().c_str());
                return false;
            }
            add[evm_common::toBigEndian(static_cast<u256>(new_height))] = new_hash.asBytes();
            new_hash = h.m_header.parentHash();
            new_height--;
        }
        xassert(new_height == current_height);
    }

    h256 new_fork_hash{new_hash};
    h256 same_height_hash{0};
    if (!get_hash(new_height, same_height_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::rebuild] get hash failed, height: %s", new_height.str().c_str());
        return false;
    }

    while (new_fork_hash != same_height_hash) {
        add[evm_common::toBigEndian(static_cast<u256>(new_height))] = new_fork_hash.asBytes();
        new_height--;
        // get previous new_fork_hash
        eth::xeth_block_header_with_difficulty_t h;
        if (!get_header(new_fork_hash, h.m_header, h.m_difficult_sum)) {
            xwarn("[xtop_evm_eth_bridge_contract::rebuild] get last header failed, hash: %s", new_hash.hex().c_str());
            return false;
        }
        new_fork_hash = h.m_header.parentHash();
        // get previous same_height_hash
        if (!get_hash(new_height, same_height_hash)) {
            xwarn("[xtop_evm_eth_bridge_contract::rebuild] get_hash failed, hash: %s", new_height.str().c_str());
            return false;
        }
    }

    for (auto const & a : add) {
        if (0 != m_contract_state->map_set(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, {a.first.begin(), a.first.end()}, {a.second.begin(), a.second.end()})) {
            xwarn("[xtop_evm_eth_bridge_contract::rebuild] set hash error");
        }
    }

    if (!set_height(new_header.number())) {
        xwarn("[xtop_evm_eth_bridge_contract::rebuild] set_height failed, height: %s", new_header.number().str().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::rebuild] rebuild success");

    return true;
}

void xtop_evm_eth_bridge_contract::release(const bigint number) {
    eth::xeth_block_header_t header;
    bigint difficulty;
    h256 hash;
    if (number > BlockReserveNum) {
        bigint n = number - BlockReserveNum;
        for (;;) {
            if (!get_hash(n, hash)) {
                break;
            }
            if (!get_header(hash, header, difficulty)) {
                break;
            }
            remove_header(hash);
            if (n == 0) {
                break;
            }
            n -= 1;
        }
    }
    if (number > HashReserveNum) {
        bigint n = number - HashReserveNum;
        for (;;) {
            if (!get_hash(n, hash)) {
                break;

            }
            remove_hash(n);
            if (n == 0) {
                break;
            }
            n -= 1;
        }
    }
}

NS_END4
