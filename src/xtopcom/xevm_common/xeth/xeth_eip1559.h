#pragma once

#include "xevm_common/common.h"
#include "xevm_common/xeth/xeth_header.h"

NS_BEG3(top, evm_common, eth)

bool verify_eip1559_header(const eth::xeth_header_t & parent, const eth::xeth_header_t & header);

NS_END3
