// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xfixed_hash.h"
#include "xbasic/xoptional.hpp"
#include "xcommon/common.h"
#include "xcommon/xeth_address.h"

#include <system_error>
#include <vector>

NS_BEG2(top, evm_common)

// The log bloom's size (2048-bit).
//using Hash = top::evm_common::h256;
// using Address = top::evm_common::h160;
using LogBloom = top::evm_common::h2048;

struct xeth_header_t {
    xh256_t parent_hash;
    xh256_t uncle_hash;
    common::xeth_address_t miner;
    xh256_t state_root;
    xh256_t transactions_root;
    xh256_t receipts_root;
    LogBloom bloom;
    u256 difficulty;
    uint64_t number;
    u256 gas_limit{0};
    u256 gas_used{0};
    uint64_t time{0};
    xbytes_t extra;
    xh256_t mix_digest;
    xh64_t nonce;

    // base_fee was added by EIP-1559 and is ignored in legacy headers.
    optional<uint64_t> base_fee_per_gas;
    optional<xh256_t> withdrawals_root;

    mutable xh256_t hash;
    mutable xh256_t partial_hash;

    bool operator==(xeth_header_t const & rhs) const;

    // hash
    xh256_t calc_hash(bool partial = false) const;
    void calc_hash(xh256_t & out, bool partial = false) const;

    // encode and decode
    xbytes_t encode_rlp(bool partial = false) const;
    void decode_rlp(xbytes_t const & bytes);
    void decode_rlp(xbytes_t const & bytes, std::error_code & ec);

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

common::xeth_address_t ecrecover(xeth_header_t const & header, std::error_code & ec);

NS_END2
