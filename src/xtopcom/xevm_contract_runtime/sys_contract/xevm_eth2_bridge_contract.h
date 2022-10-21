// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xnative_contract_address.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_crosschain_contract_face.h"
#include "xvledger/xmerkle.hpp"

#define PUBLIC_KEY_BYTES_LEN 48U

NS_BEG4(top, contract_runtime, evm, sys_contract)

struct xbeacon_block_header_t {
    uint64_t slot{0};
    uint64_t proposer_index{0};
    h256 parent_root;
    h256 state_root;
    h256 body_root;

    bool empty() const {
        return (parent_root == h256() && state_root == h256() && body_root == h256());
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto item1 = evm_common::RLP::encode(slot);
        out.insert(out.end(), item1.begin(), item1.end());
        auto item2 = evm_common::RLP::encode(proposer_index);
        out.insert(out.end(), item2.begin(), item2.end());
        auto item3 = evm_common::RLP::encode(proposer_index);
        out.insert(out.end(), item3.begin(), item3.end());
        auto item4 = evm_common::RLP::encode(proposer_index);
        out.insert(out.end(), item4.begin(), item4.end());
        auto item5 = evm_common::RLP::encode(proposer_index);
        out.insert(out.end(), item5.begin(), item5.end());
        return evm_common::RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto items = evm_common::RLP::decodeList(bytes);
        if (items.decoded.size() != 5) {
            return false;
        }
        slot = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(items.decoded.at(0)));
        proposer_index = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(items.decoded.at(1)));
        parent_root = static_cast<h256>(items.decoded.at(2));
        state_root = static_cast<h256>(items.decoded.at(3));
        body_root = static_cast<h256>(items.decoded.at(4));
        return true;
    }
};

struct xextended_beacon_block_header_t {
    xbeacon_block_header_t header;
    h256 beacon_block_root;
    h256 execution_block_hash;

    bool empty() const {
        return header.empty();
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto item1 = evm_common::RLP::encode(header.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto item2 = evm_common::RLP::encode(beacon_block_root);
        out.insert(out.end(), item2.begin(), item2.end());
        auto item3 = evm_common::RLP::encode(execution_block_hash);
        out.insert(out.end(), item3.begin(), item3.end());
        return evm_common::RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto items = evm_common::RLP::decodeList(bytes);
        if (items.decoded.size() != 3) {
            return false;
        }
        if (header.decode_rlp(items.decoded.at(0)) == false) {
            return false;
        }
        beacon_block_root = static_cast<h256>(items.decoded.at(1));
        execution_block_hash = static_cast<h256>(items.decoded.at(2));
        return true;
    }
};

struct xexecution_header_info_t {
    h256 parent_hash;
    uint64_t block_number{0};

    xexecution_header_info_t() = default;
    xexecution_header_info_t(h256 const & parent_hash_, uint64_t const block_number_) : parent_hash(parent_hash_), block_number(block_number_) {
    }

    bool empty() const {
        return (parent_hash == h256() && block_number == 0);
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto item1 = evm_common::RLP::encode(parent_hash);
        out.insert(out.end(), item1.begin(), item1.end());
        auto item2 = evm_common::RLP::encode(block_number);
        out.insert(out.end(), item2.begin(), item2.end());
        return evm_common::RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto items = evm_common::RLP::decodeList(bytes);
        if (items.decoded.size() != 2) {
            return false;
        }
        parent_hash = static_cast<h256>(items.decoded.at(0));
        block_number = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(items.decoded.at(1)));
        return true;
    }
};

struct xsync_committee_t {
    std::vector<xbytes_t> pubkeys;
    xbytes_t aggregate_pubkey;

    bool empty() const {
        return (pubkeys.empty() && aggregate_pubkey.empty());
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto item1 = evm_common::RLP::encodeList(pubkeys);
        out.insert(out.end(), item1.begin(), item1.end());
        auto item2 = evm_common::RLP::encode(aggregate_pubkey);
        out.insert(out.end(), item2.begin(), item2.end());
        return evm_common::RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto items = evm_common::RLP::decodeList(bytes);
        if (items.decoded.size() != 2) {
            return false;
        }
        auto const & pubkeys_items = evm_common::RLP::decodeList(items.decoded.at(0));
        for (auto const & key : pubkeys_items.decoded) {
            if (key.size() != PUBLIC_KEY_BYTES_LEN) {
                return false;
            }
        }
        if (items.decoded.at(1).size() != PUBLIC_KEY_BYTES_LEN) {
            return false;
        }
        pubkeys = std::move(pubkeys_items.decoded);
        aggregate_pubkey = std::move(items.decoded.at(1));
        return true;
    }
};

struct xsync_aggregate_t {
    std::vector<uint8_t> sync_committee_bits;
    xbytes_t sync_committee_signature;
};

