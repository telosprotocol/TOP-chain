// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbitset.h"
#include "xbasic/xfixed_bytes.h"
#include "xbasic/xfixed_hash.h"
#include "xbasic/xoptional.hpp"
#include "xcommon/common.h"
#include "xcommon/rlp.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_common/xerror/xerror.h"
#include "xevm_runner/evm_engine_interface.h"

#include <array>
#include <cassert>
#include <initializer_list>

NS_BEG3(top, evm_common, eth2)

XINLINE_CONSTEXPR size_t PUBLIC_KEY_BYTES_LEN{48};
XINLINE_CONSTEXPR size_t SIGNATURE_BYTES_LEN{96};
XINLINE_CONSTEXPR size_t SYNC_COMMITTEE_BITS_SIZE{512};

XINLINE_CONSTEXPR uint64_t SLOTS_PER_EPOCH{32};
XINLINE_CONSTEXPR uint64_t EPOCHS_PER_SYNC_COMMITTEE_PERIOD{256};

using xslot_t = uint64_t;
using xepoch_t = uint64_t;
using xperiod_t = uint64_t;

using xbytes48_t = xfixed_bytes_t<PUBLIC_KEY_BYTES_LEN>;
using xbytes96_t = xfixed_bytes_t<SIGNATURE_BYTES_LEN>;

constexpr auto compute_epoch_at_slot(xslot_t const slot)->xepoch_t {
    return slot / SLOTS_PER_EPOCH;
}

constexpr auto compute_sync_committee_period(xslot_t const slot) ->xperiod_t  {
    return compute_epoch_at_slot(slot) / EPOCHS_PER_SYNC_COMMITTEE_PERIOD;
}

enum class xtop_network_id : uint8_t { mainnet, kiln, goerli, sepolia };
using xnetwork_id_t = xtop_network_id;

struct xtop_proof_size {
    size_t beacon_block_body_tree_depth{};
    size_t l1_beacon_block_body_tree_execution_payload_index{};
    size_t l2_execution_payload_tree_execution_block_index{};
    size_t l1_beacon_block_body_proof_size{};
    size_t l2_execution_payload_proof_size{};
    size_t execution_proof_size{};

    xtop_proof_size() = default;
    xtop_proof_size(size_t const beacon_block_body_tree_depth,
                    size_t const l1_beacon_block_body_tree_execution_payload_index,
                    size_t const l2_execution_payload_tree_execution_block_index,
                    size_t const l1_beacon_block_body_proof_size,
                    size_t const l2_execution_payload_proof_size,
                    size_t const execution_proof_size)
      : beacon_block_body_tree_depth{beacon_block_body_tree_depth}
      , l1_beacon_block_body_tree_execution_payload_index{l1_beacon_block_body_tree_execution_payload_index}
      , l2_execution_payload_tree_execution_block_index{l2_execution_payload_tree_execution_block_index}
      , l1_beacon_block_body_proof_size{l1_beacon_block_body_proof_size}
      , l2_execution_payload_proof_size{l2_execution_payload_proof_size}
      , execution_proof_size{execution_proof_size} {
    }
};
using xproof_size_t = xtop_proof_size;

struct xtop_fork_version {
    std::array<uint8_t, 4> fork_version{{0, 0, 0, 0}};

    xtop_fork_version() = default;
    xtop_fork_version(std::initializer_list<uint8_t> il) {
        assert(il.size() == 4);
        std::copy(il.begin(), il.end(), fork_version.begin());
    }
};
using xfork_version_t  = xtop_fork_version;

using xclient_mode_t = enum : uint8_t {
    invalid = 0,
    submit_light_client_update,
    submit_header,
};

struct xbeacon_block_header_t {
    uint64_t slot{0};
    uint64_t proposer_index{0};
    xh256_t parent_root{};
    xh256_t state_root{};
    xh256_t body_root{};

#if defined(XCXX20)
    bool operator==(xbeacon_block_header_t const & rhs) const = default;
#else
    bool operator==(xbeacon_block_header_t const & rhs) const {
        return (this->slot == rhs.slot) && (this->proposer_index == rhs.proposer_index) && (this->parent_root == rhs.parent_root) && (this->state_root == rhs.state_root) &&
               (this->body_root == rhs.body_root);
    }
#endif

