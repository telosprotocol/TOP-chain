// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once


#include <string>
#include <cstdint>
#include <memory>

#include "xcontract_runtime/xuser/xuser_engine.h"
#include "xcontract_runtime/xuser/xwasm/xwasm_rustvm_extern_api.h"

NS_BEG3(top, contract_runtime, user)

class xtop_wasm_engine: public xtop_user_engine {
public:
    xtop_wasm_engine() = default;
    xtop_wasm_engine(xtop_wasm_engine const&) = delete;
    xtop_wasm_engine& operator=(xtop_wasm_engine const&) = delete;
    xtop_wasm_engine(xtop_wasm_engine&&) = default;
    xtop_wasm_engine& operator=(xtop_wasm_engine&&) = default;
    ~xtop_wasm_engine() =  default;

    virtual void deploy_contract(xbyte_buffer_t const& code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) override;
    virtual void call_contract(std::string const& func_name, std::vector<xbyte_buffer_t> const& params, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) override;

    virtual void deploy_contract_erc20(std::vector<xbyte_buffer_t> const& params, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx);
    virtual void call_contract_erc20(std::vector<xbyte_buffer_t> const& params, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx);
};
using xwasm_engine_t = xtop_wasm_engine;

NS_END3
