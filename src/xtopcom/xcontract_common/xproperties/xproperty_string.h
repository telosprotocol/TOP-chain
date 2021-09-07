// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xproperties/xbasic_property.h"
#include "xcontract_common/xbasic_contract.h"
#include "xcontract_common/xcontract_state.h"

#include <string>

NS_BEG3(top, contract_common, properties)

class xtop_string_property: public xtop_basic_property {
public:
    xtop_string_property(xtop_string_property const&) = delete;
    xtop_string_property& operator=(xtop_string_property const&) = delete;
    xtop_string_property(xtop_string_property&&) = default;
    xtop_string_property& operator=(xtop_string_property&&) = default;
    ~xtop_string_property() = default;

    explicit xtop_string_property(std::string const& prop_name, contract_common::xbasic_contract_t*  contract)
                                :xbasic_property_t{prop_name, state_accessor::properties::xproperty_type_t::string , make_observer(contract)} {
        m_contract_state->access_control()->string_prop_create(accessor(), m_id);

    }

    void update(std::string const& prop_value) {
        m_contract_state->access_control()->string_prop_update(accessor(), m_id, prop_value);
    }

    void clear() {
        m_contract_state->access_control()->string_prop_clear(accessor(), m_id);
    }

    std::string query() {
        return m_contract_state->access_control()->string_prop_query(accessor(), m_id);
    }

    std::string query(common::xaccount_address_t const & contract) {
        return m_contract_state->access_control()->string_prop_query(accessor(), contract, m_id);
    }

};

using xstring_property_t = xtop_string_property;

NS_END3