    bool empty() const {
        return parent_root.empty() && state_root.empty() && body_root.empty();
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto const & item1 = RLP::encode(slot);
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encode(proposer_index);
        out.insert(out.end(), item2.begin(), item2.end());
        auto const & item3 = RLP::encode(parent_root.asArray());
        out.insert(out.end(), item3.begin(), item3.end());
        auto const & item4 = RLP::encode(state_root.asArray());
        out.insert(out.end(), item4.begin(), item4.end());
        auto const & item5 = RLP::encode(body_root.asArray());
        out.insert(out.end(), item5.begin(), item5.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (items.decoded.size() != 5) {
            xwarn("xbeacon_block_header_t::decode_rlp, items.decoded.size() != 5");
            ec = error::xerrc_t::rlp_list_size_not_match;
            return;
        }

        slot = fromBigEndian<uint64_t>(items.decoded.at(0));
        proposer_index = fromBigEndian<uint64_t>(items.decoded.at(1));
        parent_root = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(2)}};
        state_root = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(3)}};
        body_root = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(4)}};
    }

    xh256_t tree_hash_root() const {
        auto const & rlp = encode_rlp();
        xh256_t h256;
        unsafe_beacon_header_root(rlp.data(), rlp.size(), h256.data());
        return h256;
    }
};

struct xextended_beacon_block_header_t {
    xbeacon_block_header_t header{};
    xh256_t beacon_block_root{};
    xh256_t execution_block_hash{};

    bool operator==(xextended_beacon_block_header_t const & rhs) const {
        return (this->header == rhs.header) && (this->beacon_block_root == rhs.beacon_block_root) && (this->execution_block_hash == rhs.execution_block_hash);
    }

    bool empty() const {
        return header.empty();
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto item1 = RLP::encode(header.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto item2 = RLP::encode(beacon_block_root.asBytes());
        out.insert(out.end(), item2.begin(), item2.end());
        auto item3 = RLP::encode(execution_block_hash.asBytes());
        out.insert(out.end(), item3.begin(), item3.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xextended_beacon_block_header_t::decode_rlp, decode_list failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() != 3) {
            xwarn("xextended_beacon_block_header_t::decode_rlp, items.decoded.size() != 3, actual size %zu", items.decoded.size());
            ec = error::xerrc_t::rlp_list_size_not_match;
            return;
        }

        header.decode_rlp(items.decoded.at(0), ec);
        if (ec) {
            xwarn("xextended_beacon_block_header_t::decode_rlp, xbeacon_block_header_t::decode_rlp failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        beacon_block_root = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(1)}};
        execution_block_hash = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(2)}};
    }
};

struct xexecution_header_info_t {
    xh256_t parent_hash{};
    uint64_t block_number{0};
    common::xeth_address_t submitter{};

    xexecution_header_info_t() = default;
    xexecution_header_info_t(xexecution_header_info_t const &) = default;
    xexecution_header_info_t & operator=(xexecution_header_info_t const &) = default;
    xexecution_header_info_t(xexecution_header_info_t &&) = default;
    xexecution_header_info_t & operator=(xexecution_header_info_t &&) = default;
    ~xexecution_header_info_t() = default;

    xexecution_header_info_t(xh256_t const & parent_hash, uint64_t const block_number, common::xeth_address_t const & submitter)
      : parent_hash{parent_hash}, block_number{block_number}, submitter{submitter} {
        assert(!submitter.is_zero());
    }

    bool operator==(xexecution_header_info_t const & rhs) const {
        return parent_hash == rhs.parent_hash && block_number == rhs.block_number && submitter == rhs.submitter;
    }

