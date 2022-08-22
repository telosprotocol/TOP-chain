#include "xevm_common/xcrosschain/xheco_gaslimit.h"

#include "xbase/xbase.h"
#include "xevm_common/common.h"

NS_BEG3(top, evm_common, heco)

constexpr uint64_t GasLimitBoundDivisor = 1024;
constexpr uint64_t MinGasLimit = 5000;

bool verify_gaslimit(const u256 parent_gas_limit, const u256 header_gas_limit) {
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

    if (header_gas_limit < MinGasLimit) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyGaslimit] headerGasLimit: %s too small < %lu", header_gas_limit.str().c_str(), MinGasLimit);
        return false;
    }
    return true;
}

NS_END3
