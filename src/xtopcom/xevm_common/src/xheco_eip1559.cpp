#include "xevm_common/xcrosschain/xheco_eip1559.h"

#include "xcommon/common.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_common/xcrosschain/xheco_gaslimit.h"

#include <cinttypes>

NS_BEG3(top, evm_common, heco)
static uint64_t calc_base_fee(xeth_header_t const &) {
    return 0;
}

bool verify_eip1559_header(xeth_header_t const & parent_header, xeth_header_t const & header) {
    assert(parent_header.base_fee_per_gas.has_value());

    auto const & parent_gas_limit = parent_header.gas_limit;
    if (!heco::verify_gaslimit(parent_gas_limit, header.gas_limit)) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] gaslimit mismatch, new: %s, old: %s", header.gas_limit.str().c_str(), parent_header.gas_limit.str().c_str());
        return false;
    }
    auto const expected_base_fee = calc_base_fee(parent_header);
    if (header.base_fee_per_gas.value() != expected_base_fee) {
        xwarn("[xtop_evm_eth_bridge_contract::verifyEip1559Header] wrong basefee: %" PRIu64 ", should be: %" PRIu64, header.base_fee_per_gas.value(), expected_base_fee);
        return false;
    }
    return true;
}

NS_END3