    bool empty() const {
        return parent_hash.empty() || block_number == 0;
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto item1 = RLP::encode(parent_hash.asArray());
        out.insert(out.end(), item1.begin(), item1.end());
        auto item2 = RLP::encode(block_number);
        out.insert(out.end(), item2.begin(), item2.end());
        auto item3 = RLP::encode(submitter);
        out.insert(out.end(), item3.begin(), item3.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xexecution_header_info_t::decode_rlp,  RLP::decode_list failed: %s", ec.message().c_str());
            return;
        }

        if (items.decoded.size() != 3) {
            xwarn("xexecution_header_info_t::decode_rlp, items.decoded.size() != 3, actual size: %zu", items.decoded.size());
            ec = common::error::xerrc_t::rlp_invalid_encoded_data;
            return;
        }

        parent_hash = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(0)}};
        block_number = fromBigEndian<uint64_t>(items.decoded.at(1));
        submitter = common::xeth_address_t::build_from(xspan_t<xbyte_t const>{items.decoded.at(2)});
    }
};

class xsync_committee_t {
private:
    std::vector<xbytes48_t> pubkeys_;
    xbytes48_t aggregate_pubkey_;

public:
    xsync_committee_t() = default;
    xsync_committee_t(xsync_committee_t const &) = default;
    xsync_committee_t & operator=(xsync_committee_t const &) = default;
    xsync_committee_t(xsync_committee_t &&) = default;
    xsync_committee_t & operator=(xsync_committee_t &&) = default;
    ~xsync_committee_t() = default;

    bool operator==(xsync_committee_t const & rhs) const {
        return (this->pubkeys_ == rhs.pubkeys_) && (this->aggregate_pubkey_ == rhs.aggregate_pubkey_);
    }

    bool empty() const {
        return (pubkeys_.empty() && aggregate_pubkey_.empty());
    }

    std::vector<xbytes48_t> const & pubkeys() const noexcept {
        return pubkeys_;
    }

    void add_pubkey(xbytes48_t const & pubkey) {
        pubkeys_.push_back(pubkey);
    }

    xbytes48_t const & aggregate_pubkey() const noexcept {
        return aggregate_pubkey_;
    }

    xbytes48_t & aggregate_pubkey() noexcept {
        return aggregate_pubkey_;
    }

    void aggregate_pubkey(xbytes48_t const & pubkey) {
        aggregate_pubkey_ = pubkey;
    }

    xbytes_t encode_rlp() const {
        assert(!pubkeys_.empty());
        assert(!aggregate_pubkey_.empty());
#if !defined(NDEBUG)
        for (auto const & pubkey : pubkeys_) {
            assert(pubkey.size() == PUBLIC_KEY_BYTES_LEN);
        }
#endif
        assert(aggregate_pubkey_.size() == PUBLIC_KEY_BYTES_LEN);
        xbytes_t out;
        auto item1 = RLP::encodeList(pubkeys_);
        out.insert(out.end(), item1.begin(), item1.end());
        auto item2 = RLP::encode(aggregate_pubkey_);
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xsync_committee_t::decode_rlp, RLP::decode_list failed: msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() < 2) {
            xwarn("xsync_committee_t::decode_rlp, items.decoded.size() < 2, actual decoded size %s", items.decoded.size());
            ec = error::xerrc_t::rlp_list_size_not_match;
            return;
        }

        for (size_t i = 0; i < items.decoded.size() - 1; ++i) {
            auto const & data = items.decoded.at(i);
            if (data.size() != PUBLIC_KEY_BYTES_LEN) {
                xwarn("xsync_committee_t::decode_rlp, data.size() != PUBLIC_KEY_BYTES_LEN, actual size %zu", data.size());
                ec = error::xerrc_t::invalid_public_key_size;
                return;
            }

            pubkeys_.emplace_back();
            std::copy(data.begin(), data.end(), pubkeys_.back().begin());
        }

        // aggregate_pubkey = data;
        std::copy(items.decoded.back().begin(), items.decoded.back().end(), aggregate_pubkey_.begin());
    }

    xh256_t tree_hash_root() const {
        assert(!pubkeys_.empty());
        assert(aggregate_pubkey_.size() == PUBLIC_KEY_BYTES_LEN);
        xbytes_t data;
        for (auto const & p : pubkeys_) {
            data.insert(data.end(), p.begin(), p.end());
        }
        data.insert(data.end(), aggregate_pubkey_.begin(), aggregate_pubkey_.end());
        xbytes_t hash_bytes(32);
        unsafe_sync_committee_root(data.data(), data.size(), hash_bytes.data());
        return static_cast<xh256_t>(hash_bytes);
    }
};

