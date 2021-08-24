// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcontract_common/xproperties/xproperty_identifier.h"
#include "xcontract_common/xproperties/xproperty_type.h"

#include <unordered_map>
#include <unordered_set>

NS_BEG3(top, contract_common, properties)

/**
 * JSON CONFIG EXAMPLE
 *
 *
 *
 *   {
 *
 *
 *          //  every item matches to a contract property object, xproperty_permission_item_t{prop_name, owner, level}
 *          {"top", "", 10},   // balance property
 *          {"nonce", "", 10}, // nonce property
 *
 *
 *          {"prop_1", "", 11}
 *
 *
 *   }
 *
 *
 *
 */

class xtop_property_access_control_data {
// public:
//     xtop_property_access_control_data(xtop_property_access_control_data const&) = default;
//     xtop_property_access_control_data& operator=(xtop_property_access_control_data const&) = default;
//     xtop_property_access_control_data(xtop_property_access_control_data &&) = default;
//     xtop_property_access_control_data& operator=(xtop_property_access_control_data &&) = default;

//     xtop_property_access_control_data(std::string const& base_name, xproperty_type_t type, xproperty_category_t category, common::xaccount_address_t const& owner);


// private:
//     std::string                m_base_name;
//     xproperty_type_t           m_type;
//     xproperty_category_t       m_category;
//     common::xaccount_address_t m_owner;

    std::unordered_map<xproperty_category_t, std::unordered_map<xproperty_type_t, std::unordered_set<common::xaccount_address_t>>> m_data;

public:
    void add(xproperty_category_t category, xproperty_type_t type, common::xaccount_address_t address);
    void remove(xproperty_category_t category, xproperty_type_t type, common::xaccount_address_t address);
    std::unordered_set<common::xaccount_address_t> control_list(xproperty_category_t category, xproperty_type_t type) const noexcept;
};
using xproperty_access_control_data_t = xtop_property_access_control_data;

NS_END3
