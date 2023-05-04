// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_eth_bridge_contract.h"

#include "xbasic/endianness.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xdata/xdata_common.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xcommon/common_data.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xcrosschain/xeth_config.h"
#include "xevm_common/xcrosschain/xeth_eip1559.h"
#include "xevm_common/xcrosschain/xeth_gaslimit.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_common/xcrosschain/xethash.h"

using namespace top::evm_common;

NS_BEG4(top, contract_runtime, evm, sys_contract)

constexpr uint64_t MinGasLimit = 5000;
constexpr uint64_t MaxGasLimit = 0x7fffffffffffffff;
constexpr uint64_t MaximumExtraDataSize = 32;
constexpr uint64_t ConfirmHeight = 25;
constexpr uint64_t HashReserveNum = 40000;
constexpr uint64_t BlockReserveNum = 500;

bool xtop_evm_eth_bridge_contract::init(xbytes_t const & rlp_bytes, state_ptr state) {
    // step 1: check init
    if (get_last_hash(state) != h256()) {
        xwarn("[xtop_evm_eth_bridge_contract::init] init already");
        return false;
    }
    // step 2: decode
    auto item = RLP::decode_once(rlp_bytes);
    xassert(item.decoded.size() == 1);
    xeth_header_t header;
    std::error_code ec;
    header.decode_rlp(item.decoded[0], ec);
    if (ec) {
        xwarn("xtop_evm_eth_bridge_contract::init, decode header error, msg %s", ec.message().c_str());
        return false;
    }
    // step 3: store with no check
    xinfo("[xtop_evm_eth_bridge_contract::init] header dump: %s", header.dump().c_str());
    if (!set_last_hash(header.calc_hash(), state)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_last_hash failed, hash: %s", header.calc_hash().hex().c_str());
        return false;
    }
    if (!set_effective_hash(header.number, header.calc_hash(), state)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_effective_hash failed, height: %" PRIu64 ", hash: %s", header.number, header.calc_hash().hex().c_str());
        return false;
    }
    if (!set_hashes(header.number, {header.calc_hash()}, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_hash failed, height: %" PRIu64 ", hash: %s", header.number, header.calc_hash().hex().c_str());
        return false;
    }
    if (!set_header(header.calc_hash(), header, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_header failed, height: %" PRIu64 ", hash: %s", header.number, header.calc_hash().hex().c_str());
        return false;
    }
    xeth_header_info_t const header_info{header.difficulty, header.parent_hash, header.number};
    if (!set_header_info(header.calc_hash(), header_info, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_header_info failed, height: %" PRIu64 ", hash: %s", header.number, header.calc_hash().hex().c_str());
        return false;
    }

    xinfo("[xtop_evm_eth_bridge_contract::init] init success");
    return true;
}

bool xtop_evm_eth_bridge_contract::sync(xbytes_t const & rlp_bytes, state_ptr state) {
    // step 1: check init
    if (get_last_hash(state) == h256()) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] not init yet");
        return false;
    }

    auto left_bytes = rlp_bytes;
    std::error_code ec;
    while (!left_bytes.empty()) {
        // step 2: decode
        RLP::DecodedItem item = RLP::decode(left_bytes);
        left_bytes = std::move(item.remainder);
        xeth_header_t header;
        {
            auto item_header = RLP::decode_once(item.decoded[0]);
            auto header_bytes = item_header.decoded[0];
            header.decode_rlp(header_bytes, ec);
            if (ec) {
                xwarn("xtop_evm_eth_bridge_contract::sync, xeth_header_t::decode_rlp failed, msg %s", ec.message().c_str());
                return false;
            }
        }
        xinfo("[xtop_evm_eth_bridge_contract::sync] header dump: %s", header.dump().c_str());
        auto proofs_per_node = evm_common::fromBigEndian<uint64_t>(item.decoded[item.decoded.size() - 1]);
        std::vector<double_node_with_merkle_proof> nodes;
        uint32_t nodes_size{64};
        if (proofs_per_node * nodes_size + 2 * nodes_size + 3 != item.decoded.size()) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] param error");
            return false;
        }
        uint32_t nodes_start_index{2};
        uint32_t proofs_start_index{2 + nodes_size * 2};
        for (size_t i = 0; i < nodes_size; ++i) {
            double_node_with_merkle_proof node;
            node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i]));
            node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i + 1]));
            for (size_t j = 0; j < proofs_per_node; ++j) {
                node.proof.emplace_back(static_cast<h128>(item.decoded[proofs_start_index + proofs_per_node * i + j]));
            }
            nodes.emplace_back(node);
        }
        xeth_header_t parent_header;
        if (!get_header(header.parent_hash, parent_header, state)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] get parent header failed, hash: %s", header.parent_hash.hex().c_str());
            return false;
        }
        // step 5: verify header
        if (!verify(parent_header, header, nodes)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] verify header failed");
            return false;
        }
        // step 6: record
        if (!record(header, state)) {
            xwarn("[xtop_evm_eth_bridge_contract::sync] record header failed");
            return false;
        }
    }
    xinfo("[xtop_evm_eth_bridge_contract::sync] sync success");
    return true;
}

