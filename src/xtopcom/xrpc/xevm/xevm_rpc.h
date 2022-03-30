// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xrunnable.h"
#include "xrpc/xevm/libjsonrpc.h"

#include <memory>

NS_BEG3(top, rpc, evm)

class xtop_evm_rpc : public xbasic_runnable_t<xtop_evm_rpc> {
private:
    void * m_evm_json_rpc_object{nullptr};

public:
    xtop_evm_rpc() = default;
    xtop_evm_rpc(xtop_evm_rpc const &) = delete;
    xtop_evm_rpc & operator=(xtop_evm_rpc const &) = delete;
    xtop_evm_rpc(xtop_evm_rpc &&) = default;
    xtop_evm_rpc & operator=(xtop_evm_rpc &&) = default;
    ~xtop_evm_rpc() override = default;

    void start() override;
    void stop() override;
};
using xevm_rpc_t = xtop_evm_rpc;

NS_END3
