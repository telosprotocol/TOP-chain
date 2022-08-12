// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_bsc_client_contract.h"

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xbasic/endianness.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xdata/xdata_common.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xcrosschain/xeth_header.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

using namespace top::evm_common;

constexpr uint64_t confirm_num = 15;
constexpr uint64_t validator_num = 21;
constexpr uint64_t hash_reserve_num = 40000;
constexpr uint64_t block_reserve_num = 500;

constexpr uint64_t epoch = 200;
constexpr uint64_t extra_vanity = 32;
constexpr uint64_t extra_seal = 64 + 1;
constexpr uint64_t address_length = 20;
constexpr uint64_t max_gas_limit = 0x7fffffffffffffff;
constexpr uint64_t min_gas_limit = 5000;
constexpr uint64_t gas_limit_bound_divisor = 256;
constexpr uint64_t bsc_chainid = 56;

auto static empty_unclehash = static_cast<h256>(from_hex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"));

bool xtop_evm_bsc_client_contract::init(const xbytes_t & rlp_bytes, state_ptr state) {
    // step 1: check init
    if (get_last_hash(state) != h256()) {
        xwarn("[xtop_evm_bsc_client_contract::init] init already");
        return false;
    }
    // step 2: decode
    auto left_bytes = std::move(rlp_bytes);
    std::vector<xeth_header_t> headers;
    while (!left_bytes.empty()) {
        auto item = RLP::decode_once(left_bytes);
        left_bytes = item.remainder;
        xeth_header_t header;
        if (header.decode_rlp(item.decoded[0]) == false) {
            xwarn("[xtop_evm_bsc_client_contract::init] header decode error");
            return false;
        };
        headers.emplace_back(header);
    }
    // min 12 to construnct state
    if (headers.size() < (validator_num / 2 + 1) + 1) {
        xwarn("[xtop_evm_bsc_client_contract::init] not enough headers");
        return false;
    }

    std::error_code ec;
    xvalidators_snapshot_t snap;
    if (!snap.init_with_epoch(headers[0])) {
        xwarn("[xtop_evm_bsc_client_contract::init] new_epoch_snapshot error");
        return false;
    }

    for (size_t i = 0; i < headers.size(); ++i) {
        auto const & h = headers[i];
        if (i != 0) {
            if (!snap.apply_with_chainid(h, bsc_chainid, false)) {
                xwarn("[xtop_evm_bsc_client_contract::init] apply_with_chainid failed");
                return false;
            }
        }
        auto snap_hash = snap.digest();
        auto header_hash = h.hash();
        // step 3: store with no check
        xinfo("[xtop_evm_bsc_client_contract::init] header dump: %s, snap_hash: %s", h.dump().c_str(), snap_hash.hex().c_str());
        if (!set_last_hash(header_hash, state)) {
            xwarn("[xtop_evm_bsc_client_contract::init] set_last_hash failed, hash: %s", header_hash.hex().c_str());
            return false;
        }
        if (!set_effective_hash(h.number, header_hash, state)) {
            xwarn("[xtop_evm_bsc_client_contract::init] set_effective_hash failed, height: %s, hash: %s", h.number.str().c_str(), header_hash.hex().c_str());
            return false;
        }
        if (!set_hashes(h.number, {header_hash}, state)) {
            xwarn("[xtop_evm_bsc_client_contract::init] set_hash failed, height: %s, hash: %s", h.number.str().c_str(), header_hash.hex().c_str());
            return false;
        }
        if (!set_header(header_hash, h, state)) {
            xwarn("[xtop_evm_bsc_client_contract::init] set_header failed, height: %s, hash: %s", h.number.str().c_str(), header_hash.hex().c_str());
            return false;
        }
        xvalidators_snap_info_t snap_info{snap_hash, h.parent_hash, h.number};
        if (!set_snap_info(header_hash, snap_info, state)) {
            xwarn("[xtop_evm_bsc_client_contract::init] set_snap_info failed, height: %s, hash: %s", h.number.str().c_str(), header_hash.hex().c_str());
            return false;
        }
    }

    xinfo("[xtop_evm_bsc_client_contract::init] init success");
    return true;
}

bool xtop_evm_bsc_client_contract::sync(const xbytes_t & rlp_bytes, state_ptr state) {
    // step 1: check init
    if (get_last_hash(state) == h256()) {
        xwarn("[xtop_evm_bsc_client_contract::sync] not init yet");
        return false;
    }

    auto left_bytes = std::move(rlp_bytes);
    while (left_bytes.size() != 0) {
        // step 2: decode
        auto item = RLP::decode(left_bytes);
        auto decoded_size = item.decoded.size();
        if (decoded_size < 2) {
            xwarn("[xtop_evm_bsc_client_contract::sync] sync param error");
            return false;
        }
        left_bytes = std::move(item.remainder);
        xeth_header_t header;
        {
            auto item_header = RLP::decode_once(item.decoded[0]);
            auto header_bytes = item_header.decoded[0];
            if (header.decode_rlp(header_bytes) == false) {
                xwarn("[xtop_evm_bsc_client_contract::sync] decode header error");
                return false;
            }
        }
        xinfo("[xtop_evm_bsc_client_contract::sync] header dump: %s", header.dump().c_str());

        xvalidators_snapshot_t snap;
        const uint32_t validator_num_index = 1;
        snap.number = static_cast<uint64_t>(header.number) - 1;
        snap.hash = header.parent_hash;
        auto validator_num = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[validator_num_index]));
        // check decoded_size with recent_num
        if (decoded_size < validator_num + validator_num_index + 1 + 1) {
            xwarn("[xtop_evm_bsc_client_contract::sync] sync param error");
            return false;
        }
        const uint32_t validators_index = validator_num_index + 1;
        for (uint64_t i = 0; i < validator_num; ++i) {
            snap.validators.insert(item.decoded[i + validators_index]);
        }
        const uint32_t recent_num_index = validator_num + validator_num_index + 1;
        auto recent_num = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[recent_num_index]));
        if (decoded_size < recent_num_index + 1 + recent_num) {
            xwarn("[xtop_evm_bsc_client_contract::sync] sync param error");
            return false;
        }
        const uint32_t recents_index = recent_num_index + 1;
        for (uint64_t i = 0; i < recent_num; ++i) {
            auto k = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[recents_index + i * 2]));
            snap.recents[k] = item.decoded[recents_index + i * 2 + 1];
        }

        xeth_header_t parent_header;
        if (!get_header(header.parent_hash, parent_header, state)) {
            xwarn("[xtop_evm_bsc_client_contract::sync] get parent header failed, hash: %s", header.parent_hash.hex().c_str());
            return false;
        }
        // step 5: verify header
        if (!verify(parent_header, header, snap, state)) {
            xwarn("[xtop_evm_bsc_client_contract::sync] verify header failed");
            return false;
        }
        // step 6: record
        if (!record(header, snap, state)) {
            xwarn("[xtop_evm_bsc_client_contract::sync] record header failed");
            return false;
        }
    }
    xinfo("[xtop_evm_bsc_client_contract::sync] sync success");
    return true;
}

