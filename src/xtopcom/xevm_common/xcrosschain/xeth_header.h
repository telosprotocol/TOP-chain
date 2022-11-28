#pragma once

#include "xbasic/xoptional.hpp"
#include "xevm_common/common.h"
#include "xevm_common/fixed_hash.h"

#include <vector>

NS_BEG2(top, evm_common)

// The log bloom's size (2048-bit).
using Hash = top::evm_common::h256;
using Address = top::evm_common::h160;
using LogBloom = top::evm_common::h2048;
using BlockNonce = top::evm_common::h64;

struct xeth_header_t {
    Hash parent_hash;
    Hash uncle_hash;
    Address miner;
    Hash state_merkleroot;
    Hash tx_merkleroot;
    Hash receipt_merkleroot;
    LogBloom bloom;
    bigint difficulty;
    bigint number;
    uint64_t gas_limit{0};
    uint64_t gas_used{0};
    uint64_t time{0};
    xbytes_t extra;
    Hash mix_digest;
    BlockNonce nonce;

    // base_fee was added by EIP-1559 and is ignored in legacy headers.
    optional<bigint> base_fee;

    bool operator==(xeth_header_t const & rhs) const;

    // hash
    Hash hash() const;
    Hash hash_without_seal() const;

    // encode and decode
    xbytes_t encode_rlp() const;
    xbytes_t encode_rlp_withoutseal() const;
    bool decode_rlp(const xbytes_t & bytes);

    // debug
    std::string dump() const;
    void print() const;
};

struct xeth_header_info_t {
    xeth_header_info_t() = default;
    xeth_header_info_t(bigint difficult_sum_, Hash parent_hash_, bigint number_);

    xbytes_t encode_rlp() const;
    bool decode_rlp(const xbytes_t & input);

    bigint difficult_sum;
    Hash parent_hash;
    bigint number;
};

NS_END2
