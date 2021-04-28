#include "xcontract_common/xcontract_api_params.h"
#include "xcontract_common/xproperties/xproperty_identifier.h"

extern "C" {
int32_t c_call(erc20_params * ptr, int32_t * used_gas) {
    printf("[debug]test c_call %p\n", ptr);
    *used_gas += 40;
    return 0;
}

int32_t c_depoly(erc20_params * ptr, int32_t * used_gas) {
    printf("[debug]test c_depoly %p\n", ptr);
    // printf("[debug]%s \n", ptr->code.c_str());
    printf("[debug]%s \n", ptr->token_symbol.c_str());
    printf("[debug]%lu \n", ptr->total_supply);

    auto contract_state = ptr->contract_state;
    auto state_account = contract_state->state_account_address();
    top::contract_common::properties::xproperty_identifier_t src_property_id{
        "src_code", top::contract_common::properties::xproperty_type_t::src_code, top::contract_common::properties::xproperty_category_t::user};
    contract_state->access_control()->code_prop_create(state_account, src_property_id);
    contract_state->access_control()->code_prop_update(state_account, src_property_id, ptr->code);

    top::contract_common::properties::xproperty_identifier_t balances_property_id{
        "map_balances", top::contract_common::properties::xproperty_type_t::map, top::contract_common::properties::xproperty_category_t::user};
    contract_state->access_control()->map_prop_create<std::string, std::string>(state_account, balances_property_id);

    contract_state->access_control()->STR_PROP_CREATE("token_symbol");
    contract_state->access_control()->STR_PROP_UPDATE("token_symbol", ptr->token_symbol);


    printf("used_gas: %p %d\n", used_gas, *used_gas);
    *used_gas += 30;
    return 0;
}
}