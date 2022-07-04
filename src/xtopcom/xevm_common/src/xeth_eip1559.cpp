#include "xevm_common/xeth/xeth_eip1559.h"

#include "xevm_common/xeth/xeth_config.h"
#include "xevm_common/xeth/xeth_gaslimit.h"

NS_BEG3(top, evm_common, eth)

constexpr uint64_t GasLimitBoundDivisor = 1024;
constexpr uint64_t ElasticityMultiplier = 2;
constexpr uint64_t InitialBaseFee = 1000000000;
constexpr uint64_t BaseFeeChangeDenominator = 8;

static bigint calc_baseFee(const eth::xeth_block_header_t & parentHeader) {
    if (!eth::config::is_london(parentHeader.number())) {
        return bigint(InitialBaseFee);
    }
    auto parentGasTarget = parentHeader.gasLimit() / ElasticityMultiplier;
    bigint parentGasTargetBig = parentGasTarget;
    bigint baseFeeChangeDenominator = BaseFeeChangeDenominator;
    // If the parent gasUsed is the same as the target, the baseFee remains unchanged.
    if (parentHeader.gasUsed() == parentGasTarget) {
        return bigint(parentHeader.baseFee());
    }
    if (parentHeader.gasUsed() > parentGasTarget) {
        bigint gasUsedDelta = bigint(parentHeader.gasUsed() - parentGasTarget);
        bigint x = parentHeader.baseFee() * gasUsedDelta;
        bigint y = x / parentGasTargetBig;
        bigint baseFeeDelta = y / baseFeeChangeDenominator;
        if (baseFeeDelta < 1) {
            baseFeeDelta = 1;
        }
        return parentHeader.baseFee() + baseFeeDelta;
    } else {
        // Otherwise if the parent block used less gas than its target, the baseFee should decrease.
        bigint gasUsedDelta = bigint(parentGasTarget - parentHeader.gasUsed());
        bigint x = parentHeader.baseFee() * gasUsedDelta;
        bigint y = x / parentGasTargetBig;
        bigint baseFeeDelta = y / baseFeeChangeDenominator;
        x = parentHeader.baseFee() - baseFeeDelta;
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
bool verify_eip1559_header(const eth::xeth_header_t & parentHeader, const eth::xeth_header_t & header) {
    // Verify that the gas limit remains within allowed bounds
    auto parentGasLimit = parentHeader.gasLimit();
    if (!eth::config::is_london(parentHeader.number())) {
        parentGasLimit = parentHeader.gasLimit() * ElasticityMultiplier;
    }
    if (!eth::verify_gaslimit(parentGasLimit, header.gasLimit())) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] gaslimit mismatch, new: %lu, old: %lu", header.gasLimit(), parentHeader.gasLimit());
        return false;
    }
    // Verify the baseFee is correct based on the parent header.
    auto expectedBaseFee = calc_baseFee(parentHeader);
    if (header.baseFee() != expectedBaseFee) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] wrong basefee: %s, should be: %s", header.baseFee().str().c_str(), expectedBaseFee.str().c_str());
        return false;
    }
    return true;
}

NS_END3
