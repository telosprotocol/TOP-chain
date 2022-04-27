// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_contract_runtime/xevm_sys_contract_face.h"

NS_BEG4(top, contract_runtime, evm, sys_contract)

// sample code
class xtop_evm_erc20_sys_contract : public xevm_syscontract_face_t {
public:
    xtop_evm_erc20_sys_contract() = default;
    ~xtop_evm_erc20_sys_contract() override = default;
    bool execute(xbytes_t input,
                 uint64_t target_gas,
                 sys_contract_context const & context,
                 bool is_static,
                 sys_contract_precompile_output & output,
                 sys_contract_precompile_error & err) override {
        assert(false);

        output.exit_status = precompile_output_ExitSucceed::Stopped;
        output.cost = 100;
        output.output = xvariant_bytes("12345678", true).to_bytes();
        // output.logs = ...
        return true;

        // ============================

        err.fail_status = precompile_error::Revert;
        err.cost = 100;
        err.minor_status = static_cast<uint32_t>(precompile_error_ExitFatal::NotSupported);
        err.output = xvariant_bytes("12345678", true).to_bytes();
        return false;
    }
};

NS_END4