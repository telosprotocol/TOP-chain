// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xfixed_hash.h"
#include "xcommon/xcommon.h"
#include "xevm_common/xcommon.h"

#include <cstdint>
#include <limits>

NS_BEG4(top, evm, crosschain, bsc)

constexpr auto is_forked(uint64_t const s, uint64_t const head) noexcept -> bool {
    return (std::numeric_limits<uint64_t>::max() != s) && (std::numeric_limits<uint64_t>::max() != head) && (s <= head);
}

struct xtop_parlia_config {
    uint64_t period;
    uint64_t epoch;
};
using xparlia_config_t = xtop_parlia_config;

struct xtop_chain_config {
    uint64_t chain_id;

    uint64_t homestead_block;

    uint64_t dao_fork_block;
    bool    dao_fork_support;

    uint64_t eip150_block;
    xh256_t eip150_hash;

    uint64_t eip155_block;
    uint64_t eip158_block;

    uint64_t byzantium_block;
    uint64_t constantinople_block;
    uint64_t petersburg_block;
    uint64_t istanbul_block;
    uint64_t muir_glacier_block;
    uint64_t berlin_block;
    uint64_t yolo_v3_block;
    uint64_t catalyst_block;
    uint64_t london_block;
    uint64_t arrow_glacier_block;
    uint64_t merge_fork_block;

    uint64_t terminal_total_difficulty;

    uint64_t ramanujan_block;
    uint64_t niels_block;
    uint64_t mirror_sync_block;
    uint64_t bruno_block;
    uint64_t euler_block;
    uint64_t nano_block;
    uint64_t moran_block;
    uint64_t gibbs_block;
    uint64_t planck_block;
    uint64_t luban_block;
    uint64_t plato_block;
    uint64_t hertz_block;

    xparlia_config_t parlia_config;
};
using xchain_config_t = xtop_chain_config;

extern xchain_config_t const bsc_chain_config;

XINLINE_CONSTEXPR size_t EXTRA_VANITY{32};
XINLINE_CONSTEXPR size_t EXTRA_SEAL{65};
XINLINE_CONSTEXPR size_t VALIDATOR_BYTES_LENGTH{ETH_ADDRESS_LENGTH + evm::common::BLS_PUBLIC_KEY_LEN};
XINLINE_CONSTEXPR size_t VALIDATOR_NUMBER_SIZE{1};

constexpr auto is_luban(xchain_config_t const & chain_config, uint64_t const number) noexcept -> bool {
    return is_forked(chain_config.luban_block, number);
}

NS_END4
