// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xproperties/xbasic_property.h"
#include "xcontract_common/xbasic_contract.h"
#include "xcontract_common/xcontract_state.h"

#include <map>

NS_BEG3(top, contract_common, properties)

// template<typename KEYT, typename VALUET, typename = typename std::enable_if<(std::is_same<KEYT, std::string>::value ||
//                                                                              std::is_base_of<xenable_to_string_t<KEYT>, KEYT>::value
//                                                                              ) && (
//                                                                               std::is_base_of<xenable_to_string_t<VALUET>, VALUET>::value ||
//                                                                               std::is_base_of<xtop_basic_property, VALUET>::value ||
//                                                                               std::is_same<VALUET, std::string>::value  ||
//                                                                               std::is_same<VALUET, std::int8_t>::value  ||
//                                                                               std::is_same<VALUET, std::int16_t>::value ||
//                                                                               std::is_same<VALUET, std::int32_t>::value ||
//                                                                               std::is_same<VALUET, std::int64_t>::value ||
//                                                                               std::is_same<VALUET, std::uint64_t>::value
//                                                                             )>::type>
// class xtop_map_property;

template<typename KEYT, typename VALUET, typename = typename std::enable_if<std::is_convertible<KEYT, std::string>::value &&
                                                                            std::is_convertible<VALUET, std::string>::value>::type>
class xtop_map_property: public xtop_basic_property {
public:
    xtop_map_property(xtop_map_property const&) = delete;
    xtop_map_property& operator=(xtop_map_property const&) = delete;
    xtop_map_property(xtop_map_property&&) = default;
    xtop_map_property& operator=(xtop_map_property&&) = default;
    ~xtop_map_property() = default;

    explicit xtop_map_property(std::string const& prop_name, contract_common::xbasic_contract_t*  contract)
                                :xbasic_property_t{prop_name, xproperty_type_t::map , make_observer(contract)} {
        m_contract_state->access_control()->map_prop_create<std::string, std::string>(accessor(), m_id);

    }


    void add(KEYT const& prop_key, VALUET const& prop_value) {
        m_contract_state->access_control()->map_prop_add<std::string, std::string>(accessor(), m_id, (std::string)prop_key, (std::string)prop_value);
    }

    void erase(KEYT const& prop_key) {
        m_contract_state->access_control()->map_prop_erase<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
    }

    void update(KEYT const& prop_key,  VALUET const& prop_value) {
        m_contract_state->access_control()->map_prop_update<std::string, std::string>(accessor(), m_id, (std::string)prop_key, (std::string)prop_value);
    }

    void update(std::map<std::string, std::string> const& prop_value) {
        m_contract_state->access_control()->map_prop_update<std::string, std::string>(accessor(), m_id, prop_value);
    }

    void clear() {
        m_contract_state->access_control()->map_prop_clear<std::string, std::string>(accessor(), m_id);
    }

    std::string query(KEYT const& prop_key) {
        return m_contract_state->access_control()->map_prop_query<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
    }
    std::map<std::string, std::string> clone() {
        return m_contract_state->access_control()->map_prop_query<std::string, std::string>(accessor(), m_id);
    }

};

template <typename KEYT, typename VALUET>
using xmap_property_t = xtop_map_property<KEYT, VALUET>;

NS_END3
