#include "xevm_common/xcrosschain/xheco_eip1559.h"

#include "xevm_common/common.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_common/xcrosschain/xheco_gaslimit.h"

NS_BEG3(top, evm_common, heco)

static bigint calc_baseFee(const xeth_header_t & parentHeader) {
    return bigint(0);
}

bool verify_eip1559_header(const xeth_header_t & parent, const xeth_header_t & header) {
    auto parentGasLimit = parent.gas_limit;
    if (!heco::verify_gaslimit(parentGasLimit, header.gas_limit)) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] gaslimit mismatch, new: %lu, old: %lu", header.gas_limit, parent.gas_limit);
        return false;
    }
    auto expectedBaseFee = calc_baseFee(parent);
    if (header.base_fee.value() != expectedBaseFee) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] wrong basefee: %s, should be: %s", header.base_fee.value().str().c_str(), expectedBaseFee.str().c_str());
        return false;
    }
    return true;
}

NS_END3
