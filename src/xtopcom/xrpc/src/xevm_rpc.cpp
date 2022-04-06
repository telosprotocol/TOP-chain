// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xrpc/xevm/xevm_rpc.h"

#include <cassert>

NS_BEG3(top, rpc, evm)

void xtop_evm_rpc::start() {
    // m_evm_json_rpc_object = RunJsonRpc({"0x538", 5}, {"0x538", 5}, {"127.0.0.1:37399", 15}, {"x", 1}, {"192.168.30.22:37389", 19});
    // assert(false);
    running(true);
}

void xtop_evm_rpc::stop() {
    running(false);
    // auto r = StopJsonRpc(m_evm_json_rpc_object);
    // if (r) {
    //     assert(false);
    // }
}

NS_END3
