#include "xcontract_common/xcontract_api_params.h"
#include "xcontract_common/xproperties/xproperty_identifier.h"

extern "C" {
int32_t c_call(erc20_params * ptr) {
    printf("[debug]test c_call %p\n", ptr);
    printf("[debug]%s \n", ptr->account_from.c_str());
    printf("[debug]%s \n", ptr->account_to.c_str());
    printf("[debug]%d \n", ptr->value);
    return 40;
}

int32_t c_depoly(erc20_params * ptr) {
    auto contract_state = ptr->contract_state;
    auto state_account = contract_state->state_account_address();
    top::contract_common::properties::xproperty_identifier_t src_property_id{
        "src_code", top::contract_common::properties::xproperty_type_t::src_code, top::contract_common::properties::xproperty_category_t::user};
    contract_state->access_control()->code_prop_create(state_account, src_property_id);
    contract_state->access_control()->code_prop_update(state_account, src_property_id, ptr->code);
    top::contract_common::properties::xproperty_identifier_t balances_property_id{
        "map_balances", top::contract_common::properties::xproperty_type_t::map, top::contract_common::properties::xproperty_category_t::user};
    contract_state->access_control()->map_prop_create<std::string, std::string>(state_account, balances_property_id);
    // assert(params.size() == 3);  // 1: erc20 symbal, 2: total supply [0: the code]

    // contract_common::properties::xproperty_identifier_t balances_property_id{std::string{params[0].data(), params[0].size()}, contract_common::properties::xproperty_type_t::map,
    // contract_common::properties::xproperty_category_t::user};
    // contract_state->access_control()->STR_PROP_CREATE(ptr->code});
    // contract_state->access_control()->map_prop_add<std::string, std::string>(
    //     state_account, src_property_id, state_account.value(), std::string{params[1].data(), params[1].data() + params[1].size()});
    return 30;
}
}