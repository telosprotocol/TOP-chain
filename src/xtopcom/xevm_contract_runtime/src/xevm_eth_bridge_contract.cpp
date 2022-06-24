// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_eth_bridge_contract.h"

#include "xbasic/endianness.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xdata/xdata_common.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xeth/xeth_config.h"
#include "xevm_common/xeth/xeth_eip1559.h"
#include "xevm_common/xeth/xeth_gaslimit.h"
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_common/xeth/xethash.h"

#include <cinttypes>

NS_BEG4(top, contract_runtime, evm, sys_contract)

using namespace top::evm_common;

constexpr uint64_t MinGasLimit = 5000;
constexpr uint64_t MaxGasLimit = 0x7fffffffffffffff;
constexpr uint64_t MaximumExtraDataSize = 32;
constexpr uint64_t ConfirmHeight = 25;

static uint64_t cpu_time() {
    struct timespec time;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
    return uint64_t(time.tv_sec) * 1000000 + uint64_t(time.tv_nsec) / 1000;
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
    // init_genesis_block_header(bytes, string)     => 78dcb6c7
    // syncBlockHeader(bytes)                       => 1e090626
    // getCurrentHeightOfMainChain(uint64)          => daf0c99a
    // getHeaderIfHeightConfirmed(bytes,uint64)     => 524dfb52
    //--------------------------------------------------
    constexpr uint32_t method_id_init{0x78dcb6c7};
    constexpr uint32_t method_id_sync{0x1e090626};
    constexpr uint32_t method_id_get_current_height{0xdaf0c99a};
    constexpr uint32_t method_id_get_if_confirmed_height{0x524dfb52};
    constexpr uint32_t method_id_get_hash{0xd150aa9c};
    
    auto time1 = cpu_time();
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
    xinfo("[xtop_evm_eth_bridge_contract::execute] caller: %s, method_id: %u, input size: %zu", context.caller.to_hex_string().c_str(), function_selector.method_id, input.size());

    switch (function_selector.method_id) {
    case method_id_init: {
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
        auto verify_str = abi_decoder.extract<std::string>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract string error");
            return false;
        }
        if (!verifyOwner(verify_str)) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] verify string error");
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
        auto time2 = cpu_time();
        xinfo("[xtop_evm_eth_bridge_contract::execute] init success, time: %lu", time2 - time1);
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
        auto time2 = cpu_time();
        xinfo("[xtop_evm_eth_bridge_contract::execute] sync success, time: %lu", time2 - time1);
        return true;
    }
    case method_id_get_current_height: {
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
    case method_id_get_hash: {
        auto height = abi_decoder.extract<uint64_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract error");
            return false;
        }
        h256 hash{0};
        if (!get_hash(height, hash)) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::CallErrorAsFatal);
            xwarn("[xtop_evm_eth_bridge_contract::execute] get_hash failed, height: %lu", height);
            return false;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = hash.asBytes();
        xinfo("[xtop_evm_eth_bridge_contract::execute] get_hash %s", hash.hex().c_str());
        return true;
    }
    case method_id_get_if_confirmed_height: {
        uint32_t confirmed{0};
        auto headers_rlp = abi_decoder.extract<xbytes_t>(ec);
        if (ec) {
            err.fail_status = precompile_error::Fatal;
            err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::Other);
            xwarn("[xtop_evm_eth_bridge_contract::execute] abi_decoder.extract bytes error");
            return false;
        }
        if (is_confirmed(headers_rlp)) {
            confirmed = 1;
        }
        output.exit_status = Returned;
        output.cost = 0;
        output.output = evm_common::toBigEndian(static_cast<u256>(confirmed));
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
    if (!header.from_rlp(item.decoded[0])) {
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
    // step 2: decode
    auto item = RLP::decode(rlp_bytes);
        eth::xeth_block_header_t header;
    {
        auto item_header = RLP::decode_once(item.decoded[0]);
        auto header_bytes = item_header.decoded[0];
        header.from_rlp(header_bytes);
    }
    const uint64_t proofs_per_node = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[item.decoded.size() - 1]));
    const uint32_t nodes_size{64};
    if (proofs_per_node * nodes_size + 2 * nodes_size + 3 != item.decoded.size()) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] param error");
            return false;
        }
    std::vector<ethash::double_node_with_merkle_proof> nodes;
    const uint32_t nodes_start_index{2};
    const uint32_t proofs_start_index{2 + nodes_size * 2};
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
        // step 4: get parent header
        eth::xeth_block_header_t parentHeader;
        bigint preSumOfDifficult{0};
        if (!get_header(header.parentHash(), parentHeader, preSumOfDifficult)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] get parent header failed, hash: %s", header.parentHash().hex().c_str());
            return false;
        }
        // step 5: verify header common
        if (!verifyCommon(parentHeader, header)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] verify header common failed, header: %s, parrent header: %s", header.hash().hex().c_str(), parentHeader.hash().hex().c_str());
            return false;
        }
        if (eth::config::is_london(header.number())) {
            if (!eth::verify_eip1559_header(parentHeader, header)) {
                xwarn("[xtop_evm_eth_bridge_contract::sync] verifyEip1559Header failed, new: %lu, old: %lu", header.gasLimit(), parentHeader.gasLimit());
                return false;
            }
        } else {
            if (!eth::verify_gaslimit(parentHeader.gasLimit(), header.gasLimit())) {
                xwarn("[xtop_evm_eth_bridge_contract::sync] gaslimit mismatch, new: %lu, old: %lu", header.gasLimit(), parentHeader.gasLimit());
                return false;
            }
        }
        // step 6: verify difficulty
        bigint diff = ethash::xethash_t::instance().calc_difficulty(header.time(), parentHeader);
        // if (eth::config::is_arrow_glacier(header.number())) {
        //     diff = eth::difficulty::calculate(header.time(), &parentHeader, ArrowGlacierBombDelay);
        // } else if (eth::config::is_london(header.number())) {
        //     diff = eth::difficulty::calculate(header.time(), &parentHeader, LondonBombDelay);
        // } else {
        //     xwarn("[xtop_evm_eth_bridge_contract::sync] unexpected fork");
        //     xassert(false);
        // }
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

    return true;
}

