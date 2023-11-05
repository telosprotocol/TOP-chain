// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbitset.h"
#include "xbasic/xbytes.h"
#include "xbasic/xfixed_hash.h"
#include "xbasic/xoptional.hpp"
#include "xevm_common/xcommon.h"

#include <cstdint>

NS_BEG4(top, evm, crosschain, bsc)

constexpr size_t MAX_ATTESTATION_EXTRA_SIZE = 256;

class xtop_vote_data {
private:
    std::uint64_t source_number_{0};
    xh256_t source_hash_{};
    std::uint64_t target_number_{0};
    xh256_t target_hash_{};

public:
    xtop_vote_data() = default;
    xtop_vote_data(std::uint64_t source_number, xh256_t const & source_hash, std::uint64_t target_number, xh256_t const & target_hash);

    auto source_number() const noexcept -> std::uint64_t;
    auto source_hash() const noexcept -> xh256_t const &;
    auto target_number() const noexcept -> std::uint64_t;
    auto target_hash() const noexcept -> xh256_t const &;

    auto encode_rlp() const -> xbytes_t;

    auto hash() const -> xh256_t;
};
using xvote_data_t = xtop_vote_data;

struct xtop_vote_attestation {
    uint64_t vote_address_set{};
    evm::common::xbls_signature_t aggregate_signature;
    optional<xvote_data_t> data;
    xbytes_t extra;

    auto decode_rlp(xbytes_t const & bytes, std::error_code & ec) -> void;
};
using xvote_attestation_t = xtop_vote_attestation;

NS_END4
