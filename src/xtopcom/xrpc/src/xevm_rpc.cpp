// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xrpc/xevm/xevm_rpc.h"

#include <cassert>

NS_BEG3(top, rpc, evm)

void xtop_evm_rpc::start() {
    // RunJsonRpc();
    running(true);
}

void xtop_evm_rpc::stop() {
    running(false);
}

NS_END3
