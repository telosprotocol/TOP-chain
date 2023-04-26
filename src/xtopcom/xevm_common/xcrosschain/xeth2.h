
#pragma once

#include "xbasic/xfixed_hash.h"
#include "xbasic/xfixed_bytes.h"
#include "xcommon/common.h"
#include "xcommon/rlp.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_runner/evm_engine_interface.h"

NS_BEG3(top, evm_common, eth2)

XINLINE_CONSTEXPR size_t PUBLIC_KEY_BYTES_LEN{48};
#define HASH_LEN 32U
XINLINE_CONSTEXPR size_t SIGNATURE_BYTES_LEN{96};
XINLINE_CONSTEXPR size_t SYNC_COMMITTEE_BITS_SIZE_IN_BYTES{512 >> 3};

using xbytes48_t = xfixed_bytes_t<PUBLIC_KEY_BYTES_LEN>;

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
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() != 5) {
            return false;
        }
        slot = fromBigEndian<uint64_t>(items.decoded.at(0));
        proposer_index = fromBigEndian<uint64_t>(items.decoded.at(1));
        parent_root = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(2)}};
        state_root = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(3)}};
        body_root = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(4)}};
        return true;
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
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() != 3) {
            return false;
        }
        if (header.decode_rlp(items.decoded.at(0)) == false) {
            assert(header.empty());
            return false;
        }
        beacon_block_root = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(1)}};
        execution_block_hash = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(2)}};
        return true;
    }
};

struct xexecution_header_info_t {
    xh256_t parent_hash{};
    uint64_t block_number{0};

    xexecution_header_info_t() = default;
    xexecution_header_info_t(xexecution_header_info_t const &) = default;
    xexecution_header_info_t & operator=(xexecution_header_info_t const &) = default;
    xexecution_header_info_t(xexecution_header_info_t &&) = default;
    xexecution_header_info_t & operator=(xexecution_header_info_t &&) = default;
    ~xexecution_header_info_t() = default;

    xexecution_header_info_t(xh256_t const & parent_hash, uint64_t const block_number) : parent_hash(parent_hash), block_number(block_number) {
    }

    bool operator==(xexecution_header_info_t const & rhs) const {
        return (this->parent_hash == rhs.parent_hash) && (this->block_number == rhs.block_number);
    }

    bool empty() const {
        return (parent_hash.empty() && block_number == 0);
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto item1 = RLP::encode(parent_hash.asArray());
        out.insert(out.end(), item1.begin(), item1.end());
        auto item2 = RLP::encode(block_number);
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() != 2) {
            return false;
        }
        parent_hash = xh256_t{xspan_t<xbyte_t const>{items.decoded.at(0)}};
        block_number = fromBigEndian<uint64_t>(items.decoded.at(1));
        return true;
    }
};

struct xsync_committee_t {
    std::vector<xbytes48_t> pubkeys;
    xbytes48_t aggregate_pubkey;

    bool operator==(xsync_committee_t const & rhs) const {
        return (this->pubkeys == rhs.pubkeys) && (this->aggregate_pubkey == rhs.aggregate_pubkey);
    }

    bool empty() const {
        return (pubkeys.empty() && aggregate_pubkey.empty());
    }

    xbytes_t encode_rlp() const {
        assert(!pubkeys.empty());
        assert(!aggregate_pubkey.empty());
#if !defined(NDEBUG)
        for (auto const & pubkey : pubkeys) {
            assert(pubkey.size() == PUBLIC_KEY_BYTES_LEN);
        }
#endif
        assert(aggregate_pubkey.size() == PUBLIC_KEY_BYTES_LEN);
        xbytes_t out;
        auto item1 = RLP::encodeList(pubkeys);
        out.insert(out.end(), item1.begin(), item1.end());
        auto item2 = RLP::encode(aggregate_pubkey);
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() < 2) {
            return false;
        }

        for (size_t i = 0; i < items.decoded.size(); ++i) {
            auto const & data = items.decoded.at(i);
            if (data.size() != PUBLIC_KEY_BYTES_LEN) {
                return false;
            }
            if (i != items.decoded.size() - 1) {
                pubkeys.emplace_back();
                std::copy(data.begin(), data.end(), pubkeys.back().begin());
            } else {
                // aggregate_pubkey = data;
                std::copy(data.begin(), data.end(), aggregate_pubkey.begin());
            }
        }
        return true;
    }

    h256 tree_hash_root() const {
        assert(!pubkeys.empty());
        assert(aggregate_pubkey.size() == PUBLIC_KEY_BYTES_LEN);
        xbytes_t data;
        for (auto const & p : pubkeys) {
            data.insert(data.end(), p.begin(), p.end());
        }
        data.insert(data.end(), aggregate_pubkey.begin(), aggregate_pubkey.end());
        xbytes_t hash_bytes(32);
        unsafe_sync_committee_root(data.data(), data.size(), hash_bytes.data());
        return static_cast<h256>(hash_bytes);
    }
};

struct xsync_aggregate_t {
    xbytes_t sync_committee_bits;
    xbytes_t sync_committee_signature;

    bool operator==(xsync_aggregate_t const & rhs) const {
        return (this->sync_committee_bits == rhs.sync_committee_bits) && (this->sync_committee_signature == rhs.sync_committee_signature);
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        assert(sync_committee_signature.size() == SIGNATURE_BYTES_LEN);
        auto const & item1 = RLP::encode(sync_committee_bits);
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encode(sync_committee_signature);
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() != 2) {
            return false;
        }
        sync_committee_bits = items.decoded.at(0);
        sync_committee_signature = items.decoded.at(1);
        if (sync_committee_signature.size() != SIGNATURE_BYTES_LEN) {
            return false;
        }
        return true;
    }
};

