// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xuser/xwasm/xwasm_engine.h"

#include "xbase/xbase.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xerror/xerror.h"

#if defined(BUILD_RUSTVM)
extern "C" bool validate_wasm_with_content(uint8_t *s, uint32_t size);
#endif

NS_BEG3(top, contract_runtime, user)

void xtop_wasm_engine::deploy_contract(xbyte_buffer_t const& code, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
#if defined(BUILD_RUSTVM)
    if (!validate_wasm_with_content((uint8_t*)code.data(), code.size())) {
        std::error_code ec{ error::xenum_errc::enum_wasm_code_invalid };
        top::error::throw_error(ec, "invalid wasm code");
    }
#endif
    state_accessor::properties::xproperty_identifier_t bytes_code_property_id{"code", state_accessor::properties::xproperty_type_t::bin_code, state_accessor::properties::xproperty_category_t::user};
    exe_ctx->contract_state()->deploy_bin_code(bytes_code_property_id, code);
}

void xtop_wasm_engine::call_contract(std::string const& func_name, std::vector<xbyte_buffer_t> const& params, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
}

void xtop_wasm_engine::deploy_contract_erc20(std::vector<xbyte_buffer_t> const& params, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
#if defined(BUILD_RUSTVM)
    if (!validate_wasm_with_content((uint8_t*)params[0].data(), params[0].size())) {
        std::error_code ec{ error::xenum_errc::enum_wasm_code_invalid };
        top::error::throw_error(ec, "invalid wasm code");
    }

    assert(params.size() == 3); // 0: the code, 1: erc20 symbal, 2: total supply
    erc20_params params_ptr{
        exe_ctx->contract_state(),
        std::string{params[0].data(),  params[0].data() +  params[0].size()},
        std::string{params[1].data(),  params[1].data() +  params[1].size()},
        std::string{params[2].data(),  params[2].data() +  params[2].size()},
    };
    Erc20_Instance * ins_ptr = get_erc20_instance((uint8_t*)params[0].data(),params[0].size());

    set_gas_left(ins_ptr, 1000);
    auto result = depoly_erc20(ins_ptr, &params_ptr);
    std::cout << "result" << result << "\n";

    return;

    // auto contract_state = exe_ctx->contract_state();
    // auto state_account = contract_state->state_account_address();
    // contract_common::properties::xproperty_identifier_t src_property_id{"src_code", contract_common::properties::xproperty_type_t::src_code, contract_common::properties::xproperty_category_t::user};
    // contract_state->access_control()->code_prop_create(state_account, src_property_id);
    // contract_state->access_control()->code_prop_update(state_account, src_property_id, std::string{params[0].data(), params[0].data() + params[0].size()});
    // contract_common::properties::xproperty_identifier_t balances_property_id{"map_balances", contract_common::properties::xproperty_type_t::map, contract_common::properties::xproperty_category_t::user};
    // contract_state->access_control()->map_prop_create<std::string, std::string>(state_account, balances_property_id);
    // assert(params.size() == 3); // 1: erc20 symbal, 2: total supply [0: the code]

    // // contract_common::properties::xproperty_identifier_t balances_property_id{std::string{params[0].data(), params[0].size()}, contract_common::properties::xproperty_type_t::map, contract_common::properties::xproperty_category_t::user};
    // contract_state->access_control()->STR_PROP_CREATE(std::string{params[0].data(), params[0].data() + params[0].size()});
    // contract_state->access_control()->map_prop_add<std::string, std::string>(state_account, src_property_id, state_account.value(), std::string{params[1].data(), params[1].data() + params[1].size()});
#endif
}


void xtop_wasm_engine::call_contract_erc20(std::vector<xbyte_buffer_t> const&  params, observer_ptr<contract_common::xcontract_execution_context_t> exe_ctx) {
#if defined(BUILD_RUSTVM)
    erc20_params params_ptr{
        exe_ctx->contract_state(),
        params,
    };
    Erc20_Instance * ins_ptr = get_erc20_instance((uint8_t*)erc20_code.data(), erc20_code.size());

    set_gas_left(ins_ptr, 1000);
    auto result = call_erc20(ins_ptr, &params_ptr);
    std::cout << "result" << result << "\n";
#endif
}

NS_END3
