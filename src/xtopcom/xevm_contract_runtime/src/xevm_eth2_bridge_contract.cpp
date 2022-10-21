// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/sys_contract/xevm_eth2_bridge_contract.h"

#include "xbasic/endianness.h"
#include "xcommon/xaccount_address.h"
#include "xcommon/xeth_address.h"
#include "xdata/xdata_common.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xabi_decoder.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

#define EPOCHS_PER_SYNC_COMMITTEE_PERIOD 256U
#define SLOTS_PER_EPOCH 32U
#define MIN_SYNC_COMMITTEE_PARTICIPANTS 1U

constexpr uint64_t hashes_gc_threshold = 51000;

static uint64_t compute_epoch_at_slot(uint64_t const slot) {
    return slot / SLOTS_PER_EPOCH;
}

static uint64_t compute_sync_committee_period(uint64_t const slot) {
    return compute_epoch_at_slot(slot) / EPOCHS_PER_SYNC_COMMITTEE_PERIOD;
}

static std::vector<xbytes_t> get_participant_pubkeys(std::vector<xbytes_t> const & public_keys, std::vector<uint8_t> const & sync_committee_bits) {
    std::vector<xbytes_t> ret;
    for (auto i = 0; i < sync_committee_bits.size(); ++i) {
        if (sync_committee_bits[i]) {
            ret.emplace_back(public_keys[i]);
        }
    }
    return ret;
}

bool xtop_evm_eth2_bridge_contract::init(state_ptr const & state, xinit_input_t & const init_input) {
    auto const & hash = init_input.finalized_execution_header.hash();
    if (hash != init_input.finalized_beacon_header.execution_block_hash) {
        xwarn("xtop_evm_eth2_bridge_contract::init hash mismatch %s, %s", hash.hex().c_str(), init_input.finalized_beacon_header.execution_block_hash.hex().c_str());
        return false;
    }
    auto const & finalized_execution_header_info =
        xexecution_header_info_t{init_input.finalized_execution_header.parent_hash, static_cast<uint64_t>(init_input.finalized_execution_header.number)};
    if (false == set_finalized_beacon_header(state, init_input.finalized_beacon_header)) {
        xwarn("xtop_evm_eth2_bridge_contract::init set_finalized_beacon_header error");
        return false;
    }
    if (false == set_finalized_execution_header(state, finalized_execution_header_info)) {
        xwarn("xtop_evm_eth2_bridge_contract::init set_finalized_execution_header error");
        return false;
    }
    if (false == set_current_sync_committee(state, init_input.current_sync_committee)) {
        xwarn("xtop_evm_eth2_bridge_contract::init set_current_sync_committee error");
        return false;
    }
    if (false == set_next_sync_committee(state, init_input.next_sync_committee)) {
        xwarn("xtop_evm_eth2_bridge_contract::init set_next_sync_committee error");
        return false;
    }
    xinfo("xtop_evm_eth2_bridge_contract::init success");
    return true;
}

bool xtop_evm_eth2_bridge_contract::initialized(state_ptr const & state) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    return !finalized_beacon_header.empty();
}

uint64_t xtop_evm_eth2_bridge_contract::last_block_number(state_ptr const & state) {
    return get_finalized_execution_header(state).block_number;
}

h256 xtop_evm_eth2_bridge_contract::block_hash_safe(state_ptr const & state, uint64_t const block_number) {
    return get_finalized_execution_blocks(state, block_number);
}

bool xtop_evm_eth2_bridge_contract::is_known_execution_header(state_ptr const & state, h256 const & hash) {
    auto const & header = get_unfinalized_headers(state, hash);
    if (header.empty()) {
        return false;
    }
    return true;
}

h256 xtop_evm_eth2_bridge_contract::finalized_beacon_block_root(state_ptr const & state) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::finalized_beacon_block_root get_finalized_beacon_header empty");
        return {};
    }
    return finalized_beacon_header.beacon_block_root;
}

uint64_t xtop_evm_eth2_bridge_contract::finalized_beacon_block_slot(state_ptr const & state) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::finalized_beacon_block_slot get_finalized_beacon_header empty");
        return 0;
    }
    return finalized_beacon_header.header.slot;
}

xextended_beacon_block_header_t xtop_evm_eth2_bridge_contract::finalized_beacon_block_header(state_ptr const & state) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::finalized_beacon_block_header get_finalized_beacon_header empty");
        return {};
    }
    return finalized_beacon_header;
}

