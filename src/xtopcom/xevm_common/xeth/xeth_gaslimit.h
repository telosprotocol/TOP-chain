#pragma once

#include "xevm_common/common.h"

NS_BEG3(top, evm_common, eth)

bool verify_gaslimit(const evm_common::u256 parent_gas_limit, const evm_common::u256 header_gas_limit);

NS_END3
