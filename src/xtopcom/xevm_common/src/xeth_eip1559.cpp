// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/xcrosschain/xeth_eip1559.h"

#include "xevm_common/xcrosschain/xeth_config.h"
#include "xevm_common/xcrosschain/xeth_gaslimit.h"

#include <cassert>
#include <cinttypes>

NS_BEG3(top, evm_common, eth)

constexpr uint64_t GasLimitBoundDivisor = 1024;
constexpr uint64_t ElasticityMultiplier = 2;
constexpr uint64_t InitialBaseFee = 1000000000;
constexpr uint64_t BaseFeeChangeDenominator = 8;

static uint64_t calc_base_fee(xeth_header_t const & parent_header) {
    if (!eth::config::is_london(parent_header.number)) {
        return InitialBaseFee;
    }

    assert(parent_header.base_fee_per_gas.has_value());

    auto const parent_gas_target = parent_header.gas_limit / ElasticityMultiplier;
    auto const & parent_gas_target_big = parent_gas_target;
    constexpr auto base_fee_change_denominator = BaseFeeChangeDenominator;
    // If the parent gasUsed is the same as the target, the baseFee remains unchanged.
    if (parent_header.gas_used == parent_gas_target) {
        return parent_header.base_fee_per_gas.value();
    }

    if (parent_header.gas_used > parent_gas_target) {
        auto const gas_used_delta = parent_header.gas_used - parent_gas_target;
        auto const x = parent_header.base_fee_per_gas.value() * gas_used_delta;
        auto const y = x / parent_gas_target_big;
        auto base_fee_delta = y / base_fee_change_denominator;
        if (base_fee_delta < 1) {
            base_fee_delta = 1;
        }
        return (parent_header.base_fee_per_gas.value() + base_fee_delta).convert_to<uint64_t>();
    }

    // Otherwise if the parent block used less gas than its target, the baseFee should decrease.
    auto const gas_used_delta = parent_gas_target - parent_header.gas_used;
    auto x = parent_header.base_fee_per_gas.value() * gas_used_delta;
    auto const y = x / parent_gas_target_big;
    auto const base_fee_delta = y / base_fee_change_denominator;
    x = parent_header.base_fee_per_gas.value() - base_fee_delta;
    if (x < 0) {
        x = 0;
    }

    return x.convert_to<uint64_t>();
}

// VerifyEip1559Header verifies some header attributes which were changed in EIP-1559,
// - gas limit check
// - basefee check
bool verify_eip1559_header(xeth_header_t const & parent_header, xeth_header_t const & header) {
    // Verify that the gas limit remains within allowed bounds
    auto parent_gas_limit = parent_header.gas_limit;
    if (!eth::config::is_london(parent_header.number)) {
        parent_gas_limit = parent_header.gas_limit * ElasticityMultiplier;
    }

    assert(parent_header.base_fee_per_gas.has_value());

    if (!eth::verify_gaslimit(parent_gas_limit, header.gas_limit)) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] gaslimit mismatch, new: %s, old: %s", header.gas_limit.str().c_str(), parent_header.gas_limit.str().c_str());
        return false;
    }
    // Verify the baseFee is correct based on the parent header.
    auto const expected_base_fee = calc_base_fee(parent_header);
    if (header.base_fee_per_gas.value() != expected_base_fee) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] wrong basefee: %" PRIu64 ", should be: %" PRIu64, header.base_fee_per_gas.value(), expected_base_fee);
        return false;
    }
    return true;
}

NS_END3
