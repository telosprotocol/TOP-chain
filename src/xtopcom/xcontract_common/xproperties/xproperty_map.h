// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xstring.h"
#include "xcontract_common/xcontract_face.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_common/xproperties/xbasic_property.h"

#include <map>

NS_BEG3(top, contract_common, properties)

template <typename KeyT, typename ValueT>
class xtop_map_property : public xbasic_property_t {
public:
    xtop_map_property(xtop_map_property const&) = delete;
    xtop_map_property& operator=(xtop_map_property const&) = delete;
    xtop_map_property(xtop_map_property&&) = default;
    xtop_map_property& operator=(xtop_map_property&&) = default;
    ~xtop_map_property() override = default;

    explicit xtop_map_property(std::string const& name, xcontract_face_t * contract)
        : xbasic_property_t{name, state_accessor::properties::xproperty_type_t::map , make_observer(contract)} {
    }

    void add(KeyT const & key, ValueT const & value, std::error_code & ec) {
        m_associated_contract->contract_state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), top::to_string(key), top::to_bytes(value), ec);
    }

    void add(KeyT const & key, ValueT const & value) {
        m_associated_contract->contract_state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), top::to_string(key), top::to_bytes(value));
    }

    void remove(KeyT const & key, std::error_code & ec) {
        // m_associated_contract->state()->access_control()->map_prop_erase<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
    }

    void remove(KeyT const & key) {
        // m_associated_contract->state()->access_control()->map_prop_erase<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
    }

    void set(KeyT const & key, ValueT const & value, std::error_code & ec) {
        m_associated_contract->contract_state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), top::to_string(key), top::to_bytes(value), ec);
    }

    void set(KeyT const & key, ValueT const & value) {
        printf("wens_test, the key : %s, value: %zu\n", top::to_string(key).c_str(), top::to_bytes(value).size());
        m_associated_contract->contract_state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), top::to_string(key), top::to_bytes(value));
    }

    void set(std::map<KeyT, ValueT> const& value) {
        // m_associated_contract->state()->access_control()->map_prop_update<std::string, std::string>(accessor(), m_id, prop_value);
    }

    void clear() {
        // m_associated_contract->state()->access_control()->map_prop_clear<std::string, std::string>(accessor(), m_id);
    }

    bool exist(KeyT const & key, std::error_code & ec) const {
        assert(m_associated_contract != nullptr);
        assert(!ec);

        return m_associated_contract->contract_state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), top::to_string(key), ec);
    }

    bool exist(KeyT const & key) {
        return m_associated_contract->contract_state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), top::to_string(key));
    }

    ValueT get(KeyT const & key, std::error_code & ec) const {
        auto const & bytes = m_associated_contract->contract_state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), top::to_string(key), ec);
        if (ec) {
            return {};
        }

        return from_bytes<ValueT>(bytes, ec);
    }

    ValueT get(KeyT const & key) const {
        return from_bytes<ValueT>(m_associated_contract->contract_state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), top::to_string(key)));
    }

    std::map<KeyT, ValueT> value() {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), m_id);
        // assert(false);
        return {};
    }

    std::map<KeyT, ValueT> clone(common::xaccount_address_t const & contract) {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), contract, m_id);
        assert(false);
        return {};
    }
};

template <typename KeyT, typename ValueT>
using xmap_property_t = xtop_map_property<KeyT, ValueT>;

NS_END3