xlight_client_state_t xtop_evm_eth2_bridge_contract::get_light_client_state(state_ptr const & state) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::get_light_client_state get_finalized_beacon_header empty");
        return {};
    }
    auto const & current_sync_committee = get_current_sync_committee(state);
    if (current_sync_committee.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::get_light_client_state current_sync_committee empty");
        return {};
    }
    auto const & next_sync_committee = get_next_sync_committee(state);
    if (next_sync_committee.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::get_light_client_state next_sync_committee empty");
        return {};
    }
    return {finalized_beacon_header, current_sync_committee, next_sync_committee};
}

bool xtop_evm_eth2_bridge_contract::submit_beacon_chain_light_client_update(state_ptr const & state, xlight_client_update_t const & update) {
    if (false == validate_light_client_update(state, update)) {
        xwarn("xtop_evm_eth2_bridge_contract::submit_beacon_chain_light_client_update validate_light_client_update error");
        return false;
    }
    if (false == commit_light_client_update(state, update)) {
        xwarn("xtop_evm_eth2_bridge_contract::submit_beacon_chain_light_client_update commit_light_client_update error");
        return false;
    }
    xinfo("xtop_evm_eth2_bridge_contract::submit_beacon_chain_light_client_update success");
    return true;
}

bool xtop_evm_eth2_bridge_contract::submit_execution_header(state_ptr const & state, evm_common::xeth_header_t const & block_header) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::verify_finality_branch get_finalized_beacon_header empty");
        return false;
    }
    if (finalized_beacon_header.execution_block_hash != block_header.parent_hash) {
        auto unfinalized_header = get_unfinalized_headers(state, block_header.parent_hash);
        if (unfinalized_header.empty()) {
            xwarn("xtop_evm_eth2_bridge_contract::submit_execution_header parent %s not submitted", block_header.parent_hash.hex().c_str());
            return false;
        }
    }
    auto const & block_hash = block_header.hash();
    auto const & block_info = xexecution_header_info_t{block_header.parent_hash, static_cast<uint64_t>(block_header.number)};
    if (false == set_unfinalized_headers(state, block_hash, block_info)) {
        xwarn("xtop_evm_eth2_bridge_contract::submit_execution_header set_unfinalized_headers error: %s", block_hash.hex().c_str());
        return false;
    }
    xinfo("xtop_evm_eth2_bridge_contract::submit_execution_header header %s set succss", block_hash.hex().c_str());
    return true;
}

bool xtop_evm_eth2_bridge_contract::validate_light_client_update(state_ptr const & state, xlight_client_update_t const & update) {
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::verify_finality_branch get_finalized_beacon_header empty");
        return false;
    }
    auto finalized_period = compute_sync_committee_period(finalized_beacon_header.header.slot);
    if (false == verify_finality_branch(state, update, finalized_period)) {
        xwarn("xtop_evm_eth2_bridge_contract::validate_light_client_update verify_finality_branch error");
        return false;
    }
    auto const & sync_committee_bits = update.sync_aggregate.sync_committee_bits;
    uint64_t sync_committee_bits_sum{0};
    for (auto const & b : sync_committee_bits) {
        sync_committee_bits_sum += (b > 0 ? 1 : 0);
    }
    if (sync_committee_bits_sum < MIN_SYNC_COMMITTEE_PARTICIPANTS) {
        xwarn("xtop_evm_eth2_bridge_contract::validate_light_client_update error sync_committee_bits_sum: %lu", sync_committee_bits_sum);
        return false;
    }
    if (sync_committee_bits_sum * 3 >= (sync_committee_bits.size() * 2)) {
        xwarn("xtop_evm_eth2_bridge_contract::validate_light_client_update committee bits sum is less than 2/3 threshold: %lu, %lu", sync_committee_bits_sum, sync_committee_bits);
        return false;
    }
    if (false == verify_bls_signatures(state, update, sync_committee_bits, finalized_period)) {
        xwarn("xtop_evm_eth2_bridge_contract::validate_light_client_update verify_bls_signatures error");
        return false;
    }
    xinfo("xtop_evm_eth2_bridge_contract::validate_light_client_update validate update success");
    return true;
}