bool xtop_evm_bsc_client_contract::reset(state_ptr state) {
    if (get_flag(state) != 0) {
        xwarn("[xtop_evm_bsc_client_contract::reset] flag not 0");
        return false;
    }
    state->map_clear(data::system_contract::XPROPERTY_EFFECTIVE_HASHES);
    state->map_clear(data::system_contract::XPROPERTY_ALL_HASHES);
    state->map_clear(data::system_contract::XPROPERTY_HEADERS);
    state->map_clear(data::system_contract::XPROPERTY_HEADERS_SUMMARY);
    set_last_hash(h256(), state);
    return true;
}

bool xtop_evm_bsc_client_contract::disable_reset(state_ptr state) {
    return set_flag(state);
}

bigint xtop_evm_bsc_client_contract::get_height(state_ptr state) const {
    h256 last_hash = get_last_hash(state);
    xvalidators_snap_info_t info;
    if (!get_snap_info(last_hash, info, state)) {
        xwarn("[xtop_evm_bsc_client_contract::get_height] get header info failed, hash: %s", last_hash.hex().c_str());
        return 0;
    }
    xinfo("[xtop_evm_bsc_client_contract::get_height] height: %s", info.number.str().c_str());
    return info.number;
}

xheader_status_t xtop_evm_bsc_client_contract::query_height(const u256 height, state_ptr state) const {
    auto now_height = get_height(state);
    if (now_height == 0) {
        return xheader_status_t::header_not_confirmed;
    }
    if (now_height >= height + hash_reserve_num) {
        return xheader_status_t::header_overdue;
    }
    if (now_height <= height + confirm_num) {
        return xheader_status_t::header_not_confirmed;
    }
    return xheader_status_t::header_confirmed;
}

