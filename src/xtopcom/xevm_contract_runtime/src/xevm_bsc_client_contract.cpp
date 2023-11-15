// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_bsc_client_contract.h"

#include "xbasic/endianness.h"
#include "xcommon/common_data.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xdata/xdata_common.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xcrosschain/xbsc/xutility.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_contract_runtime/xerror/xerror.h"

#if defined(XCXX20)
#include <fifo_map.hpp>
#else
#include <nlohmann/fifo_map.hpp>
#endif
#include <nlohmann/json.hpp>

using namespace top::evm_common;

using namespace top::evm::crosschain::bsc;

NS_BEG4(top, contract_runtime, evm, sys_contract)

constexpr uint64_t confirm_num = 15;
constexpr uint64_t VALIDATOR_NUM = 21;
constexpr uint64_t hash_reserve_num = 40000;
constexpr uint64_t block_reserve_num = 500;

constexpr uint64_t epoch = 200;
constexpr uint64_t address_length = 20;
constexpr uint64_t max_gas_limit = 0x7fffffffffffffff;
constexpr uint64_t MIN_GAS_LIMIT = 5000;
constexpr uint64_t GAS_LIMIT_BOUND_DIVISOR = 256;
constexpr uint64_t bsc_chainid = 56;

auto static empty_unclehash = static_cast<h256>(from_hex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347"));

bool xtop_evm_bsc_client_contract::init(xbytes_t const & rlp_bytes, state_ptr state) {
    // step 1: check init
    if (!get_last_hash(state).empty()) {
        xwarn("[xtop_evm_bsc_client_contract::init] init already");
        return false;
    }
    // step 2: decode
    auto left_bytes = rlp_bytes;
    std::vector<xeth_header_t> headers;
    std::error_code ec;
    while (!left_bytes.empty()) {
        auto item = RLP::decode_once(left_bytes);
        left_bytes = item.remainder;
        xeth_header_t header;
        header.decode_rlp(item.decoded[0], ec);
        if (ec) {
            xwarn("[xtop_evm_bsc_client_contract::init] header decode error. msg %s", ec.message().c_str());
            return false;
        }
        headers.emplace_back(header);
    }

    // two epoch headers + VALIDATOR_NUM / 2  headers
    if (headers.size() < (VALIDATOR_NUM / 2 + 1) + 1) {
        xwarn("[xtop_evm_bsc_client_contract::init] not enough headers");
        return false;
    }


    //xsnapshot_t last_snapshot = xsnapshot_t::new_snapshot(headers[0].number, headers[0].hash);

    //xvalidators_snapshot_t snap;
    //if (!snap.init_with_double_epoch(headers[0], headers[1])) {
    //    xwarn("[xtop_evm_bsc_client_contract::init] new_epoch_snapshot error");
    //    return false;
    //}

    //for (size_t i = 0; i < headers.size(); ++i) {
    //    if (i == 0) {
    //        continue;
    //    }
    //    auto const & h = headers[i];
    //    if (i != 1) {
    //        if (!snap.apply_with_chainid(h, bsc_chainid, false)) {
    //            xwarn("[xtop_evm_bsc_client_contract::init] apply_with_chainid failed");
    //            return false;
    //        }
    //    }
    //    auto snap_hash = snap.digest();
    //    auto header_hash = h.calc_hash();
    //    // step 3: store with no check
    //    xinfo("[xtop_evm_bsc_client_contract::init] header dump: %s, snap_hash: %s", h.dump().c_str(), snap_hash.hex().c_str());
    //    if (!set_last_hash(header_hash, state)) {
    //        xwarn("[xtop_evm_bsc_client_contract::init] set_last_hash failed, hash: %s", header_hash.hex().c_str());
    //        return false;
    //    }
    //    if (!set_effective_hash(h.number, header_hash, state)) {
    //        xwarn("[xtop_evm_bsc_client_contract::init] set_effective_hash failed, height: %" PRIu64 ", hash: %s", h.number, header_hash.hex().c_str());
    //        return false;
    //    }
    //    if (!set_hashes(h.number, {header_hash}, state)) {
    //        xwarn("[xtop_evm_bsc_client_contract::init] set_hash failed, height: %" PRIu64 ", hash: %s", h.number, header_hash.hex().c_str());
    //        return false;
    //    }
    //    if (!set_header(header_hash, h, state)) {
    //        xwarn("[xtop_evm_bsc_client_contract::init] set_header failed, height: %" PRIu64 ", hash: %s", h.number, header_hash.hex().c_str());
    //        return false;
    //    }
    //    xvalidators_snap_info_t snap_info{snap_hash, h.parent_hash, h.number};
    //    if (!set_snap_info(header_hash, snap_info, state)) {
    //        xwarn("[xtop_evm_bsc_client_contract::init] set_snap_info failed, height: %" PRIu64 ", hash: %s", h.number, header_hash.hex().c_str());
    //        return false;
    //    }
    //}

    xinfo("[xtop_evm_bsc_client_contract::init] init success");
    return true;
}

bool xtop_evm_bsc_client_contract::sync(xbytes_t const & rlp_bytes, state_ptr state) {
    // step 1: check init
    if (get_last_hash(state) == h256()) {
        xwarn("[xtop_evm_bsc_client_contract::sync] not init yet");
        return false;
    }

    auto left_bytes = rlp_bytes;
    std::error_code ec;
    while (!left_bytes.empty()) {
        // step 2: decode
        auto item = RLP::decode(left_bytes);
        auto const decoded_size = item.decoded.size();
        if (decoded_size < 2) {
            xwarn("[xtop_evm_bsc_client_contract::sync] sync param error");
            return false;
        }
        left_bytes = std::move(item.remainder);
        xeth_header_t header;
        {
            auto item_header = RLP::decode_once(item.decoded[0]);
            auto header_bytes = item_header.decoded[0];
            header.decode_rlp(header_bytes, ec);
            if (ec) {
                xwarn("xtop_evm_bsc_client_contract::sync, decode header error, msg %s", ec.message().c_str());
                return false;
            }
        }
        xinfo("[xtop_evm_bsc_client_contract::sync] header dump: %s", header.dump().c_str());

        xvalidators_snapshot_t snap;
        constexpr uint32_t validator_num_index = 1;
        snap.number = header.number - 1;
        snap.hash = header.parent_hash;
        auto const validators_num = evm_common::fromBigEndian<uint64_t>(item.decoded[validator_num_index]);
        // check decoded_size with recent_num
        if (decoded_size < validators_num + validator_num_index + 1 + 1) {
            xwarn("[xtop_evm_bsc_client_contract::sync] sync param error");
            return false;
        }
        constexpr uint32_t validators_index = validator_num_index + 1;
        for (uint64_t i = 0; i < validators_num; ++i) {
            snap.validators.insert(common::xeth_address_t::build_from(item.decoded[i + validators_index]));
        }

        uint64_t const last_validator_num_index = validators_num + validator_num_index + 1;
        auto const last_validator_num = evm_common::fromBigEndian<uint64_t>(item.decoded[last_validator_num_index]);
        if (decoded_size < last_validator_num_index + 1 + last_validator_num) {
            xwarn("[xtop_evm_bsc_client_contract::sync] sync param error");
            return false;
        }
        uint64_t const last_validators_index = last_validator_num_index + 1;
        for (uint64_t i = 0; i < last_validator_num; ++i) {
            snap.last_validators.insert(common::xeth_address_t::build_from(item.decoded[i + last_validators_index]));
        }

        uint64_t const recent_num_index = last_validator_num + last_validator_num_index + 1;
        auto const recent_num = evm_common::fromBigEndian<uint64_t>(item.decoded[recent_num_index]);
        if (decoded_size < recent_num_index + 1 + recent_num) {
            xwarn("[xtop_evm_bsc_client_contract::sync] sync param error");
            return false;
        }
        uint32_t const recents_index = recent_num_index + 1;
        for (uint64_t i = 0; i < recent_num; ++i) {
            auto k = evm_common::fromBigEndian<uint64_t>(item.decoded[recents_index + i * 2]);
            snap.recents[k] = common::xeth_address_t::build_from(item.decoded[recents_index + i * 2 + 1]);
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

xheader_status_t xtop_evm_bsc_client_contract::query_height(u256 const height, state_ptr state) const {
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

bool xtop_evm_bsc_client_contract::is_known(u256 const height, xbytes_t const & hash_bytes, state_ptr state) const {
    h256 hash = static_cast<h256>(hash_bytes);
    auto hashes = get_hashes(height, state);
    if (!hashes.count(hash)) {
        xwarn("[xtop_evm_bsc_client_contract::is_known] not found, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
        return false;
    }
    xinfo("[xtop_evm_bsc_client_contract::is_known] is known, height: %s, hash: %s", height.str().c_str(), hash.hex().c_str());
    return true;
}

bool xtop_evm_bsc_client_contract::is_confirmed(u256 const height, xbytes_t const & hash_bytes, state_ptr state) const {
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

bool xtop_evm_bsc_client_contract::verify(xeth_header_t const & prev_header, xeth_header_t const & new_header, xvalidators_snapshot_t & snap, state_ptr state) const {
    if (new_header.extra.size() < top::evm::crosschain::bsc::EXTRA_VANITY) {
        xwarn("xtop_evm_bsc_client_contract::verify: header extra missing EXTRA_VANITY. extra size %" PRIu64, new_header.extra.size());
        return false;
    }

    if (new_header.extra.size() < top::evm::crosschain::bsc::EXTRA_VANITY + top::evm::crosschain::bsc::EXTRA_SEAL) {
        xwarn("xtop_evm_bsc_client_contract::verify: header extra missing signature. extra size %" PRIu64, new_header.extra.size());
        return false;
    }

    std::error_code ec;

    bool const is_epoch = new_header.number % epoch == 0;
    auto const signers_bytes =
        get_validator_bytes_from_header(new_header, top::evm::crosschain::bsc::bsc_chain_config, top::evm::crosschain::bsc::bsc_chain_config.parlia_config, ec);
    if (ec) {
        xwarn("xtop_evm_bsc_client_contract::verify: get_validator_bytes_from_header failed, msg: %s", ec.message().c_str());
        return false;
    }

    // Ensure that the extra-data contains a signer list on checkpoint, but none otherwise
    if (!is_epoch && !signers_bytes.empty()) {
        xwarn("xtop_evm_bsc_client_contract::verify: extra validators");
        return false;
    }

    if (is_epoch && signers_bytes.empty()) {
        xwarn("xtop_evm_bsc_client_contract::verify: invalid span validators");
        return false;
    }

    // Ensure that the mix digest is zero as we don't have fork protection currently
    if (new_header.mix_digest != h256(0)) {
        xwarn("xtop_evm_bsc_client_contract::verify: invalid mix digest. %s", new_header.mix_digest.hex().c_str());
        return false;
    }

    // Ensure that the block doesn't contain any uncles which are meaningless in PoA
    if (new_header.uncle_hash != empty_unclehash) {
        xwarn("xtop_evm_bsc_client_contract::verify: invalid uncle hash. %s, should be: %s", new_header.uncle_hash.hex().c_str(), empty_unclehash.hex().c_str());
        return false;
    }

    if (new_header.number > 0) {
        if (new_header.difficulty == 0) {
            xwarn("xtop_evm_bsc_client_contract::verify: invalid difficulty. block number %" PRIu64, new_header.number);
            return false;
        }
    }

    if (!verify_fork_hashes(top::evm::crosschain::bsc::bsc_chain_config, new_header, false)) {
        xwarn("xtop_evm_bsc_client_contract::verify: verify_fork_hashes failed");
        return false;
    }

    if (prev_header.number + 1 != new_header.number || prev_header.hash != new_header.parent_hash) {
        xwarn("xtop_evm_bsc_client_contract::verify: unknown ancestor. parent hash and number from current header: %s:%" PRIu64 ", specified parent hash and number: %s:" PRIu64, new_header.parent_hash.hex().c_str(), new_header.number - 1, prev_header.hash.hex().c_str(), prev_header.number);
        return false;
    }

    if (!top::evm::crosschain::bsc::bsc_chain_config.is_london(new_header.number)) {
        xwarn("xtop_evm_bsc_client_contract::verify: not london fork");
        return false;
    }

    if (!verify_eip1559_header(top::evm::crosschain::bsc::bsc_chain_config, prev_header, new_header)) {
        xwarn("xtop_evm_bsc_client_contract::verify: verify_eip1559_header failed");
        return false;
    }

    if (new_header.gas_limit > max_gas_limit) {
        xwarn("xtop_evm_bsc_client_contract::verify: gaslimit too big: %s > %lu", new_header.gas_limit.str().c_str(), max_gas_limit);
        return false;
    }
    if (new_header.gas_used > new_header.gas_limit) {
        xwarn("xtop_evm_bsc_client_contract::verify: gasUsed: %s > gasLimit: %s", new_header.gas_used.str().c_str(), new_header.gas_limit.str().c_str());
        return false;
    }
    auto diff = prev_header.gas_limit - new_header.gas_limit;
    if (diff < 0) {
        diff *= -1;
    }
    auto const limit = prev_header.gas_limit / GAS_LIMIT_BOUND_DIVISOR;
    if (diff >= limit || new_header.gas_limit < MIN_GAS_LIMIT) {
        xwarn("xtop_evm_bsc_client_contract::verify: invalid gas limit: have %s, want %s + %s",
              new_header.gas_limit.str().c_str(),
              prev_header.gas_limit.str().c_str(),
              diff.str().c_str());
        return false;
    }
    if (new_header.number != prev_header.number + 1) {
        xwarn("xtop_evm_bsc_client_contract::verify: height mismatch, new: %" PRIu64 ", old: %" PRIu64, new_header.number, prev_header.number);
        return false;
    }

    h256 const last_hash = get_last_hash(state);
    xvalidators_snap_info_t last_info;
    if (!get_snap_info(last_hash, last_info, state)) {
        xwarn("xtop_evm_bsc_client_contract::verify: get_header_info failed, hash: %s", last_hash.hex().c_str());
        return false;
    }
    //auto const snap_hash = snap.digest();
    //if (last_info.snap_hash != snap_hash) {
    //    xwarn("xtop_evm_bsc_client_contract::verify: snap hash mismatch: %s", last_info.snap_hash.hex().c_str(), snap_hash.hex().c_str());
    //    return false;
    //}
    //if (!snap.apply_with_chainid(new_header, bsc_chainid, true)) {
    //    xwarn("xtop_evm_bsc_client_contract::verify: snap apply_with_chainid failed");
    //    return false;
    //}
    return true;
}

bool xtop_evm_bsc_client_contract::record(xeth_header_t const & header, xvalidators_snapshot_t const & snap, state_ptr state) {
    //h256 header_hash = header.calc_hash();
    //h256 last_hash = get_last_hash(state);
    //xvalidators_snap_info_t last_info;
    //if (!get_snap_info(last_hash, last_info, state)) {
    //    xwarn("[xtop_evm_bsc_client_contract::record] get_header_info failed, hash: %s", last_hash.hex().c_str());
    //    return false;
    //}
    //if (header.number + block_reserve_num < last_info.number) {
    //    xwarn("[xtop_evm_bsc_client_contract::record] header is too old height: %" PRIu64 ", hash: %s, now height: %s",
    //          header.number,
    //          header_hash.hex().c_str(),
    //          last_info.number.str().c_str());
    //    return false;
    //}
    //xvalidators_snap_info_t parent_info;
    //if (!get_snap_info(header.parent_hash, parent_info, state)) {
    //    xwarn("[xtop_evm_bsc_client_contract::record] get parent header_info failed, hash: %s", header.parent_hash.hex().c_str());
    //    return false;
    //}
    //auto all_hashes = get_hashes(header.number, state);
    //if (all_hashes.count(header_hash)) {
    //    xwarn("[xtop_evm_bsc_client_contract::record] header existed, hash: %s", header_hash.hex().c_str());
    //    return false;
    //}
    //all_hashes.insert(header_hash);
    //if (!set_hashes(header.number, all_hashes, state)) {
    //    xwarn("[xtop_evm_bsc_client_contract::record] set hashes failed, height: %" PRIu64, header.number);
    //    return false;
    //}
    //if (!set_header(header_hash, header, state)) {
    //    xwarn("[xtop_evm_bsc_client_contract::record] set header failed, hash: %s", header_hash.hex().c_str());
    //    return false;
    //}
    //xvalidators_snap_info_t info{snap.digest(), header.parent_hash, header.number};
    //if (!set_snap_info(header.calc_hash(), info, state)) {
    //    xwarn("[xtop_evm_bsc_client_contract::record] set_header_info failed, height: %" PRIu64 ", hash: %s", header.number, header.calc_hash().hex().c_str());
    //    return false;
    //}
    //if (!rebuild(header, last_info, info, state)) {
    //    xwarn("[xtop_evm_bsc_client_contract::record] rebuild failed");
    //    return false;
    //}
    //release(header.number, state);
    return true;
}

bool xtop_evm_bsc_client_contract::rebuild(xeth_header_t const & header, xvalidators_snap_info_t const & last_info, xvalidators_snap_info_t const & cur_info, state_ptr state) {
    if (last_info.number > cur_info.number) {
        for (bigint i = cur_info.number + 1; i <= last_info.number; ++i) {
            remove_effective_hash(i, state);
        }
    }
    auto const header_hash = header.calc_hash();
    if (!set_last_hash(header_hash, state)) {
        xwarn("[xtop_evm_bsc_client_contract::record] set_last_hash failed, hash: %s", header_hash.hex().c_str());
        return false;
    }
    if (!set_effective_hash(header.number, header_hash, state)) {
        xwarn("[xtop_evm_bsc_client_contract::init] set_effective_hash failed, height: %" PRIu64 ", hash: %s", header.number, header_hash.hex().c_str());
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

void xtop_evm_bsc_client_contract::release(bigint const number, state_ptr state) {
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

bool xtop_evm_bsc_client_contract::get_header(h256 const hash, xeth_header_t & header, state_ptr state) const {
    auto k = hash.asBytes();
    auto header_str = state->map_get(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()});
    if (header_str.empty()) {
        xwarn("[xtop_evm_bsc_client_contract::get_header] get_header not exist, hash: %s", hash.hex().c_str());
        return false;
    }
    std::error_code ec;
    header.decode_rlp({std::begin(header_str), std::end(header_str)}, ec);
    if (ec) {
        xwarn("xtop_evm_bsc_client_contract::get_header, decode_header failed, hash: %s, msg %s", hash.hex().c_str(), ec.message().c_str());
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::set_header(h256 const hash, xeth_header_t const & header, state_ptr state) {
    auto k = hash.asBytes();
    auto v = header.encode_rlp();
    if (0 != state->map_set(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::remove_header(h256 const hash, state_ptr state) {
    auto k = hash.asBytes();
    if (0 != state->map_remove(data::system_contract::XPROPERTY_HEADERS, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::get_snap_info(h256 const hash, xvalidators_snap_info_t & header_info, state_ptr state) const {
    //auto k = hash.asBytes();
    //auto info_str = state->map_get(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()});
    //if (info_str.empty()) {
    //    xwarn("[xtop_evm_bsc_client_contract::get_snap_info] get_header not exist, hash: %s", hash.hex().c_str());
    //    return false;
    //}
    //if (header_info.decode_rlp({std::begin(info_str), std::end(info_str)}) == false) {
    //    xwarn("[xtop_evm_bsc_client_contract::get_snap_info] decode_header failed, hash: %s", hash.hex().c_str());
    //    return false;
    //}
    return true;
}

bool xtop_evm_bsc_client_contract::set_snap_info(h256 const hash, xvalidators_snap_info_t const & snap_info, state_ptr state) {
    //auto k = hash.asBytes();
    //auto v = snap_info.encode_rlp();
    //if (0 != state->map_set(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()}, {v.begin(), v.end()})) {
    //    return false;
    //}
    return true;
}

bool xtop_evm_bsc_client_contract::remove_snap_info(h256 const hash, state_ptr state) {
    auto k = hash.asBytes();
    if (0 != state->map_remove(data::system_contract::XPROPERTY_HEADERS_SUMMARY, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

h256 xtop_evm_bsc_client_contract::get_effective_hash(bigint const height, state_ptr state) const {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto hash_str = state->map_get(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()});
    if (hash_str.empty()) {
        return h256();
    }
    return static_cast<h256>(xbytes_t{std::begin(hash_str), std::end(hash_str)});
}

bool xtop_evm_bsc_client_contract::set_effective_hash(bigint const height, h256 const hash, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    auto v = hash.asBytes();
    if (0 != state->map_set(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        return false;
    }
    return true;
}

bool xtop_evm_bsc_client_contract::remove_effective_hash(bigint const height, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != state->map_remove(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

std::set<h256> xtop_evm_bsc_client_contract::get_hashes(bigint const height, state_ptr state) const {
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

bool xtop_evm_bsc_client_contract::set_hashes(bigint const height, std::set<h256> const & hashes, state_ptr state) {
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

bool xtop_evm_bsc_client_contract::remove_hashes(bigint const height, state_ptr state) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != state->map_remove(data::system_contract::XPROPERTY_ALL_HASHES, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

h256 xtop_evm_bsc_client_contract::get_last_hash(state_ptr const & state) const {
    auto hash_str = state->string_get(data::system_contract::XPROPERTY_LAST_HASH);
    if (hash_str.empty()) {
        return h256{};
    }
    return h256{xbytes_t{std::begin(hash_str), std::end(hash_str)}};
}

bool xtop_evm_bsc_client_contract::set_last_hash(h256 const & hash, state_ptr const & state) {
    auto bytes = hash.asBytes();
    return 0 == state->string_set(data::system_contract::XPROPERTY_LAST_HASH, {bytes.begin(), bytes.end()});
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

bool xtop_evm_bsc_client_contract::verify_fork_hashes(xchain_config_t const & chain_config, xeth_header_t const & header, bool const uncle) {
    if (uncle) {
        return true;
    }

    if (chain_config.eip150_block > 0 && header.number == chain_config.eip150_block) {
        if (chain_config.eip150_hash != xh256_t{0} && chain_config.eip150_hash != header.hash) {
            return false;
        }
    }

    return true;
}

bool xtop_evm_bsc_client_contract::verify_eip1559_header(xchain_config_t const & chain_config,
                                                         xeth_header_t const & parent,
                                                         xeth_header_t const & header) {
    if (!header.base_fee_per_gas.has_value()) {
        xwarn("xtop_evm_bsc_client_contract::verify_eip1559_header: header is missing baseFee");
        return false;
    }

    uint64_t const expected_base_fee = calc_base_fee(chain_config, parent);

    if (header.base_fee_per_gas.value() != expected_base_fee) {
        xwarn("xtop_evm_bsc_client_contract::verify_eip1559_header: base fee mismatch: %" PRIu64 ", %" PRIu64, header.base_fee_per_gas.value(), expected_base_fee);
        return false;
    }

    return true;
}

void xtop_evm_bsc_client_contract::verify_eip4844_header(xeth_header_t const & parent, xeth_header_t const & header, std::error_code & ec) {
    assert(!ec);

    if (!header.excess_blob_gas.has_value()) {
        ec = top::evm_runtime::error::xerrc_t::bsc_header_missing_excess_blob_gas;
        return;
    }

    if (!header.blob_gas_used.has_value()) {
        ec = top::evm_runtime::error::xerrc_t::bsc_header_missing_blob_gas_used;
        return;
    }

    if (header.blob_gas_used.value() > BLOB_TX_MAX_BLOB_GAS_PER_BLOCK) {
        ec = top::evm_runtime::error::xerrc_t::bsc_blob_gas_used_exceeds_maximum_allowance;
        return;
    }

    if (header.blob_gas_used.value() % BLOB_TX_BLOB_GAS_PER_BLOB != 0) {
        ec = top::evm_runtime::error::xerrc_t::bsc_blob_gas_used_not_a_multiple_of_blob_gas_per_blob;
        return;
    }

    uint64_t parent_excess_blob_gas = 0;
    uint64_t parent_blob_gas_used = 0;
    if (parent.excess_blob_gas.has_value()) {
        parent_excess_blob_gas = parent.excess_blob_gas.value();
        parent_blob_gas_used = parent.blob_gas_used.value();
    }

    auto const excess_blob_gas = parent_excess_blob_gas + parent_blob_gas_used;
    auto const expected_excess_blob_gas = excess_blob_gas < BLOB_TX_TARGET_BLOB_GAS_PER_BLOCK ? 0u : excess_blob_gas - BLOB_TX_TARGET_BLOB_GAS_PER_BLOCK;
    if (header.excess_blob_gas.value() != expected_excess_blob_gas) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_excess_blob_gas;
    }
}

uint64_t xtop_evm_bsc_client_contract::calc_base_fee(xchain_config_t const &, xeth_header_t const &) {
    return 0;
}

// verifyCascadingFields verifies all the header fields that are not standalone,
// rather depend on a batch of previous headers. The caller may optionally pass
// in a batch of parents (ascending order) to avoid looking those up from the
// database. This is useful for concurrently verifying a batch of new headers.
bool xtop_evm_bsc_client_contract::verify_cascading_fields(xchain_config_t const & chain_config,
                                                           xeth_header_t const & header,
                                                           xspan_t<xeth_header_t const> parents,
                                                           state_ptr state,
                                                           std::error_code & ec) const {
    assert(!ec);

    if (header.number == 0) {
        return true;
    }

    auto const & parent = get_parent(header, parents, state, ec);
    if (ec) {
        xerror("%s: get_parent failed, msg: %s", __func__, ec.message().c_str());
        return false;
    }

    constexpr uint64_t capacity = 0x7fffffffffffffffull;
    if (header.gas_limit > capacity) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_gas_limit;
        xerror("xtop_evm_bsc_client_contract::verify_cascading_fields: invalid gas limit: have %" PRIu64 ", max %" PRIu64, header.gas_limit.convert_to<uint64_t>(), capacity);
        return false;
    }

    if (header.gas_used > header.gas_limit) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_gas_used;
        xwarn("xtop_evm_bsc_client_contract::verify_cascading_fields: invalid gas used: have %" PRIu64 ", gas limit %" PRIu64, header.gas_used.convert_to<uint64_t>(), header.gas_limit.convert_to<uint64_t>());
        return false;
    }

    // Verify that the gas limit remains within allowed bounds
    auto diff = parent.gas_limit - header.gas_limit;
    if (diff < 0) {
        diff *= -1;
    }
    auto const limit = parent.gas_limit / GAS_LIMIT_BOUND_DIVISOR;

    if (diff.convert_to<uint64_t>() >= limit || header.gas_limit < MIN_GAS_LIMIT) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_gas_limit;
        xwarn("xtop_evm_bsc_client_contract::verify_cascading_fields: invalid gas limit: have %" PRIu64 ", want %" PRIu64 " += %" PRIu64 "-1",
              header.gas_limit.convert_to<uint64_t>(),
              parent.gas_limit.convert_to<uint64_t>(),
              limit.convert_to<uint64_t>());
        return false;
    }

    verify_vote_attestation(chain_config, header, parents, state, ec);
    if (ec) {
        xerror("%s: Verify vote attestation failed. ec: %s", __func__, ec.message().c_str());
        return false;
    }

    // return verify_seal();
    return true;
}

void xtop_evm_bsc_client_contract::verify_vote_attestation(xchain_config_t const & chain_config,
                                                           xeth_header_t const & header,
                                                           xspan_t<xeth_header_t const> parents,
                                                           state_ptr state,
                                                           std::error_code & ec) const {
    assert(!ec);
    auto const attestation = get_vote_attestation_from_header(header, chain_config, ec);
    if (ec) {
        xerror("%s: get_vote_attestation_from_header failed, msg: %s", __func__, ec.message().c_str());
        return;
    }

    /*
     * if (attestation.empty()) {
     *      return;
     *  }
     */

    if (!attestation.data.has_value()) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_attestation;
        xerror("%s: invalid attestation, vote data is null", __func__);
        return;
    }

    if (attestation.extra.size() > MAX_ATTESTATION_EXTRA_SIZE) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_attestation;
        xerror("%s: invalid attestation, too large extra length %" PRIu64, __func__, attestation.extra.size());
        return;
    }

    xeth_header_t const & parent = get_parent(header, parents, state, ec);
    if (ec) {
        xerror("%s: get_parent failed, msg: %s", __func__, ec.message().c_str());
        return;
    }

    auto const target_number = attestation.data.value().target_number();
    auto const target_hash = attestation.data.value().target_hash();
    if (target_number != parent.number || target_hash != parent.hash) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_attestation;
        xerror("%s: invalid attestation, target mismatch, expected block: %" PRIu64 ", hash: %s; real block: %" PRIu64 ", hash: %s",
               __func__,
               parent.number,
               parent.hash.hex().c_str(),
               target_number,
               target_hash.hex().c_str());
        return;
    }

    auto const parent_attestation = get_vote_attestation_from_header(parent, chain_config, ec);
    if (ec) {
        xerror("%s: get_vote_attestation_from_header failed on parent header, msg: %s", __func__, ec.message().c_str());
        return;
    }
    auto const source_number = attestation.data.value().source_number();
    auto const source_hash = attestation.data.value().source_hash();
    uint64_t justified_block_number = parent_attestation.data.value().target_number();
    xh256_t justified_block_hash = parent_attestation.data.value().target_hash();

    if (source_number != justified_block_number || source_hash != justified_block_hash) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_attestation;
        xerror("%s: invalid attestation, source mismatch, expected block: %" PRIu64 ", hash: %s; real block: %" PRIu64 ", hash: %s",
               justified_block_number,
               justified_block_hash.hex().c_str(),
               source_number,
               source_hash.hex().c_str());
        return;
    }

    if (!parents.empty()) {
        parents = parents.first(parents.size() - 1);
    }

    xsnapshot_t snapshot;   // = snapshot(number, hash, parents);
    if (ec) {
        return;
    }

    auto const validators = snapshot.validators();
    std::bitset<VALIDATOR_NUM> const validators_bitset{attestation.vote_address_set};
    if (validators_bitset.count() > validators.size()) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_attestation;
        xerror("%s: invalid attestation, vote number larger than validators number", __func__);
        return;
    }


}

void xtop_evm_bsc_client_contract::get_justified_number_and_hash(xeth_header_t const & header,
                                                                 state_ptr const & state,
                                                                 uint64_t & justified_number,
                                                                 xh256_t & justified_hash,
                                                                 std::error_code & ec) const {
    assert(!ec);
    // auto snapshot = snapshot();
    xsnapshot_t snapshot;

    justified_number = snapshot.attestation().target_number();
    justified_hash = snapshot.attestation().target_hash();
}


xvote_attestation_t xtop_evm_bsc_client_contract::get_vote_attestation_from_header(xeth_header_t const & header, xchain_config_t const & chain_config, std::error_code & ec) const {
    assert(!ec);

    xvote_attestation_t attestation;
    if (header.extra.size() <= EXTRA_VANITY + EXTRA_SEAL) {
        return attestation;
    }

    xbytes_t attestation_bytes;
    if (header.number % epoch == 0) {
        auto const begin = std::next(std::begin(header.extra), static_cast<ptrdiff_t>(EXTRA_VANITY));
        auto const end = std::next(std::end(header.extra), -static_cast<ptrdiff_t>(EXTRA_SEAL));

        attestation_bytes = xbytes_t{begin, end};
        return attestation;
    }

    int const num = static_cast<int>(header.extra[EXTRA_VANITY]);
    if (header.extra.size() <= EXTRA_VANITY + EXTRA_SEAL + VALIDATOR_NUMBER_SIZE + num * VALIDATOR_BYTES_LENGTH) {
        return attestation;
    }

    auto const begin = std::next(std::begin(header.extra), static_cast<ptrdiff_t>(EXTRA_VANITY + VALIDATOR_NUMBER_SIZE + num * VALIDATOR_BYTES_LENGTH));
    auto const end = std::next(std::end(header.extra), -static_cast<ptrdiff_t>(EXTRA_SEAL));

    attestation_bytes = xbytes_t{begin, end};

    attestation.decode_rlp(attestation_bytes, ec);
    if (ec) {
        ec = top::evm_runtime::error::xerrc_t::bsc_invalid_extra_data;
        xerror("%s: decode_rlp failed, msg: %s", __func__, ec.message().c_str());
        return attestation;
    }

    return attestation;
}

bool xtop_evm_bsc_client_contract::verify_headers(std::vector<xeth_header_t> const & headers, state_ptr state) const {
    for (size_t i = 0; i < headers.size(); ++i) {
        if (!verify_header(headers[i], xspan_t<xeth_header_t const>{headers.data(), i}, state)) {
            return false;
        }
    }

    return true;
}

bool xtop_evm_bsc_client_contract::verify_header(xeth_header_t const & header, xspan_t<xeth_header_t const> const parents, state_ptr state) const {
    if (header.number == 0) {
        xerror("%s: block number invalid.", __func__);
        return false;
    }

    if (header.extra.size() < EXTRA_VANITY) {
        xerror("%s: header extra missing EXTRA_VANITY.", __func__);
        return false;
    }

    if (header.extra.size() < EXTRA_VANITY + EXTRA_SEAL) {
        xerror("%s: header extra missing signature.", __func__);
        return false;
    }

    // check extra data
    uint64_t const number = header.number;
    bool const is_epoch = number % epoch == 0;
    std::error_code ec;

    // Ensure that the extra-data contains a signer list on checkpoint, but none otherwise
    auto const & signers_bytes = get_validator_bytes_from_header(header, bsc_chain_config, bsc_chain_config.parlia_config);

    if (!is_epoch && !signers_bytes.empty()) {
        xerror("%s: extra validators.", __func__);
        return false;
    }

    if (is_epoch && signers_bytes.empty()) {
        xerror("%s: invalid span validators.", __func__);
        return false;
    }

    // Ensure that the mix digest is zero as we don't have fork protection currently
    if (header.mix_digest != xh256_t{}) {
        xerror("%s: invalid mix digest.", __func__);
        return false;
    }

    // Ensure that the block doesn't contain any uncles which are meaningless in PoA
    if (header.uncle_hash != empty_unclehash) {
        xerror("%s: invalid uncle hash.", __func__);
        return false;
    }

    if (header.number > 0) {
        if (header.difficulty == 0) {
            xerror("%s: invalid difficulty.", __func__);
            return false;
        }
    }

    auto const parent = get_parent(header, parents, state, ec);
    if (ec) {
        xerror("%s: get_parent failed, msg: %s", __func__, ec.message().c_str());
        return false;
    }

    if (!verify_eip1559_header(bsc_chain_config, parent, header)) {
        xerror("%s: verify_eip1559_header failed.", __func__);
        return false;
    }

    if (header.withdrawals_root.has_value()) {
        xerror("%s: invalid withdrawals root: have %s, expected null", __func__, header.withdrawals_root.value().hex().c_str());
        return false;
    }

    // Ethereum cancun fork
    // https://github.com/ethereum/execution-specs/blob/master/network-upgrades/mainnet-upgrades/cancun.md
    // https://forum.bnbchain.org/t/bnb-chain-upgrades-mainnet/936#future-4
    //  Future
    //      StateExpiry
    //      Parallel EVM
    //      Dynamic GasFee
    //      Segmented Archive State Maintenance
    //      Safe Hard Fork With Transition Period ?
    //      Part of Cancun Upgrade:
    // https://forum.bnbchain.org/t/bnb-chain-upgrades-mainnet/936#kepler-ideas-collecting-5

    verify_eip4844_header(parent, header, ec);
    if (ec) {
        return false;
    }

    return verify_cascading_fields(bsc_chain_config, header, parents, state, ec);
}

xeth_header_t xtop_evm_bsc_client_contract::get_parent(xeth_header_t const & header, xspan_t<xeth_header_t const> parents, state_ptr state, std::error_code & ec) const {
    xeth_header_t parent;
    assert(!ec);

    if (!parents.empty()) {
        parent = parents.back();
    } else {
        if (!get_header(header.parent_hash, parent, state)) {
            ec = evm_runtime::error::xerrc_t::bsc_unknown_ancestor;
        }
    }

    return parent;
}

/// <summary>
/// snapshot retrieves the authorization snapshot at a given point in time.
/// !!! be careful
/// the block with `number` and `hash` is just the last element of `parents`,
/// unlike other interfaces such as verifyCascadingFields, `parents` are real parents
///
/// extra notes:
///  1. `number  is the last element of `parents`' number
///  2. `hash` is the last element of `parents`' hash
/// </summary>
/// <param name="number">snapshot at the number</param>
/// <param name="hash">snapshot at the hash</param>
/// <param name="parents">the parent headers when calling this API</param>
/// <param name="state">world state object</param>
/// <param name="ec">error code</param>
/// <returns>true for success, false for the other</returns>
xsnapshot_t xtop_evm_bsc_client_contract::snapshot(uint64_t number, xh256_t const & hash, xspan_t<xeth_header_t const> parents, state_ptr state, std::error_code & ec) const {
    std::vector<xeth_header_t> headers;
    xsnapshot_t snap;

    while (snap.empty()) {
        snap = get_recent_snapshot(hash, state, ec);
        if (!ec) {
            break;
        }


    }

    return snap;
}

xsnapshot_t xtop_evm_bsc_client_contract::get_recent_snapshot(xh256_t const & snapshot_hash, state_ptr & state, std::error_code & ec) const {
    assert(!ec);
    {
        auto const snapshot_it = recent_snapshots_.find(snapshot_hash);
        if (snapshot_it != recent_snapshots_.end()) {
            return top::get<xsnapshot_t>(*snapshot_it);
        }
    }

    xsnapshot_t snapshot;
    std::string snapshot_bytes = state->map_get(data::system_contract::XPROPERTY_RECENT_SNAPSHOTS, {std::begin(snapshot_hash), std::end(snapshot_hash)});
    if (snapshot_bytes.empty()) {
        ec = evm_runtime::error::xerrc_t::bsc_snapshot_not_found;
        return snapshot;
    }

    snapshot.decode({std::begin(snapshot_bytes), std::end(snapshot_bytes)}, ec);
    if (ec) {
        xerror("%s: decode snapshot failed, msg: %s", __func__, ec.message().c_str());
        return snapshot;
    }

    recent_snapshots_.insert({snapshot_hash, snapshot});

    return snapshot;
}

void xtop_evm_bsc_client_contract::add_recent_snapshot(xsnapshot_t const & snapshot, state_ptr & state, std::error_code & ec) {
    assert(!ec);
    auto const & hash = snapshot.hash();

    std::string const key{std::begin(hash), std::end(hash)};

    auto const & bytes = snapshot.encode(ec);
    if (ec) {
        xerror("%s: encode snapshot failed, msg: %s", __func__, ec.message().c_str());
        return;
    }
    std::string const value{std::begin(bytes), std::end(bytes)};

    state->map_set(data::system_contract::XPROPERTY_RECENT_SNAPSHOTS, key, value);

    recent_snapshots_.insert({hash, snapshot});
}

void xtop_evm_bsc_client_contract::del_recent_snapshot(xh256_t const & snapshot_hash, state_ptr & state, std::error_code & ec) {
    assert(!ec);
    std::string const key{std::begin(snapshot_hash), std::end(snapshot_hash)};
    state->map_remove(data::system_contract::XPROPERTY_RECENT_SNAPSHOTS, key);

    {
        auto const snapshot_it = recent_snapshots_.find(snapshot_hash);
        if (snapshot_it != recent_snapshots_.end()) {
            recent_snapshots_.erase(snapshot_it);
        }
    }
}


xsnapshot_t xtop_evm_bsc_client_contract::last_validator_set(state_ptr & state, std::error_code & ec) const {
    xsnapshot_t snapshot;
    return snapshot;
}

void xtop_evm_bsc_client_contract::last_validator_set(top::evm::crosschain::bsc::xsnapshot_t const & snapshot, state_ptr & state, std::error_code & ec) {
    
}

xsnapshot_t xtop_evm_bsc_client_contract::pre_last_validator_set(state_ptr & state, std::error_code & ec) const {
    xsnapshot_t snapshot;
    return snapshot;
}

void xtop_evm_bsc_client_contract::pre_last_validator_set(top::evm::crosschain::bsc::xsnapshot_t const & snapshot, state_ptr & state, std::error_code & ec) {
    assert(!ec);

    auto const & hash = snapshot.hash();
    std::string const key{std::begin(hash), std::end(hash)};

    auto const & bytes = snapshot.encode(ec);
    if (ec) {
        xerror("%s: encode snapshot failed, msg: %s", __func__, ec.message().c_str());
        return;
    }
    std::string const value{std::begin(bytes), std::end(bytes)};

    state->map_set(data::system_contract::XPROPERTY_PRE_LAST_VALIDATOR_SET, key, value);
}

void xtop_evm_bsc_client_contract::verify_seal(xeth_header_t const & header, xspan_t<xeth_header_t const> parents, std::error_code & ec) const {
    assert(!ec);

    //auto const number = header.number;
    //xsnapshot_t snapshot;
}


NS_END4