struct xsync_committee_update_t {
    xsync_committee_t next_sync_committee;
    std::vector<h256> next_sync_committee_branch;
};

struct xheader_update_t {
    xbeacon_block_header_t beacon_header;
    h256 execution_block_hash;
    std::vector<h256> execution_hash_branch;
};

struct xfinalized_header_update_t {
    xheader_update_t header_update;
    std::vector<h256> finality_branch;
};

struct xlight_client_update_t {
    xbeacon_block_header_t attested_beacon_header;
    xsync_aggregate_t sync_aggregate;
    uint64_t signature_slot;
    xfinalized_header_update_t finality_update;
    xsync_committee_update_t sync_committee_update;
};

struct xlight_client_state_t {
    xextended_beacon_block_header_t finalized_beacon_header;
    xsync_committee_t current_sync_committee;
    xsync_committee_t next_sync_committee;

    xlight_client_state_t() = default;
    xlight_client_state_t(xextended_beacon_block_header_t const & header, xsync_committee_t const & cur_committee, xsync_committee_t const & next_committee)
      : finalized_beacon_header(header), current_sync_committee(cur_committee), next_sync_committee(next_committee) {
    }
};

struct xinit_input_t {
    evm_common::xeth_header_t finalized_execution_header;
    xextended_beacon_block_header_t finalized_beacon_header;
    xsync_committee_t current_sync_committee;
    xsync_committee_t next_sync_committee;
};

class xtop_evm_eth2_bridge_contract : public xtop_evm_syscontract_face {
public:
    xtop_evm_eth2_bridge_contract() = default;
    ~xtop_evm_eth2_bridge_contract() override = default;

    bool execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 observer_ptr<statectx::xstatectx_face_t> state_ctx,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) override {
    }

    bool init(state_ptr const & state, xinit_input_t & const init_input);
    bool initialized(state_ptr const & state);
    uint64_t last_block_number(state_ptr const & state);
    h256 block_hash_safe(state_ptr const & state, uint64_t const block_number);
    bool is_known_execution_header(state_ptr const & state, h256 const & hash);
    h256 finalized_beacon_block_root(state_ptr const & state);
    uint64_t finalized_beacon_block_slot(state_ptr const & state);
    xextended_beacon_block_header_t finalized_beacon_block_header(state_ptr const & state);
    xlight_client_state_t get_light_client_state(state_ptr const & state);
    bool submit_beacon_chain_light_client_update(state_ptr const & state, xlight_client_update_t const & update);
    bool xtop_evm_eth2_bridge_contract::submit_execution_header(state_ptr const & state, evm_common::xeth_header_t const & block_header);

private:
    // impl
    bool validate_light_client_update(state_ptr const & state, xlight_client_update_t const & update);
    bool verify_finality_branch(state_ptr const & state, xlight_client_update_t const & update, uint64_t const finalized_period) ;
    bool verify_bls_signatures(state_ptr const & state, xlight_client_update_t const & update, std::vector<uint8_t> const & sync_committee_bits, uint64_t const finalized_period);
    bool update_finalized_header(state_ptr const & state, xextended_beacon_block_header_t const & finalized_header);
    bool commit_light_client_update(state_ptr const & state, xlight_client_update_t const & update);
    void release_finalized_execution_blocks(state_ptr const & state, uint64_t number);

    // properties
    h256 get_finalized_execution_blocks(state_ptr const & state, uint64_t const height);
    bool set_finalized_execution_blocks(state_ptr const & state, uint64_t const height, h256 const & hash);
    bool del_finalized_execution_blocks(state_ptr const & state, uint64_t const height);
    xexecution_header_info_t get_unfinalized_headers(state_ptr const & state, h256 const & hash);
    bool set_unfinalized_headers(state_ptr const & state, h256 const & hash, xexecution_header_info_t const & info);
    bool del_unfinalized_headers(state_ptr const & state, h256 const & hash);
    xextended_beacon_block_header_t get_finalized_beacon_header(state_ptr const & state);
    bool set_finalized_beacon_header(state_ptr const & state, xextended_beacon_block_header_t const & beacon);
    xexecution_header_info_t get_finalized_execution_header(state_ptr const & state);
    bool set_finalized_execution_header(state_ptr const & state, xexecution_header_info_t const & info);
    xsync_committee_t get_current_sync_committee(state_ptr const & state);
    bool set_current_sync_committee(state_ptr const & state, xsync_committee_t const & committee);
    xsync_committee_t get_next_sync_committee(state_ptr const & state);
    bool set_next_sync_committee(state_ptr const & state, xsync_committee_t const & committee);
};
using xevm_eth2_bridge_contract_t = xtop_evm_eth2_bridge_contract;

NS_END4