struct xsync_committee_update_t {
    xsync_committee_t next_sync_committee;
    std::vector<xbytes_t> next_sync_committee_branch;

    bool operator==(xsync_committee_update_t const & rhs) const {
        return (this->next_sync_committee == rhs.next_sync_committee) && (this->next_sync_committee_branch == rhs.next_sync_committee_branch);
    }

    xbytes_t encode_rlp() const {
        assert(!next_sync_committee_branch.empty());
#if !defined(NDEBUG)
        for (auto const & b : next_sync_committee_branch) {
            assert(b.size() == HASH_LEN);
        }
#endif
        xbytes_t out;
        auto const & item1 = RLP::encode(next_sync_committee.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encodeList(next_sync_committee_branch);
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() < 2) {
            return false;
        }
        if (false == next_sync_committee.decode_rlp(items.decoded.at(0))) {
            return false;
        }
        for (size_t i = 1; i < items.decoded.size(); ++i) {
            auto const & b = items.decoded.at(i);
            if (b.size() != HASH_LEN) {
                return false;
            }
            next_sync_committee_branch.emplace_back(b);
        }
        return true;
    }
};

struct xheader_update_t {
    xbeacon_block_header_t beacon_header;
    h256 execution_block_hash;

    bool operator==(xheader_update_t const & rhs) const {
        return (this->beacon_header == rhs.beacon_header) && (this->execution_block_hash == rhs.execution_block_hash);
    }

    xbytes_t encode_rlp() const {
        xbytes_t out;
        auto const & item1 = RLP::encode(beacon_header.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encode(execution_block_hash.to_bytes());
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() != 2) {
            return false;
        }
        if (false == beacon_header.decode_rlp(items.decoded.at(0))) {
            return false;
        }
        execution_block_hash = static_cast<h256>(items.decoded.at(1));
        return true;
    }
};

struct xfinalized_header_update_t {
    xheader_update_t header_update;
    std::vector<xbytes_t> finality_branch;

    bool operator==(xfinalized_header_update_t const & rhs) const {
        return (this->header_update == rhs.header_update) && (this->finality_branch == rhs.finality_branch);
    }

    xbytes_t encode_rlp() const {
#if !defined(NDEBUG)
        for (auto const & b : finality_branch) {
            assert(b.size() == HASH_LEN);
        }
#endif
        xbytes_t out;
        auto const & item1 = RLP::encode(header_update.encode_rlp());
        out.insert(out.end(), item1.begin(), item1.end());
        auto const & item2 = RLP::encodeList(finality_branch);
        out.insert(out.end(), item2.begin(), item2.end());
        return RLP::encodeList(out);
    }

    bool decode_rlp(xbytes_t const & bytes) {
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() < 2) {
            return false;
        }
        if (false == header_update.decode_rlp(items.decoded.at(0))) {
            return false;
        }
        for (size_t i = 1; i < items.decoded.size(); ++i) {
            auto const & b = items.decoded.at(i);
            if (b.size() != HASH_LEN) {
                return false;
            }
            finality_branch.emplace_back(b);
        }
        return true;
    }
};

struct xlight_client_update_t {
    xbeacon_block_header_t attested_beacon_header;
    xsync_aggregate_t sync_aggregate;
    uint64_t signature_slot;
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
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() != 4 && items.decoded.size() != 5) {
            return false;
        }
        if (false == attested_beacon_header.decode_rlp(items.decoded.at(0))) {
            return false;
        }
        if (false == sync_aggregate.decode_rlp(items.decoded.at(1))) {
            return false;
        }
        signature_slot = static_cast<uint64_t>(fromBigEndian<u64>(items.decoded.at(2)));
        if (false == finality_update.decode_rlp(items.decoded.at(3))) {
            return false;
        }
        if (items.decoded.size() == 5) {
            if (false == sync_committee_update.decode_rlp(items.decoded.at(4))) {
                return false;
            }
        }
        return true;
    }
};

struct xlight_client_state_t {
    xextended_beacon_block_header_t finalized_beacon_header;
    xsync_committee_t current_sync_committee;
    xsync_committee_t next_sync_committee;

    xlight_client_state_t() = default;
    xlight_client_state_t(xextended_beacon_block_header_t const & header, xsync_committee_t const & cur_committee, xsync_committee_t const & next_committee)
      : finalized_beacon_header(header), current_sync_committee(cur_committee), next_sync_committee(next_committee) {
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
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() != 3) {
            return false;
        }
        if (false == finalized_beacon_header.decode_rlp(items.decoded.at(0))) {
            return false;
        }
        if (false == current_sync_committee.decode_rlp(items.decoded.at(1))) {
            return false;
        }
        if (false == next_sync_committee.decode_rlp(items.decoded.at(2))) {
            return false;
        }
        return true;
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
        auto const & items = RLP::decodeList(bytes);
        if (items.decoded.size() != 4) {
            return false;
        }
        if (false == finalized_execution_header.decode_rlp(items.decoded.at(0))) {
            return false;
        }
        if (false == finalized_beacon_header.decode_rlp(items.decoded.at(1))) {
            return false;
        }
        if (false == current_sync_committee.decode_rlp(items.decoded.at(2))) {
            return false;
        }
        if (false == next_sync_committee.decode_rlp(items.decoded.at(3))) {
            return false;
        }
        return true;
    }
};

NS_END3