struct xsync_aggregate_t {
    xbitset_t<SYNC_COMMITTEE_BITS_SIZE> sync_committee_bits;
    xbytes96_t sync_committee_signature;

    bool operator==(xsync_aggregate_t const & rhs) const {
        return (this->sync_committee_bits == rhs.sync_committee_bits) && (this->sync_committee_signature == rhs.sync_committee_signature);
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        assert(sync_committee_signature.size() == SIGNATURE_BYTES_LEN);

        auto bytes_le = sync_committee_bits.to<xbytes_t>(xendian_t::little);
        assert(bytes_le.size() == sync_committee_bits.size() / 8);
        auto const & item1 = RLP::encode(bytes_le);

        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encode(sync_committee_signature);
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xsync_aggregate_t::decode_rlp, RLP::decode_list failed: msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() != 2) {
            ec = error::xerrc_t::rlp_list_size_not_match;
            xwarn("xsync_aggregate_t::decode_rlp, items.decoded.size() != 2, actual decoded size %s", items.decoded.size());
            return;
        }

        auto const & committee_bits_bytes = items.decoded.at(0);
        if (committee_bits_bytes.size() != sync_committee_bits.size() / 8) {
            xwarn("xsync_aggregate_t::decode_rlp, committee_bits_bytes.size() != sync_committee_bits.size() / 8, actual size %zu", committee_bits_bytes.size());
            ec = error::xerrc_t::rlp_bytes_invalid;
            return;
        }
        sync_committee_bits = xbitset_t<SYNC_COMMITTEE_BITS_SIZE>::build_from(committee_bits_bytes, xendian_t::little);

        auto const & signature_bytes = items.decoded.at(1);
        if (signature_bytes.size() != SIGNATURE_BYTES_LEN) {
            xwarn("xsync_aggregate_t::decode_rlp, signature_bytes.size() != SIGNATURE_BYTES_LEN, actual size %zu", signature_bytes.size());
            ec = error::xerrc_t::rlp_bytes_invalid;
            return;
        }

        std::copy(std::begin(signature_bytes), std::end(signature_bytes), sync_committee_signature.begin());
        assert(!sync_committee_signature.empty());
    }
};

struct xsync_committee_update_t {
    xsync_committee_t next_sync_committee;
    std::vector<xh256_t> next_sync_committee_branch;

    bool operator==(xsync_committee_update_t const & rhs) const {
        return (this->next_sync_committee == rhs.next_sync_committee) && (this->next_sync_committee_branch == rhs.next_sync_committee_branch);
    }

    xbytes_t encode_rlp() const {
        assert(!next_sync_committee_branch.empty());

        xbytes_t out;
        auto const & item1 = RLP::encode(next_sync_committee.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encodeList(next_sync_committee_branch);
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xsync_committee_update_t::decode_rlp, RLP::decode_list failed: msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() < 2) {
            xwarn("xsync_committee_update_t::decode_rlp, items.decoded.size() < 2, actual size %zu", items.decoded.size());
            ec = error::xerrc_t::rlp_list_size_not_match;
            return;
        }

        next_sync_committee.decode_rlp(items.decoded.at(0), ec);
        if (ec) {
            xwarn("xsync_committee_update_t::decode_rlp, next_sync_committee.decode_rlp failed: msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        for (size_t i = 1; i < items.decoded.size(); ++i) {
            auto const & b = items.decoded.at(i);
            if (b.size() != xh256_t::size()) {
                xwarn("xsync_committee_update_t::decode_rlp, items.decoded[%zu].size() != 32, actual size %zu", i, b.size());
                ec = error::xerrc_t::rlp_bytes_invalid;
                return;
            }

            next_sync_committee_branch.emplace_back();
            std::copy(std::begin(b), std::end(b), next_sync_committee_branch.back().begin());
        }
    }
};

struct xheader_update_t {
    xbeacon_block_header_t beacon_header;
    xh256_t execution_block_hash;
    std::vector<xh256_t> execution_hash_branch;

