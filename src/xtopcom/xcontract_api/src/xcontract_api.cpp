#include "xcontract_common/xcontract_api_params.h"
#include "xcontract_common/xproperties/xproperty_identifier.h"
#include "xbase/xutl.h"

extern "C" {
int32_t c_call(erc20_params * ptr, int32_t * used_gas) {
    printf("[debug]test c_call %p\n", ptr);

    auto contract_state = ptr->contract_state;
    auto state_account = contract_state->state_account_address();
    // param 0 always is function name
    std::string function_name{(char*)ptr->call_param[0].data(), ptr->call_param[0].size()};
    if (function_name == "balanceof") {
        top::contract_common::properties::xproperty_identifier_t balances_property_id{"map_balances", top::contract_common::properties::xproperty_type_t::map, top::contract_common::properties::xproperty_category_t::user};
        auto balance = contract_state->access_control()->map_prop_query<std::string, std::string>(state_account, balances_property_id, state_account.value());
        std::cout << "balance is: " << balance << "\n";
        *used_gas += 40;
        // return top::base::xstring_utl::toint32(balance);
    }
    printf("[in c_call] one test param is %s\n", std::string((char*)ptr->call_param[1].data(),  ptr->call_param[1].size()).c_str());

    *used_gas += 40;
    return 0;
}

int32_t c_depoly(erc20_params * ptr, int32_t * used_gas) {
    printf("[debug]test c_depoly %p\n", ptr);

    auto contract_state = ptr->contract_state;
    auto state_account = contract_state->state_account_address();
    // code
    top::contract_common::properties::xproperty_identifier_t src_property_id{
        "src_code", top::contract_common::properties::xproperty_type_t::src_code, top::contract_common::properties::xproperty_category_t::user};
    contract_state->access_control()->code_prop_create(state_account, src_property_id);
    contract_state->access_control()->code_prop_update(state_account, src_property_id, ptr->code);
    // balances map & owner balance
    top::contract_common::properties::xproperty_identifier_t balances_property_id{
        "map_balances", top::contract_common::properties::xproperty_type_t::map, top::contract_common::properties::xproperty_category_t::user};
    contract_state->access_control()->map_prop_create<std::string, std::string>(state_account, balances_property_id);
    contract_state->access_control()->map_prop_add<std::string, std::string>(
        state_account, balances_property_id, state_account.value(), ptr->total_supply);
    //symbol
    std::string symbol_property_name = "symbol";
    contract_state->access_control()->STR_PROP_CREATE(symbol_property_name);
    contract_state->access_control()->STR_PROP_UPDATE(symbol_property_name, ptr->symbol);

    std::cout << "in c_deploy:  " << contract_state->access_control()->STR_PROP_QUERY(symbol_property_name) << "\n";


    printf("used_gas: %p %d\n", used_gas, *used_gas);
    *used_gas += 30;
    return 0;
}
}