// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xoptional.hpp"
#include "xcommon/common.h"
#include "xcommon/xfixed_hash.h"
#include "xcommon/xeth_address.h"

#include <vector>

NS_BEG2(top, evm_common)

// The log bloom's size (2048-bit).
//using Hash = top::evm_common::h256;
// using Address = top::evm_common::h160;
using LogBloom = top::evm_common::h2048;
using BlockNonce = uint64_t;

struct xeth_header_t {
    xh256_t parent_hash;
    xh256_t uncle_hash;
    common::xeth_address_t miner;
    xh256_t state_merkleroot;
    xh256_t tx_merkleroot;
    xh256_t receipt_merkleroot;
    LogBloom bloom;
    u256 difficulty;
    uint64_t number;
    uint64_t gas_limit{0};
    uint64_t gas_used{0};
    uint64_t time{0};
    xbytes_t extra;
    xh256_t mix_digest;
    uint64_t nonce;

    // base_fee was added by EIP-1559 and is ignored in legacy headers.
    optional<bigint> base_fee;
    optional<xh256_t> withdrawals_hash;

    bool operator==(xeth_header_t const & rhs) const;

    // hash
    xh256_t hash() const;
    xh256_t hash_without_seal() const;

    // encode and decode
    xbytes_t encode_rlp() const;
    xbytes_t encode_rlp_withoutseal() const;
    bool decode_rlp(xbytes_t const & bytes);

    // debug
    std::string dump() const;
    void print() const;
};

struct xeth_header_info_t {
    xeth_header_info_t() = default;
    xeth_header_info_t(xeth_header_info_t const &) = default;
    xeth_header_info_t & operator=(xeth_header_info_t const &) = default;
    xeth_header_info_t(xeth_header_info_t &&) = default;
    xeth_header_info_t & operator=(xeth_header_info_t &&) = default;
    ~xeth_header_info_t() = default;

    xeth_header_info_t(bigint difficult_sum, xh256_t parent_hash, bigint number);

    xbytes_t encode_rlp() const;
    bool decode_rlp(xbytes_t const & input);

    bigint difficult_sum;
    xh256_t parent_hash;
    bigint number;
};

NS_END2
