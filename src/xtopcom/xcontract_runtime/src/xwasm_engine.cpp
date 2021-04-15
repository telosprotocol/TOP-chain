// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xbase.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xcontract_runtime/xuser/xwasm/xwasm_engine.h"

extern "C" bool validate_wasm_with_content(uint8_t *s, uint32_t size);

NS_BEG3(top, contract_runtime, user)

void xtop_wasm_engine::deploy_contract(xbyte_buffer_t const& code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
    if (!validate_wasm_with_content((uint8_t*)code.data(), code.size())) {
        throw error::xcontract_runtime_error_t{error::xenum_errc::enum_wasm_code_invalid, "invalid wasm code"};
    }
    exe_ctx->contract_state()->deploy_src_code(std::string{(char*)code.data(), code.size()});

}

NS_END3