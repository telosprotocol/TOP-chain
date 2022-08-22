#include "xevm_common/xcrosschain/xeth_gaslimit.h"

#include "xevm_common/xcrosschain/xeth_header.h"

NS_BEG3(top, evm_common, eth)

constexpr uint64_t GasLimitBoundDivisor = 1024;

// VerifyGaslimit verifies the header gas limit according increase/decrease
// in relation to the parent gas limit.
bool verify_gaslimit(const evm_common::u256 parent_gas_limit, const evm_common::u256 header_gas_limit) {
    // Verify that the gas limit remains within allowed bounds
    bigint diff = bigint(parent_gas_limit) - bigint(header_gas_limit);
    if (diff < 0) {
        diff *= -1;
    }
    bigint limit = parent_gas_limit / GasLimitBoundDivisor;
    if (uint64_t(diff) >= limit) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyGaslimit] diff: %lu >= limit: %s", uint64_t(diff), limit.str().c_str());
        return false;
    }

    if (header_gas_limit < 5000) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyGaslimit] headerGasLimit: %s too small < 5000", header_gas_limit.str().c_str());
        return false;
    }
    return true;
}

NS_END3