bool xtop_evm_eth_bridge_contract::is_confirmed(const xbytes_t & rlp_bytes) {
    // cmp height
    bigint height{0};
    if (!get_height(height)) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] get height failed, height: %s", height.str().c_str());
        return false;
    }
    auto item = RLP::decode_once(rlp_bytes);
    xassert(item.decoded.size() == 1);
    eth::xeth_block_header_t header;
    if (!header.from_rlp(item.decoded[0])) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] decode header error");
        return false;
    }
    if (height < header.number() + ConfirmHeight) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] height not enough: %s, %s, limit:%lu", height.str().c_str(), header.number().str().c_str(), ConfirmHeight);
        return false;
    }

    h256 hash;
    if (!get_hash(header.number(), hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] get_hash failed, height: %s", header.number().str().c_str());
        return false;
    }

    eth::xeth_block_header_t cur_header;
    bigint sumOfDifficult{0};
    if (!get_header(hash, cur_header, sumOfDifficult)) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] get_header failed, hash: %s", hash.hex().c_str());
        return false;
    }

    if (header.stateMerkleRoot() != cur_header.stateMerkleRoot()) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] state MR mismatch, %s, %s", header.stateMerkleRoot().hex().c_str(), cur_header.stateMerkleRoot().hex().c_str());
        return false;
    }

    if (header.txMerkleRoot() != cur_header.txMerkleRoot()) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] tx MR mismatch, %s, %s", header.txMerkleRoot().hex().c_str(), cur_header.txMerkleRoot().hex().c_str());
        return false;
    }

    if (header.receiptMerkleRoot() != cur_header.receiptMerkleRoot()) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] receipt MR mismatch, %s, %s", header.receiptMerkleRoot().hex().c_str(), cur_header.receiptMerkleRoot().hex().c_str());
        return false;
    }

    return true;
}

bool xtop_evm_eth_bridge_contract::verifyOwner(const std::string & verify_str) const {
    utl::xkeccak256_t hasher;
    xbytes_t output;
    hasher.update(verify_str);
    hasher.get_hash(output);
    if (to_hex(output) != "030b540da3bab783d3feea2e60320ec6be299e41932a9f40c3965d7cb3b4ab94") {
        xwarn("[xtop_evm_eth_bridge_contract::verifyOwner] %s is not a valid verify str", verify_str.c_str());
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::verifyCommon(const eth::xeth_block_header_t & prev_header, const eth::xeth_block_header_t & new_header) const {
    if (new_header.number() != prev_header.number() + 1) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] height mismatch, new: %s, old: %s", new_header.number().str().c_str(), prev_header.number().str().c_str());
        return false;
    }
    if (new_header.extra().size() > MaximumExtraDataSize) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] extra size too big: %zu > 32", new_header.extra().size());
        return false;
    }
    if (new_header.time() <= prev_header.time()) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] time mismatch, new: %lu, old: %lu", new_header.time(), prev_header.time());
        return false;
    }
    if (new_header.gasLimit() > MaxGasLimit) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] gaslimit too big: %lu > 0x7fffffffffffffff", new_header.gasLimit());
        return false;
    }
    if (new_header.gasUsed() > new_header.gasLimit()) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] gasUsed: %lu > gasLimit: %lu", new_header.gasUsed(), new_header.gasLimit());
        return false;
    }
    if ((new_header.gasLimit() >= prev_header.gasLimit() * 1025 / 1024) || (new_header.gasLimit() <= prev_header.gasLimit() * 1023 / 1024)) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyCommon] gaslimit mismatch, new: %lu, old: %lu", new_header.gasLimit(), prev_header.gasLimit());
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
        for (bigint h = new_height + 1; new_height <= current_height; ++h) {
            auto k = evm_common::toBigEndian(static_cast<u256>(h));
            if (!m_contract_state->map_remove(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, {k.begin(), k.end()})) {
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
        add[evm_common::toBigEndian(static_cast<u256>(new_height))] = new_hash.asBytes();
        new_height--;
        // get previous new_fork_hash
        eth::xeth_block_header_with_difficulty_t h;
        if (!get_header(new_hash, h.m_header, h.m_difficult_sum)) {
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

NS_END4