bool xtop_evm_eth2_bridge_contract::verify_finality_branch(state_ptr const & state, xlight_client_update_t const & update, uint64_t const finalized_period) {
    auto const & active_header = update.finality_update.header_update.beacon_header;
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::verify_finality_branch get_finalized_beacon_header empty");
        return false;
    }
    if (active_header.slot <= finalized_beacon_header.header.slot) {
        xwarn("xtop_evm_eth2_bridge_contract::verify_finality_branch slot mismatch, %lu, %lu", active_header.slot, finalized_beacon_header.header.slot);
        return;
    }
    if (update.attested_beacon_header.slot < update.finality_update.header_update.beacon_header.slot) {
        xwarn("xtop_evm_eth2_bridge_contract::verify_finality_branch slot mismatch, %lu, %lu", active_header.slot, finalized_beacon_header.header.slot);
        return;
    }
    if (update.signature_slot <= update.attested_beacon_header.slot) {
        xwarn("xtop_evm_eth2_bridge_contract::verify_finality_branch slot mismatch, %lu, %lu", update.signature_slot, update.attested_beacon_header.slot);
        return;
    }
    auto update_period = compute_sync_committee_period(active_header.slot);
    if (update_period != finalized_period && update_period != finalized_period + 1) {
        xwarn("xtop_evm_eth2_bridge_contract::verify_finality_branch period mismatch, %lu, %lu", update_period, finalized_period);
        return;
    }
    auto const & branch = update.finality_update.finality_branch;
    // TODO: merkle_proof::verify_merkle_proof
    return true;
}

bool xtop_evm_eth2_bridge_contract::verify_bls_signatures(state_ptr const & state, xlight_client_update_t const & update, std::vector<uint8_t> const & sync_committee_bits, uint64_t const finalized_period) {
    auto signature_period = compute_sync_committee_period(update.signature_slot);
    if (signature_period != finalized_period && signature_period != finalized_period + 1) {
        xwarn("xtop_evm_eth2_bridge_contract::verify_bls_signatures signature_period mismatch: %lu, %lu", signature_period, finalized_period);
        return false;
    }
    xsync_committee_t sync_committee;
    if (signature_period == finalized_period) {
        sync_committee = get_current_sync_committee(state);
        if (sync_committee.empty()) {
            xwarn("xtop_evm_eth2_bridge_contract::verify_bls_signatures get_current_sync_committee empty");
            return false;
        }
    } else {
        sync_committee = get_next_sync_committee(state);
        if (sync_committee.empty()) {
            xwarn("xtop_evm_eth2_bridge_contract::verify_bls_signatures get_next_sync_committee empty");
            return false;
        }
    }
    auto const & participant_pubkeys = get_participant_pubkeys(sync_committee.pubkeys, sync_committee_bits);
    // TODO: tree_hash_derive::TreeHash, bls::AggregateSignature, bls::PublicKey
    return true;
}

bool xtop_evm_eth2_bridge_contract::update_finalized_header(state_ptr const & state, xextended_beacon_block_header_t const & finalized_header) {
    //  finalized_execution_header_info
    auto const & finalized_execution_header_info = get_unfinalized_headers(state, finalized_header.execution_block_hash);
    if (finalized_execution_header_info.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::update_finalized_header get_unfinalized_headers %s empty", finalized_header.execution_block_hash.hex().c_str());
        return;
    }
    auto cursor_header = finalized_execution_header_info;
    auto cursor_header_hash = finalized_header.execution_block_hash;

    for (;;) {
        del_unfinalized_headers(state, cursor_header_hash);
        if (false == set_finalized_execution_blocks(state, cursor_header.block_number, cursor_header_hash)) {
            xwarn("xtop_evm_eth2_bridge_contract::update_finalized_header set_finalized_execution_blocks error, %lu, %s",
                  cursor_header.block_number,
                  cursor_header_hash.hex().c_str());
            return false;
        }
        auto const & finalized_beacon_header = get_finalized_beacon_header(state);
        if (finalized_beacon_header.empty()) {
            xwarn("xtop_evm_eth2_bridge_contract::update_finalized_header get_finalized_beacon_header empty");
            return false;
        }
        if (cursor_header.parent_hash == finalized_beacon_header.execution_block_hash) {
            break;
        }
        cursor_header_hash = cursor_header.parent_hash;
        cursor_header = get_unfinalized_headers(state, cursor_header_hash);
        if (cursor_header.empty()) {
            xwarn("xtop_evm_eth2_bridge_contract::update_finalized_header get_unfinalized_headers %s empty", cursor_header_hash.hex().c_str());
            return false;
        }
    }
    if (false == set_finalized_beacon_header(state, finalized_header)) {
        xwarn("xtop_evm_eth2_bridge_contract::update_finalized_header set_finalized_beacon_header %s error", finalized_header.beacon_block_root.hex().c_str());
        return false;
    }
    if (false == set_finalized_execution_header(state, finalized_execution_header_info)) {
        xwarn("xtop_evm_eth2_bridge_contract::update_finalized_header set_finalized_execution_header %lu error", finalized_execution_header_info.block_number);
        return false;
    }
    if (finalized_execution_header_info.block_number > hashes_gc_threshold) {
        release_finalized_execution_blocks(state, finalized_execution_header_info.block_number - hashes_gc_threshold);
    }
    xinfo("xtop_evm_eth2_bridge_contract::update_finalized_header %s success", finalized_header.beacon_block_root.hex().c_str());
    return true;
}

