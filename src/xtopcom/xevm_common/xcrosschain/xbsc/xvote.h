// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbytes.h"
#include "xbasic/xfixed_hash.h"
#include "xevm_common/xcommon.h"

#include <cstdint>

NS_BEG4(top, evm, crosschain, bsc)

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

NS_END4