bool xtop_evm_eth_bridge_contract::reset(state_ptr state) {
    if (get_flag(state) != 0) {
        xwarn("[xtop_evm_eth_bridge_contract::reset] reset already disabled");
        return false;
    }
    state->map_clear(data::system_contract::XPROPERTY_EFFECTIVE_HASHES);
    state->map_clear(data::system_contract::XPROPERTY_ALL_HASHES);
    state->map_clear(data::system_contract::XPROPERTY_HEADERS);
    state->map_clear(data::system_contract::XPROPERTY_HEADERS_SUMMARY);
    set_last_hash(h256(), state);
    xinfo("[xtop_evm_eth_bridge_contract::reset] reset success");
    return true;
}

bool xtop_evm_eth_bridge_contract::disable_reset(state_ptr state) {
    xinfo("[xtop_evm_eth_bridge_contract::disable_reset] disable_reset");
    return set_flag(state);
}

bigint xtop_evm_eth_bridge_contract::get_height(state_ptr state) const {
    h256 last_hash = get_last_hash(state);
    xeth_header_info_t info;
    if (!get_header_info(last_hash, info, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::get_height] get header info failed, hash: %s", last_hash.hex().c_str());
        return 0;
    }
    xinfo("[xtop_evm_eth_bridge_contract::get_height] height: %s", info.number.str().c_str());
    return info.number;
}

xheader_status_t xtop_evm_eth_bridge_contract::query_height(u256 const height, state_ptr state) const {
    auto now_height = get_height(state);
    if (now_height == 0) {
        return xheader_status_t::header_not_confirmed;
    }
    if (now_height >= height + HashReserveNum) {
        return xheader_status_t::header_overdue;
    }
    if (now_height <= height + ConfirmHeight) {
        return xheader_status_t::header_not_confirmed;
    }
    return xheader_status_t::header_confirmed;
}

bool xtop_evm_eth_bridge_contract::is_known(u256 const height, xbytes_t const & hash_bytes, state_ptr state) const {
    h256 hash = static_cast<h256>(hash_bytes);
    auto hashes = get_hashes(height, state);
    if (!hashes.count(hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::is_known] not found, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_eth_bridge_contract::is_known] is known, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
    return true;
}

bool xtop_evm_eth_bridge_contract::is_confirmed(u256 const height, xbytes_t const & hash_bytes, state_ptr state) const {
    h256 hash = static_cast<h256>(hash_bytes);
    h256 origin_hash = get_effective_hash(height, state);
    if (hash != origin_hash) {
        xwarn("[xtop_evm_eth_bridge_contract::is_confirmed] hash mismatch: %s, %s", hash.hex().c_str(), origin_hash.hex().c_str());
        return false;
    }
    h256 last_hash = get_last_hash(state);
    xeth_header_info_t info;
    if (!get_header_info(last_hash, info, state)) {
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

bool xtop_evm_eth_bridge_contract::verify(xeth_header_t const & prev_header, xeth_header_t const & new_header, std::vector<double_node_with_merkle_proof> const & nodes) const {
    if (new_header.number != prev_header.number + 1) {
        xwarn("[xtop_evm_eth_bridge_contract::verify] height mismatch, new: %" PRIu64 ", old: %" PRIu64, new_header.number, prev_header.number);
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
        xwarn("[xtop_evm_eth_bridge_contract::verify] gaslimit too big: %s > 0x7fffffffffffffff", new_header.gas_limit.str().c_str());
        return false;
    }
    if (new_header.gas_used > new_header.gas_limit) {
        xwarn("[xtop_evm_eth_bridge_contract::verify] gasUsed: %s > gasLimit: %s", new_header.gas_used.str().c_str(), new_header.gas_limit.str().c_str());
        return false;
    }
    if ((new_header.gas_limit >= prev_header.gas_limit * 1025 / 1024) || (new_header.gas_limit <= prev_header.gas_limit * 1023 / 1024)) {
        xwarn("[xtop_evm_eth_bridge_contract::verify] gaslimit mismatch, new: %s, old: %s", new_header.gas_limit.str().c_str(), prev_header.gas_limit.str().c_str());
        return false;
    }
    if (!eth::config::is_london(new_header.number)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] not london fork");
        return false;
    }
    if (!eth::verify_eip1559_header(prev_header, new_header)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] verifyEip1559Header failed, new: %s, old: %s", new_header.gas_limit.str().c_str(), prev_header.gas_limit.str().c_str());
        return false;
    }
#if !defined(XBUILD_DEV) && !defined(XBUILD_CI) && !defined(XBUILD_BOUNTY) && !defined(XBUILD_GALILEO)
    // step 6: verify difficulty
    bigint diff = eth::xethash_t::instance().calc_difficulty(new_header.time, prev_header);
    if (diff != new_header.difficulty) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] difficulty check mismatch");
        return false;
    }
    // step 7: verify ethash
    if (!eth::xethash_t::instance().verify_seal(new_header, nodes)) {
        xwarn("[xtop_evm_eth_bridge_contract::sync] ethash verify failed, header: %s", new_header.calc_hash().hex().c_str());
        return false;
    }
