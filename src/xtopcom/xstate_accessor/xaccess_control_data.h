// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"

#include <unordered_map>
#include <unordered_set>

NS_BEG2(top, state_accessor)

class xtop_state_access_control_data {
public:
    xtop_state_access_control_data() = default;
    xtop_state_access_control_data(xtop_state_access_control_data const &) = default;
    xtop_state_access_control_data & operator=(xtop_state_access_control_data const &) = default;
    xtop_state_access_control_data(xtop_state_access_control_data &&) = default;
    xtop_state_access_control_data & operator=(xtop_state_access_control_data &&) = default;
    ~xtop_state_access_control_data() = default;

private:
    std::unordered_map<std::string, std::unordered_set<common::xaccount_address_t>> m_data{};

public:
    // void add(xproperty_category_t category, xproperty_type_t type, common::xaccount_address_t address);
    // void remove(xproperty_category_t category, xproperty_type_t type, common::xaccount_address_t address);
    std::unordered_set<common::xaccount_address_t> control_list(properties::xproperty_identifier_t const & property_id) const noexcept;
};
using xstate_access_control_data_t = xtop_state_access_control_data;

NS_END2
