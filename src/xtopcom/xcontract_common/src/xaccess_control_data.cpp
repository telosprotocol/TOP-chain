// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xaccess_control_data.h"

#include "xbasic/xutility.h"

NS_BEG3(top, contract_common, properties)

void xtop_property_access_control_data::add(xproperty_category_t category, xproperty_type_t type, common::xaccount_address_t address) {
    m_data[category][type].insert(std::move(address));
}

void xtop_property_access_control_data::remove(xproperty_category_t category, xproperty_type_t type, common::xaccount_address_t address) {

}

std::unordered_set<common::xaccount_address_t> xtop_property_access_control_data::control_list(xproperty_category_t category, xproperty_type_t type) const noexcept {
    auto const it_category = m_data.find(category);
    if (it_category == std::end(m_data)) {
        return std::unordered_set<common::xaccount_address_t>{};
    }

    auto const & types = top::get<std::unordered_map<xproperty_type_t, std::unordered_set<common::xaccount_address_t>>>(*it_category);
    auto const it_type = types.find(type);
    if (it_type == std::end(types)) {
        return std::unordered_set<common::xaccount_address_t>{};
    }

    return top::get<std::unordered_set<common::xaccount_address_t>>(*it_type);
}


NS_END3