#endif
    return true;
}

bool xtop_evm_eth_bridge_contract::record(xeth_header_t const & header, state_ptr state) {
    h256 header_hash = header.calc_hash();
    h256 last_hash = get_last_hash(state);
    xeth_header_info_t last_info;
    if (!get_header_info(last_hash, last_info, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] get_header_info failed, hash: %s", last_hash.hex().c_str());
        return false;
    }
    if (header.number + BlockReserveNum < last_info.number) {
        xwarn("[xtop_evm_eth_bridge_contract::record] header is too old height: %" PRIu64 ", hash: %s, now height: %s",
              header.number,
              header_hash.hex().c_str(),
              last_info.number.str().c_str());
        return false;
    }
    xeth_header_info_t parent_info;
    if (!get_header_info(header.parent_hash, parent_info, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] get parent header_info failed, hash: %s", header.parent_hash.hex().c_str());
        return false;
    }
    auto all_hashes = get_hashes(header.number, state);
    if (all_hashes.count(header_hash)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] header existed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    all_hashes.insert(header_hash);
    if (!set_hashes(header.number, all_hashes, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] set hashes failed, height: %" PRIu64, header.number);
        return false;
    }
    if (!set_header(header_hash, header,state)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] set header failed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    xeth_header_info_t info{header.difficulty + parent_info.difficult_sum, header.parent_hash, header.number};
    if (!set_header_info(header.calc_hash(), info, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] set_header_info failed, height: %" PRIu64 ", hash: %s", header.number, header.calc_hash().hex().c_str());
        return false;
    }
    if (info.difficult_sum > last_info.difficult_sum || (info.difficult_sum == last_info.difficult_sum && header.difficulty % 2 == 0)) {
        if (!rebuild(header, last_info, info,state)) {
            xwarn("[xtop_evm_eth_bridge_contract::record] rebuild failed");
            return false;
        }
        release(header.number,state);
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::rebuild(xeth_header_t const & header, xeth_header_info_t const & last_info, xeth_header_info_t const & cur_info, state_ptr state) {
    if (last_info.number > cur_info.number) {
        for (bigint i = cur_info.number + 1; i <= last_info.number; ++i) {
            remove_effective_hash(i, state);
        }
    }
    auto header_hash = header.calc_hash();
    if (!set_last_hash(header_hash, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::record] set_last_hash failed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    if (!set_effective_hash(header.number, header_hash, state)) {
        xwarn("[xtop_evm_eth_bridge_contract::init] set_effective_hash failed, height: %" PRIu64 ", hash: %s", header.number, header_hash.hex().c_str());
        return false;
    }
    bigint height = header.number - 1;
    auto current_hash = cur_info.parent_hash;
    for (;;) {
        if (height == 0) {
            break;
        }
        if (get_effective_hash(height, state) == current_hash) {
            break;
        }
        if (!set_effective_hash(height, current_hash,state)) {
            xwarn("[xtop_evm_eth_bridge_contract::init] set_effective_hash failed, height: %s, hash: %s", height.str().c_str(), current_hash.hex().c_str());
            return false;
        }
        xeth_header_info_t info;
        if (get_header_info(current_hash, info, state)) {
            current_hash = info.parent_hash;
        } else {
            break;
        }
        height -= 1;
    }
    return true;
}

void xtop_evm_eth_bridge_contract::release(bigint const number, state_ptr state) {
    xeth_header_t header;
    bigint difficulty;
    h256 hash;
    if (number > BlockReserveNum) {
        bigint n = number - BlockReserveNum;
        for (;;) {
            auto hashes = get_hashes(n, state);
            if (hashes.empty()) {
                break;
            }
            for (auto h : hashes) {
                remove_header_info(h, state);
                remove_header(h, state);
            }
            remove_hashes(n, state);
            if (n == 0) {
                break;
            }
            n -= 1;
        }
    }
    if (number > HashReserveNum) {
        bigint n = number - HashReserveNum;
        for (;;) {
            if (get_effective_hash(n, state) == h256()) {
                break;
            }
            remove_effective_hash(n, state);
            if (n == 0) {
                break;
            }
            n -= 1;
        }
    }
}

bool xtop_evm_eth_bridge_contract::get_header(h256 const hash, xeth_header_t & header, state_ptr state) const {
    auto k = hash.asBytes();
    auto header_str = state->map_get(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()});
    if (header_str.empty()) {
        xwarn("[xtop_evm_eth_bridge_contract::get_header] get_header not exist, hash: %s", hash.hex().c_str());
        return false;
    }
    std::error_code ec;
    header.decode_rlp({std::begin(header_str), std::end(header_str)}, ec);
    if (ec) {
        xwarn("xtop_evm_eth_bridge_contract::get_header, xeth_header_t::decode_rlp, hash: %s msg %s", hash.hex().c_str(), ec.message().c_str());
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::set_header(h256 const hash, xeth_header_t const & header, state_ptr state) {
    auto k = hash.asBytes();
    auto v = header.encode_rlp();
    if (0 != state->map_set(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_header(h256 const hash, state_ptr state) {
    auto k = hash.asBytes();
    if (0 != state->map_remove(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::get_header_info(h256 const hash, xeth_header_info_t & header_info, state_ptr state) const {
    auto k = hash.asBytes();
    auto info_str = state->map_get(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()});
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

bool xtop_evm_eth_bridge_contract::set_header_info(h256 const hash, xeth_header_info_t const & header_info, state_ptr state) {
    auto k = hash.asBytes();
    auto v = header_info.encode_rlp();
    if (0 != state->map_set(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_header_info(h256 const hash, state_ptr state) {
    auto k = hash.asBytes();
    if (0 != state->map_remove(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

h256 xtop_evm_eth_bridge_contract::get_effective_hash(bigint const height, state_ptr state) const {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto hash_str = state->map_get(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()});
    if (hash_str.empty()) {
        return h256();
    }
    return static_cast<h256>(xbytes_t{std::begin(hash_str), std::end(hash_str)});
}

bool xtop_evm_eth_bridge_contract::set_effective_hash(bigint const height, h256 const hash, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto v = hash.asBytes();
    if (0 != state->map_set(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_effective_hash(bigint const height, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != state->map_remove(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

std::set<h256> xtop_evm_eth_bridge_contract::get_hashes(bigint const height, state_ptr state) const {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto str = state->map_get(data::system_contract::XPROPERTY_ALL_HASHES, {k.begin(), k.end()});
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

bool xtop_evm_eth_bridge_contract::set_hashes(bigint const height, std::set<h256> const & hashes, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    xbytes_t v;
    for (auto h : hashes) {
        auto bytes = RLP::encode(h.asBytes());
        v.insert(v.end(), bytes.begin(), bytes.end());
    }
    if (0 != state->map_set(data::system_contract::XPROPERTY_ALL_HASHES, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_eth_bridge_contract::remove_hashes(bigint const height, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != state->map_remove(data::system_contract::XPROPERTY_ALL_HASHES, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

h256 xtop_evm_eth_bridge_contract::get_last_hash(state_ptr state) const {
    auto hash_str = state->string_get(data::system_contract::XPROPERTY_LAST_HASH);
    if (hash_str.empty()) {
        return h256();
    }
    return static_cast<h256>(xbytes_t{std::begin(hash_str), std::end(hash_str)});
}

bool xtop_evm_eth_bridge_contract::set_last_hash(h256 const hash, state_ptr state) {
    auto bytes = hash.asBytes();
    if (0 != state->string_set(data::system_contract::XPROPERTY_LAST_HASH, {bytes.begin(), bytes.end()})) {
        return false;
    }
    return true;
}

int xtop_evm_eth_bridge_contract::get_flag(state_ptr state) const {
    auto flag_str = state->string_get(data::system_contract::XPROPERTY_RESET_FLAG);
    if (flag_str.empty()) {
        return -1;
    }
    return from_string<int>(flag_str);
}

bool xtop_evm_eth_bridge_contract::set_flag(state_ptr state) {
    if (0 != state->string_set(data::system_contract::XPROPERTY_RESET_FLAG, top::to_string(1))) {
        return false;
    }
    return true;
}

NS_END4