bool xtop_evm_eth2_bridge_contract::commit_light_client_update(state_ptr const & state, xlight_client_update_t const & update) {
    auto const & finalized_header_update = update.finality_update.header_update;
    auto const & finalized_beacon_header = get_finalized_beacon_header(state);
    if (finalized_beacon_header.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::commit_light_client_update get_finalized_beacon_header empty");
        return false;
    }
    auto const & finalized_period = compute_sync_committee_period(finalized_beacon_header.header.slot);
    auto const & update_period = compute_sync_committee_period(finalized_header_update.beacon_header.slot);
    if (update_period == finalized_period + 1) {
        auto const & next_sync_committee = get_next_sync_committee(state);
        if (next_sync_committee.empty()) {
            xwarn("xtop_evm_eth2_bridge_contract::commit_light_client_update get_next_sync_committee error");
            return false;
        }
        if (false == set_current_sync_committee(state, next_sync_committee)) {
            xwarn("xtop_evm_eth2_bridge_contract::commit_light_client_update set_current_sync_committee error");
            return false;
        }
        if (false == set_next_sync_committee(state, update.sync_committee_update.next_sync_committee)) {
            xwarn("xtop_evm_eth2_bridge_contract::commit_light_client_update set_next_sync_committee error");
            return false;
        }
    }
    // TODO: tree_hash_derive::TreeHash
    // if (false == update_finalized_header(state, finalized_header_update)) {
    //     xwarn("xtop_evm_eth2_bridge_contract::commit_light_client_update update_finalized_header error");
    //     return false;
    // }
    return true;
}