bool xtop_evm_bsc_client_contract::is_known(const u256 height, const xbytes_t & hash_bytes, state_ptr state) const {
    h256 hash = static_cast<h256>(hash_bytes);
    auto hashes = get_hashes(height, state);
    if (!hashes.count(hash)) {
        xwarn("[xtop_evm_bsc_client_contract::is_known] not found, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_bsc_client_contract::is_known] is known, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
    return true;
}

bool xtop_evm_bsc_client_contract::is_confirmed(const u256 height, const xbytes_t & hash_bytes, state_ptr state) const {
    h256 hash = static_cast<h256>(hash_bytes);
    h256 origin_hash = get_effective_hash(height, state);
    if (hash != origin_hash) {
        xwarn("[xtop_evm_bsc_client_contract::is_confirmed] hash mismatch: %s, %s", hash.hex().c_str(), origin_hash.hex().c_str());
        return false;
    }
    h256 last_hash = get_last_hash(state);
    xvalidators_snap_info_t info;
    if (!get_snap_info(last_hash, info, state)) {
        xwarn("[xtop_evm_bsc_client_contract::is_confirmed] get header info failed, hash: %s", last_hash.hex().c_str());
        return false;
    }
    if (height + confirm_num > info.number) {
        xwarn("[xtop_evm_bsc_client_contract::is_confirmed] height not confirmed: %s, %s, limit: %d", info.number.str().c_str(), height.str().c_str(), confirm_num);
        return false;
    }
    xinfo("[xtop_evm_bsc_client_contract::is_confirmed] is_confirmed: %s", hash.hex().c_str());
    return true;
}

bool xtop_evm_bsc_client_contract::verify(const xeth_header_t & prev_header, const xeth_header_t & new_header, xvalidators_snapshot_t & snap, state_ptr state) const {
    if (new_header.extra.size() < extra_vanity) {
        xwarn("[xtop_evm_bsc_client_contract::verify] header extra size error: %lu, should < %lu", new_header.extra.size(), extra_vanity);
        return false;
    }
    if (new_header.extra.size() < extra_vanity + extra_seal) {
        xwarn("[xtop_evm_bsc_client_contract::verify] header extra miss signature: %lu, should < %lu", new_header.extra.size(), extra_vanity + extra_seal);
        return false;
    }
    bool is_epoch = (new_header.number % epoch == 0);
    uint64_t validators_bytes = new_header.extra.size() - extra_vanity - extra_seal;
    if (!is_epoch && validators_bytes != 0) {
        xwarn("[xtop_evm_bsc_client_contract::verify] not epoch but has validators_bytes");
        return false;
    }
    if (is_epoch && validators_bytes % address_length != 0) {
        xwarn("[xtop_evm_bsc_client_contract::verify] validators_bytes length error: %lu", validators_bytes);
        return false;
    }
    if (new_header.mix_digest != h256(0)) {
        xwarn("[xtop_evm_bsc_client_contract::verify] mix_digest not 0: %lu", new_header.mix_digest.hex().c_str());
        return false;
    }
    if (new_header.uncle_hash != empty_unclehash) {
        xwarn("[xtop_evm_bsc_client_contract::verify] uncle_hash mismatch: %s, should be: %s", new_header.uncle_hash.hex().c_str(), empty_unclehash.hex().c_str());
        return false;
    }
    if (new_header.gas_limit > max_gas_limit) {
        xwarn("[xtop_evm_bsc_client_contract::verify] gaslimit too big: %lu > %lu", new_header.gas_limit, max_gas_limit);
        return false;
    }
    if (new_header.gas_used > new_header.gas_limit) {
        xwarn("[xtop_evm_bsc_client_contract::verify] gasUsed: %lu > gasLimit: %lu", new_header.gas_used, new_header.gas_limit);
        return false;
    }
    bigint diff = bigint(prev_header.gas_limit) - bigint(new_header.gas_limit);
    if (diff < 0) {
        diff *= -1;
    }
    bigint limit = prev_header.gas_limit / gas_limit_bound_divisor;
    if (uint64_t(diff) >= limit || new_header.gas_limit < min_gas_limit) {
        xwarn("[xtop_evm_bsc_client_contract::verify] invalid gas limit: have %lu, want %lu + %lu", new_header.gas_limit, prev_header.gas_limit, diff.str().c_str());
        return false;
    }
    if (new_header.number != prev_header.number + 1) {
        xwarn("[xtop_evm_bsc_client_contract::verify] height mismatch, new: %s, old: %s", new_header.number.str().c_str(), prev_header.number.str().c_str());
        return false;
    }

    h256 last_hash = get_last_hash(state);
    xvalidators_snap_info_t last_info;
    if (!get_snap_info(last_hash, last_info, state)) {
        xwarn("[xtop_evm_bsc_client_contract::verify] get_header_info failed, hash: %s", last_hash.hex().c_str());
        return false;
    }
    auto snap_hash = snap.digest();
    if (last_info.snap_hash != snap_hash) {
        xwarn("[xtop_evm_bsc_client_contract::verify] snap hash mismatch: %s", last_info.snap_hash.hex().c_str(), snap_hash.hex().c_str());
        return false;
    }
    if (!snap.apply_with_chainid(new_header, bsc_chainid, true)) {
        xwarn("[xtop_evm_bsc_client_contract::verify] snap apply_with_chainid failed");
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::record(const xeth_header_t & header, const xvalidators_snapshot_t & snap, state_ptr state) {
    h256 header_hash = header.hash();
    h256 last_hash = get_last_hash(state);
    xvalidators_snap_info_t last_info;
    if (!get_snap_info(last_hash, last_info, state)) {
        xwarn("[xtop_evm_bsc_client_contract::record] get_header_info failed, hash: %s", last_hash.hex().c_str());
        return false;
    }
    if (header.number + block_reserve_num < last_info.number) {
        xwarn("[xtop_evm_bsc_client_contract::record] header is too old height: %s, hash: %s, now height: %s",
              header.number.str().c_str(),
              header_hash.hex().c_str(),
              last_info.number.str().c_str());
        return false;
    }
    xvalidators_snap_info_t parent_info;
    if (!get_snap_info(header.parent_hash, parent_info, state)) {
        xwarn("[xtop_evm_bsc_client_contract::record] get parent header_info failed, hash: %s", header.parent_hash.hex().c_str());
        return false;
    }
    auto all_hashes = get_hashes(header.number, state);
    if (all_hashes.count(header_hash)) {
        xwarn("[xtop_evm_bsc_client_contract::record] header existed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    all_hashes.insert(header_hash);
    if (!set_hashes(header.number, all_hashes, state)) {
        xwarn("[xtop_evm_bsc_client_contract::record] set hashes failed, height: %s", header.number.str().c_str());
        return false;
    }
    if (!set_header(header_hash, header, state)) {
        xwarn("[xtop_evm_bsc_client_contract::record] set header failed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    xvalidators_snap_info_t info{snap.digest(), header.parent_hash, header.number};
    if (!set_snap_info(header.hash(), info, state)) {
        xwarn("[xtop_evm_bsc_client_contract::record] set_header_info failed, height: %s, hash: %s", header.number.str().c_str(), header.hash().hex().c_str());
        return false;
    }
    if (!rebuild(header, last_info, info, state)) {
        xwarn("[xtop_evm_bsc_client_contract::record] rebuild failed");
        return false;
    }
    release(header.number, state);
    return true;
}

bool xtop_evm_bsc_client_contract::rebuild(const xeth_header_t & header, const xvalidators_snap_info_t & last_info, const xvalidators_snap_info_t & cur_info, state_ptr state) {
    if (last_info.number > cur_info.number) {
        for (bigint i = cur_info.number + 1; i <= last_info.number; ++i) {
            remove_effective_hash(i, state);
        }
    }
    auto header_hash = header.hash();
    if (!set_last_hash(header_hash, state)) {
        xwarn("[xtop_evm_bsc_client_contract::record] set_last_hash failed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    if (!set_effective_hash(header.number, header_hash, state)) {
        xwarn("[xtop_evm_bsc_client_contract::init] set_effective_hash failed, height: %s, hash: %s", header.number.str().c_str(), header_hash.hex().c_str());
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
        if (!set_effective_hash(height, current_hash, state)) {
            xwarn("[xtop_evm_bsc_client_contract::init] set_effective_hash failed, height: %s, hash: %s", height.str().c_str(), current_hash.hex().c_str());
            return false;
        }
        xvalidators_snap_info_t info;
        if (get_snap_info(current_hash, info, state)) {
            current_hash = info.parent_hash;
        } else {
            break;
        }
        height -= 1;
    }
    return true;
}

void xtop_evm_bsc_client_contract::release(const bigint number, state_ptr state) {
    xeth_header_t header;
    bigint difficulty;
    h256 hash;
    if (number > block_reserve_num) {
        bigint n = number - block_reserve_num;
        for (;;) {
            auto hashes = get_hashes(n, state);
            if (hashes.empty()) {
                break;
            }
            for (auto h : hashes) {
                remove_snap_info(h, state);
                remove_header(h, state);
            }
            remove_hashes(n, state);
            if (n == 0) {
                break;
            }
            n -= 1;
        }
    }
    if (number > hash_reserve_num) {
        bigint n = number - hash_reserve_num;
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

bool xtop_evm_bsc_client_contract::get_header(const h256 hash, xeth_header_t & header, state_ptr state) const {
    auto k = hash.asBytes();
    auto header_str = state->map_get(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()});
    if (header_str.empty()) {
        xwarn("[xtop_evm_bsc_client_contract::get_header] get_header not exist, hash: %s", hash.hex().c_str());
        return false;
    }
    if (header.decode_rlp({std::begin(header_str), std::end(header_str)}) == false) {
        xwarn("[xtop_evm_bsc_client_contract::get_header] decode_header failed, hash: %s", hash.hex().c_str());
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::set_header(const h256 hash, const xeth_header_t & header, state_ptr state) {
    auto k = hash.asBytes();
    auto v = header.encode_rlp();
    if (0 != state->map_set(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::remove_header(const h256 hash, state_ptr state) {
    auto k = hash.asBytes();
    if (0 != state->map_remove(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::get_snap_info(const h256 hash, xvalidators_snap_info_t & header_info, state_ptr state) const {
    auto k = hash.asBytes();
    auto info_str = state->map_get(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()});
    if (info_str.empty()) {
        xwarn("[xtop_evm_bsc_client_contract::get_snap_info] get_header not exist, hash: %s", hash.hex().c_str());
        return false;
    }
    if (header_info.decode_rlp({std::begin(info_str), std::end(info_str)}) == false) {
        xwarn("[xtop_evm_bsc_client_contract::get_snap_info] decode_header failed, hash: %s", hash.hex().c_str());
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::set_snap_info(const h256 hash, const xvalidators_snap_info_t & snap_info, state_ptr state) {
    auto k = hash.asBytes();
    auto v = snap_info.encode_rlp();
    if (0 != state->map_set(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::remove_snap_info(const h256 hash, state_ptr state) {
    auto k = hash.asBytes();
    if (0 != state->map_remove(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

h256 xtop_evm_bsc_client_contract::get_effective_hash(const bigint height, state_ptr state) const {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto hash_str = state->map_get(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()});
    if (hash_str.empty()) {
        return h256();
    }
    return static_cast<h256>(xbytes_t{std::begin(hash_str), std::end(hash_str)});
}

bool xtop_evm_bsc_client_contract::set_effective_hash(const bigint height, const h256 hash, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto v = hash.asBytes();
    if (0 != state->map_set(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::remove_effective_hash(const bigint height, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != state->map_remove(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

std::set<h256> xtop_evm_bsc_client_contract::get_hashes(const bigint height, state_ptr state) const {
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

bool xtop_evm_bsc_client_contract::set_hashes(const bigint height, const std::set<h256> & hashes, state_ptr state) {
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

bool xtop_evm_bsc_client_contract::remove_hashes(const bigint height, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != state->map_remove(data::system_contract::XPROPERTY_ALL_HASHES, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

h256 xtop_evm_bsc_client_contract::get_last_hash(state_ptr state) const {
    auto hash_str = state->string_get(data::system_contract::XPROPERTY_LAST_HASH);
    if (hash_str.empty()) {
        return h256();
    }
    return static_cast<h256>(xbytes_t{std::begin(hash_str), std::end(hash_str)});
}

bool xtop_evm_bsc_client_contract::set_last_hash(const h256 hash, state_ptr state) {
    auto bytes = hash.asBytes();
    if (0 != state->string_set(data::system_contract::XPROPERTY_LAST_HASH, {bytes.begin(), bytes.end()})) {
        return false;
    }
    return true;
}

int xtop_evm_bsc_client_contract::get_flag(state_ptr state) const {
    auto flag_str = state->string_get(data::system_contract::XPROPERTY_RESET_FLAG);
    if (flag_str.empty()) {
        return -1;
    }
    return from_string<int>(flag_str);
}

bool xtop_evm_bsc_client_contract::set_flag(state_ptr state) {
    if (0 != state->string_set(data::system_contract::XPROPERTY_RESET_FLAG, top::to_string(1))) {
        return false;
    }
    return true;
}

NS_END4
