// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/xcrosschain/xeth_header.h"

NS_BEG3(top, evm_common, eth)

bool verify_eip1559_header(xeth_header_t const & parent, xeth_header_t const & header);

NS_END3