void xtop_evm_eth2_bridge_contract::release_finalized_execution_blocks(state_ptr const & state, uint64_t number) {
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

h256 xtop_evm_eth2_bridge_contract::get_finalized_execution_blocks(state_ptr const & state, uint64_t const height) {
    auto str = state->map_get(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, std::to_string(height));
    if (str.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::get_finalized_execution_blocks map_get empty, height: %lu", height);
        return {};
    }
    return static_cast<h256>(xbytes_t{std::begin(str), std::end(str)});
}

bool xtop_evm_eth2_bridge_contract::set_finalized_execution_blocks(state_ptr const & state, uint64_t const height, h256 const & hash) {
    auto v = hash.asBytes();
    if (0 != state->map_set(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, std::to_string(height), {v.begin(), v.end()})) {
        xwarn("xtop_evm_eth2_bridge_contract::set_finalized_execution_blocks map_set error, height: %lu", height);
        return false;
    }
    return true;
}

bool xtop_evm_eth2_bridge_contract::del_finalized_execution_blocks(state_ptr const & state, uint64_t const height) {
    auto k = evm_common::toBigEndian(static_cast<u256>(height));
    if (0 != state->map_remove(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

xexecution_header_info_t xtop_evm_eth2_bridge_contract::get_unfinalized_headers(state_ptr const & state, h256 const & hash) {
    auto k = hash.asBytes();
    auto str = state->map_get(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS, {k.begin(), k.end()});
    if (str.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::get_unfinalized_headers map_get empty, hash: %s", hash.hex().c_str());
        return {};
    }
    xexecution_header_info_t info;
    if (info.decode_rlp({std::begin(str), std::end(str)}) == false) {
        xwarn("xtop_evm_eth2_bridge_contract::get_unfinalized_headers decode error, hash: %s", hash.hex().c_str());
        return {};
    }
    return info;
}

bool xtop_evm_eth2_bridge_contract::set_unfinalized_headers(state_ptr const & state, h256 const & hash, xexecution_header_info_t const & info) {
    auto k = hash.asBytes();
    auto v = info.encode_rlp();
    if (0 != state->map_set(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS, {k.begin(), k.end()}, {v.begin(), v.end()})) {
        xwarn("xtop_evm_eth2_bridge_contract::set_unfinalized_headers map_set error, hash: %s", hash.hex().c_str());
        return false;
    }
    return true;
}

bool xtop_evm_eth2_bridge_contract::del_unfinalized_headers(state_ptr const & state, h256 const & hash) {
    auto k = hash.to_bytes();
    if (0 != state->map_remove(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS, {k.begin(), k.end()})) {
        return false;
    }
    return true;
}

xextended_beacon_block_header_t xtop_evm_eth2_bridge_contract::get_finalized_beacon_header(state_ptr const & state) {
    auto const & v = state->string_get(data::system_contract::XPROPERTY_FINALIZED_BEACON_HEADER);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::get_finalized_beacon_header empty");
        return {};
    }
    xextended_beacon_block_header_t beacon;
    if (beacon.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_bridge_contract::get_finalized_beacon_header decode error");
    }
    return beacon;
}

bool xtop_evm_eth2_bridge_contract::set_finalized_beacon_header(state_ptr const & state, xextended_beacon_block_header_t const & beacon) {
    auto const & v = beacon.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_FINALIZED_BEACON_HEADER, {v.begin(), v.end()}) != 0) {
        xwarn("xtop_evm_eth2_bridge_contract::set_finalized_beacon_header string_set error");
        return false;
    }
    return true;
}

xexecution_header_info_t xtop_evm_eth2_bridge_contract::get_finalized_execution_header(state_ptr const & state) {
    auto const & v = state->string_get(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_HEADER);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::get_finalized_execution_header empty");
        return {};
    }
    xexecution_header_info_t info;
    if (info.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_bridge_contract::get_finalized_execution_header decode error");
    }
    return info;
}

bool xtop_evm_eth2_bridge_contract::set_finalized_execution_header(state_ptr const & state, xexecution_header_info_t const & info) {
    auto const & v = info.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_HEADER, {v.begin(), v.end()}) != 0) {
        xwarn("xtop_evm_eth2_bridge_contract::set_finalized_execution_header string_set error");
        return false;
    }
    return true;
}

xsync_committee_t xtop_evm_eth2_bridge_contract::get_current_sync_committee(state_ptr const & state) {
    auto const & v = state->string_get(data::system_contract::XPROPERTY_CURRENT_SYNC_COMMITTEE);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::get_current_sync_committee empty");
        return {};
    }
    xsync_committee_t committee;
    if (committee.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_bridge_contract::get_current_sync_committee decode error");
    }
    return committee;
}

bool xtop_evm_eth2_bridge_contract::set_current_sync_committee(state_ptr const & state, xsync_committee_t const & committee) {
    auto const & v = committee.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_CURRENT_SYNC_COMMITTEE, {v.begin(), v.end()}) != 0) {
        xwarn("xtop_evm_eth2_bridge_contract::set_current_sync_committee string_set error");
        return false;
    }
    return true;
}

xsync_committee_t xtop_evm_eth2_bridge_contract::get_next_sync_committee(state_ptr const & state) {
    auto const & v = state->string_get(data::system_contract::XPROPERTY_NEXT_SYNC_COMMITTEE);
    if (v.empty()) {
        xwarn("xtop_evm_eth2_bridge_contract::get_next_sync_committee empty");
        return {};
    }
    xsync_committee_t committee;
    if (committee.decode_rlp({v.begin(), v.end()}) == false) {
        xwarn("xtop_evm_eth2_bridge_contract::get_next_sync_committee decode error");
    }
    return committee;
}

bool xtop_evm_eth2_bridge_contract::set_next_sync_committee(state_ptr const & state, xsync_committee_t const & committee) {
    auto const & v = committee.encode_rlp();
    if (state->string_set(data::system_contract::XPROPERTY_NEXT_SYNC_COMMITTEE, {v.begin(), v.end()}) != 0) {
        xwarn("xtop_evm_eth2_bridge_contract::set_next_sync_committee string_set error");
        return false;
    }
    return true;
}

NS_END4
