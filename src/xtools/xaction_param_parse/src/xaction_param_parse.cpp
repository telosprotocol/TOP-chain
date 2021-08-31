#include "../xaction_param_parse.h"

#include <iostream>
#include "xdata/xaction.h"
#include "xdata/xproperty.h"

void xaction_param_parse_tool::parse_action_param(std::string const& action_str, uint32_t action_type) {
    using namespace top::data;

    auto action_param = top::data::hex_to_uint(action_str);
    top::base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t*)action_param.data(), (uint32_t)action_param.size());

    switch (action_type)
    {
    case enum_xaction_type::xaction_type_asset_in:
    case enum_xaction_type::xaction_type_asset_out:
        {
            top::data::xproperty_asset asset{0};
            stream >> asset.m_token_name;
            stream >>  asset.m_amount;
            if (asset.m_token_name.empty()) asset.m_token_name = XPROPERTY_ASSET_TOP;
            std::cout << "asset name: " << asset.m_token_name << "\n" << "amount: " << asset.m_amount << "\n";
        }
        break;

    default:
        std::cout << "action type not support yet\n";
        break;
    }
}
