#include "xevm_common/xcrosschain/xeth_eip1559.h"

#include "xevm_common/xcrosschain/xeth_config.h"
#include "xevm_common/xcrosschain/xeth_gaslimit.h"

NS_BEG3(top, evm_common, eth)

constexpr uint64_t GasLimitBoundDivisor = 1024;
constexpr uint64_t ElasticityMultiplier = 2;
constexpr uint64_t InitialBaseFee = 1000000000;
constexpr uint64_t BaseFeeChangeDenominator = 8;

static bigint calc_baseFee(const xeth_header_t & parentHeader) {
    if (!eth::config::is_london(parentHeader.number)) {
        return bigint(InitialBaseFee);
    }
    auto parentGasTarget = parentHeader.gas_limit / ElasticityMultiplier;
    bigint parentGasTargetBig = parentGasTarget;
    bigint baseFeeChangeDenominator = BaseFeeChangeDenominator;
    // If the parent gasUsed is the same as the target, the baseFee remains unchanged.
    if (parentHeader.gas_used == parentGasTarget) {
        return bigint(parentHeader.base_fee.value());
    }
    if (parentHeader.gas_used > parentGasTarget) {
        bigint gasUsedDelta = bigint(parentHeader.gas_used - parentGasTarget);
        bigint x = parentHeader.base_fee.value() * gasUsedDelta;
        bigint y = x / parentGasTargetBig;
        bigint baseFeeDelta = y / baseFeeChangeDenominator;
        if (baseFeeDelta < 1) {
            baseFeeDelta = 1;
        }
        return parentHeader.base_fee.value() + baseFeeDelta;
    } else {
        // Otherwise if the parent block used less gas than its target, the baseFee should decrease.
        bigint gasUsedDelta = bigint(parentGasTarget - parentHeader.gas_used);
        bigint x = parentHeader.base_fee.value() * gasUsedDelta;
        bigint y = x / parentGasTargetBig;
        bigint baseFeeDelta = y / baseFeeChangeDenominator;
        x = parentHeader.base_fee.value() - baseFeeDelta;
        if (x < 0) {
            x = 0;
        }

        return x;
    }
    return 0;
}

// VerifyEip1559Header verifies some header attributes which were changed in EIP-1559,
// - gas limit check
// - basefee check
bool verify_eip1559_header(const xeth_header_t & parentHeader, const xeth_header_t & header) {
    // Verify that the gas limit remains within allowed bounds
    auto parentGasLimit = parentHeader.gas_limit;
    if (!eth::config::is_london(parentHeader.number)) {
        parentGasLimit = parentHeader.gas_limit * ElasticityMultiplier;
    }
    if (!eth::verify_gaslimit(parentGasLimit, header.gas_limit)) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] gaslimit mismatch, new: %lu, old: %lu", header.gas_limit, parentHeader.gas_limit);
        return false;
    }
    // Verify the baseFee is correct based on the parent header.
    auto expectedBaseFee = calc_baseFee(parentHeader);
    if (header.base_fee.value() != expectedBaseFee) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] wrong basefee: %s, should be: %s", header.base_fee.value().str().c_str(), expectedBaseFee.str().c_str());
        return false;
    }
    return true;
}

NS_END3