    bool operator==(xheader_update_t const & rhs) const {
        return (this->beacon_header == rhs.beacon_header) && (this->execution_block_hash == rhs.execution_block_hash) && this->execution_hash_branch == rhs.execution_hash_branch;
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto const & item1 = RLP::encode(beacon_header.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encode(execution_block_hash.to_bytes());
        out.insert(out.end(), item2.begin(), item2.end());
        auto const & item3 = RLP::encodeList(execution_hash_branch);
        out.insert(out.end(), item3.begin(), item3.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xheader_update_t::decode_rlp, RLP::decode_list failed: msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() < 3) {
            xwarn("xheader_update_t::decode_rlp, items.decoded.size() < 3, actual size %zu", items.decoded.size());
            ec = error::xerrc_t::rlp_list_size_not_match;
            return;
        }

        beacon_header.decode_rlp(items.decoded.at(0), ec);
        if (ec) {
            xwarn("xheader_update_t::decode_rlp, beacon_header.decode_rlp failed: msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        auto const & execution_block_hash_bytes = items.decoded.at(1);
        if (execution_block_hash_bytes.size() != xh256_t::size()) {
            xwarn("xheader_update_t::decode_rlp, execution_block_hash_bytes.size() != xh256_t::size(), actual size %zu", execution_block_hash_bytes.size());
            ec = error::xerrc_t::rlp_bytes_invalid;
            return;
        }
        execution_block_hash = xh256_t{xspan_t<xbyte_t const>{execution_block_hash_bytes}};

        for (auto i = 2u; i < items.decoded.size(); ++i) {
            auto const & b = items.decoded.at(i);
            if (!b.empty()) {
                if (b.size() != xh256_t::size()) {
                    xwarn("xheader_update_t::decode_rlp, items.decoded[%zu].size() != 32, actual size %zu", i, b.size());
                    ec = error::xerrc_t::rlp_bytes_invalid;
                    return;
                }
                execution_hash_branch.emplace_back();
                std::copy(std::begin(b), std::end(b), execution_hash_branch.back().begin());
            }
        }
    }
};

struct xfinalized_header_update_t {
    xheader_update_t header_update;
    std::vector<xh256_t> finality_branch;

    bool operator==(xfinalized_header_update_t const & rhs) const {
        return (this->header_update == rhs.header_update) && (this->finality_branch == rhs.finality_branch);
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto const & item1 = RLP::encode(header_update.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encodeList(finality_branch);
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xfinalized_header_update_t::decode_rlp, RLP::decode_list failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() < 2) {
            xwarn("xfinalized_header_update_t::decode_rlp, items.decoded.size() < 2, actual size %zu", items.decoded.size());
            ec = error::xerrc_t::rlp_list_size_not_match;
            return;
        }

        header_update.decode_rlp(items.decoded.at(0), ec);
        if (ec) {
            xwarn("xfinalized_header_update_t::decode_rlp, header_update.decode_rlp failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        for (size_t i = 1; i < items.decoded.size(); ++i) {
            auto const & b = items.decoded.at(i);
            if (!b.empty()) {
                if (b.size() != xh256_t::size()) {
                    xwarn("xfinalized_header_update_t::decode_rlp, items.decoded[%zu].size() != xh256_t::size(), actual size %zu", i, b.size());
                    ec = error::xerrc_t::rlp_bytes_invalid;
                    return;
                }

                finality_branch.emplace_back();
                std::copy(std::begin(b), std::end(b), finality_branch.back().begin());
            }
        }
    }
};

struct xlight_client_update_t {
    xbeacon_block_header_t attested_beacon_header;
    xsync_aggregate_t sync_aggregate;
    uint64_t signature_slot{};
    xfinalized_header_update_t finality_update;
    xsync_committee_update_t sync_committee_update;

    bool operator==(xlight_client_update_t const & rhs) const {
        return (this->attested_beacon_header == rhs.attested_beacon_header) && (this->sync_aggregate == rhs.sync_aggregate) && (this->signature_slot == rhs.signature_slot) &&
               (this->finality_update == rhs.finality_update) && (this->sync_committee_update == rhs.sync_committee_update);
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto const & item1 = RLP::encode(attested_beacon_header.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encode(sync_aggregate.encode_rlp());
        out.insert(out.end(), item2.begin(), item2.end());
        auto const & item3 = RLP::encode(signature_slot);
        out.insert(out.end(), item3.begin(), item3.end());
        auto const & item4 = RLP::encode(finality_update.encode_rlp());
        out.insert(out.end(), item4.begin(), item4.end());
        auto const & item5 = RLP::encode(sync_committee_update.encode_rlp());
        out.insert(out.end(), item5.begin(), item5.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xlight_client_update_t::decode_rlp, RLP::decode_list failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() != 4 && items.decoded.size() != 5) {
            xwarn("xlight_client_update_t::decode_rlp, items.decoded.size() != 4 && items.decoded.size() != 5, actual size %zu", items.decoded.size());
            ec = error::xerrc_t::rlp_list_size_not_match;
            return;
        }

        attested_beacon_header.decode_rlp(items.decoded.at(0), ec);
        if (ec) {
            xwarn("xlight_client_update_t::decode_rlp, xbeacon_block_header_t::decode_rlp failed, msg %s ec %d", ec.message().c_str(), ec.value());
            return;
        }

        sync_aggregate.decode_rlp(items.decoded.at(1), ec);
        if (ec) {
            xwarn("xlight_client_update_t::decode_rlp, xsync_aggregate_t::decode_rlp failed, msg %s ec %d", ec.message().c_str(), ec.value());
            return;
        }

        signature_slot = fromBigEndian<uint64_t>(items.decoded.at(2));

        finality_update.decode_rlp(items.decoded.at(3), ec);
        if (ec) {
            xwarn("xlight_client_update_t::decode_rlp, xfinalized_header_update_t::decode_rlp failed, msg %s ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() == 5) {
            sync_committee_update.decode_rlp(items.decoded.at(4), ec);
            if (ec) {
                xwarn("xlight_client_update_t::decode_rlp, xsync_committee_update_t::decode_rlp failed, msg %s ec %d", ec.message().c_str(), ec.value());
                return;
            }
        }
    }
};

struct xlight_client_state_t {
    xextended_beacon_block_header_t finalized_beacon_header;
    xsync_committee_t current_sync_committee;
    xsync_committee_t next_sync_committee;

    xlight_client_state_t() = default;
    xlight_client_state_t(xlight_client_state_t const &) = default;
    xlight_client_state_t & operator=(xlight_client_state_t const &) = default;
    xlight_client_state_t(xlight_client_state_t &&) = default;
    xlight_client_state_t & operator=(xlight_client_state_t &&) = default;
    ~xlight_client_state_t() = default;

    xlight_client_state_t(xextended_beacon_block_header_t const & header, xsync_committee_t const & cur_committee, xsync_committee_t const & next_committee)
      : finalized_beacon_header{header}, current_sync_committee(cur_committee), next_sync_committee(next_committee) {
    }

    bool operator==(xlight_client_state_t const & rhs) const {
        return (this->finalized_beacon_header == rhs.finalized_beacon_header) && (this->current_sync_committee == rhs.current_sync_committee) &&
               (this->next_sync_committee == rhs.next_sync_committee);
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto const & item1 = RLP::encode(finalized_beacon_header.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encode(current_sync_committee.encode_rlp());
        out.insert(out.end(), item2.begin(), item2.end());
        auto const & item3 = RLP::encode(next_sync_committee.encode_rlp());
        out.insert(out.end(), item3.begin(), item3.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xlight_client_state_t::decode_rlp, RLP::decode_list failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() != 3) {
            xwarn("xlight_client_state_t::decode_rlp, items.decoded.size() != 3, actual size %zu", items.decoded.size());
            ec = error::xerrc_t::rlp_list_size_not_match;
            return;
        }

        finalized_beacon_header.decode_rlp(items.decoded.at(0), ec);
        if (ec) {
            xwarn("xlight_client_state_t::decode_rlp, xextended_beacon_block_header_t::decode_rlp failed, msg %s ec %d", ec.message().c_str(), ec.value());
            return;
        }

        current_sync_committee.decode_rlp(items.decoded.at(1), ec);
        if (ec) {
            xwarn("xlight_client_state_t::decode_rlp, xsync_committee_t::decode_rlp failed, msg %s ec %d", ec.message().c_str(), ec.value());
            return;
        }

        next_sync_committee.decode_rlp(items.decoded.at(2), ec);
        if (ec) {
            xwarn("xlight_client_state_t::decode_rlp, xsync_committee_t::decode_rlp failed, msg %s ec %d", ec.message().c_str(), ec.value());
            return;
        }
    }
};

struct xinit_input_t {
    xeth_header_t finalized_execution_header;
    xextended_beacon_block_header_t finalized_beacon_header;
    xsync_committee_t current_sync_committee;
    xsync_committee_t next_sync_committee;

    bool operator==(xinit_input_t const & rhs) const {
        return (this->finalized_execution_header == rhs.finalized_execution_header) && (this->finalized_beacon_header == rhs.finalized_beacon_header) &&
               (this->current_sync_committee == rhs.current_sync_committee) && (this->next_sync_committee == rhs.next_sync_committee);
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto const & item1 = RLP::encode(finalized_execution_header.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encode(finalized_beacon_header.encode_rlp());
        out.insert(out.end(), item2.begin(), item2.end());
        auto const & item3 = RLP::encode(current_sync_committee.encode_rlp());
        out.insert(out.end(), item3.begin(), item3.end());
        auto const & item4 = RLP::encode(next_sync_committee.encode_rlp());
        out.insert(out.end(), item4.begin(), item4.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        std::error_code ec;
        decode_rlp(bytes, ec);
        return !ec;
    }

    void decode_rlp(xbytes_t const & bytes, std::error_code & ec) {
        assert(!ec);

        auto const & items = RLP::decode_list(bytes, ec);
        if (ec) {
            xwarn("xinit_input_t::decode_rlp, decode_list failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        if (items.decoded.size() != 4) {
            xwarn("xinit_input_t::decode_rlp, items.decoded.size() != 4");
            ec = error::xerrc_t::rlp_list_size_not_match;
            return;
        }

        finalized_execution_header.decode_rlp(items.decoded.at(0), ec);
        if (ec) {
            xwarn("xinit_input_t::decode_rlp, finalized_execution_header.decode_rlp failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        finalized_beacon_header.decode_rlp(items.decoded.at(1), ec);
        if (ec) {
            xwarn("xinit_input_t::decode_rlp, finalized_beacon_header.decode_rlp failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        current_sync_committee.decode_rlp(items.decoded.at(2), ec);
        if (ec) {
            xwarn("xinit_input_t::decode_rlp, current_sync_committee.decode_rlp failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }

        next_sync_committee.decode_rlp(items.decoded.at(3), ec);
        if (ec) {
            xwarn("xinit_input_t::decode_rlp, next_sync_committee.decode_rlp failed, msg %s, ec %d", ec.message().c_str(), ec.value());
            return;
        }
    }
};

struct xtop_network_config {
    xh256_t genesis_validators_root;

    xfork_version_t bellatrix_fork_version{};
    uint64_t bellatrix_fork_epoch{};

    xfork_version_t capella_fork_version;
    uint64_t capella_fork_epoch{};

    xfork_version_t deneb_fork_version;
    uint64_t deneb_fork_epoch{std::numeric_limits<uint64_t>::max()};

    explicit xtop_network_config(xnetwork_id_t const network_id) {
        switch (network_id) {
        case xnetwork_id_t::mainnet: {
            genesis_validators_root = xh256_t{std::vector<uint8_t>{
                0x4b, 0x36, 0x3d, 0xb9, 0x4e, 0x28, 0x61, 0x20, 0xd7, 0x6e, 0xb9, 0x05, 0x34, 0x0f, 0xdd, 0x4e,
                0x54, 0xbf, 0xe9, 0xf0, 0x6b, 0xf3, 0x3f, 0xf6, 0xcf, 0x5a, 0xd2, 0x7f, 0x51, 0x1b, 0xfe, 0x95,
            }};

            bellatrix_fork_version = xfork_version_t{0x02, 0x00, 0x00, 0x00};
            bellatrix_fork_epoch = 144896;

            capella_fork_version = xfork_version_t{0x03, 0x00, 0x00, 0x00};
            capella_fork_epoch = 194048;

            deneb_fork_version = xfork_version_t{0x04, 0x00, 0x00, 0x00};
            deneb_fork_epoch = 269568;

            break;
        }

        case xnetwork_id_t::goerli: {
            genesis_validators_root = xh256_t{std::vector<uint8_t>{
                0x04, 0x3d, 0xb0, 0xd9, 0xa8, 0x38, 0x13, 0x55, 0x1e, 0xe2, 0xf3, 0x34, 0x50, 0xd2, 0x37, 0x97,
                0x75, 0x7d, 0x43, 0x09, 0x11, 0xa9, 0x32, 0x05, 0x30, 0xad, 0x8a, 0x0e, 0xab, 0xc4, 0x3e, 0xfb,
            }};

            bellatrix_fork_version = xfork_version_t{0x02, 0x00, 0x10, 0x20};
            bellatrix_fork_epoch = 112260;

            capella_fork_version = xfork_version_t{0x03, 0x00, 0x10, 0x20};
            capella_fork_epoch = 162304;

            deneb_fork_version = xfork_version_t{0x04, 0x00, 0x10, 0x20};
            deneb_fork_epoch = 231680;

            break;
        }

        case xnetwork_id_t::sepolia: {
            genesis_validators_root = xh256_t{std::vector<uint8_t>{
                0xd8, 0xea, 0x17, 0x1f, 0x3c, 0x94, 0xae, 0xa2, 0x1e, 0xbc, 0x42, 0xa1, 0xed, 0x61, 0x05, 0x2a,
                0xcf, 0x3f, 0x92, 0x09, 0xc0, 0x0e, 0x4e, 0xfb, 0xaa, 0xdd, 0xac, 0x09, 0xed, 0x9b, 0x80, 0x78,
            }};

            bellatrix_fork_version = xfork_version_t{0x90, 0x00, 0x00, 0x71};
            bellatrix_fork_epoch = 100;

            capella_fork_version = xfork_version_t{0x90, 0x00, 0x00, 0x72};
            capella_fork_epoch = 56832;

            deneb_fork_version = xfork_version_t{0x90, 0x00, 0x00, 0x73};
            deneb_fork_epoch = 132608;

            break;
        }
        default: {
            assert(false);
            break;
        }
        }
    }

    auto compute_fork_version(xepoch_t const epoch) const -> optional<xfork_version_t> {
        if (epoch >= this->deneb_fork_epoch) {
            return this->deneb_fork_version;
        }

        if (epoch >= this->capella_fork_epoch) {
            return this->capella_fork_version;
        }

        if (epoch >= this->bellatrix_fork_epoch) {
            return this->bellatrix_fork_version;
        }

        return top::nullopt;
    }

    auto compute_fork_version_by_slot(xslot_t const slot) const -> optional<xfork_version_t> {
        return this->compute_fork_version(compute_epoch_at_slot(slot));
    }

    auto compute_proof_size(xepoch_t const epoch) const -> xproof_size_t {
        if (epoch >= this->deneb_fork_epoch) {
            return xproof_size_t{
                4,   // beacon_block_body_tree_depth
                9,   // l1_beacon_block_body_tree_execution_payload_index
                12,  // l2_execution_payload_tree_execution_block_index
                4,   // l1_beacon_block_body_proof_size
                5,   // l2_execution_payload_proof_size
                9,   // execution_proof_size
            };
        }

        return xproof_size_t{
            4,   // beacon_block_body_tree_depth
            9,   // l1_beacon_block_body_tree_execution_payload_index
            12,  // l2_execution_payload_tree_execution_block_index
            4,   // l1_beacon_block_body_proof_size
            4,   // l2_execution_payload_proof_size
            8,   // execution_proof_size
        };
    }

    auto compute_proof_size_by_slot(xslot_t const slot) const -> xproof_size_t {
        return this->compute_proof_size(compute_epoch_at_slot(slot));
    }
};
using xnetwork_config_t = xtop_network_config;

NS_END3
