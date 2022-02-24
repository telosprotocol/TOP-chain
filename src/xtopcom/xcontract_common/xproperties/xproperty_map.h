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
    xtop_map_property() = default;
    xtop_map_property(xtop_map_property const&) = delete;
    xtop_map_property& operator=(xtop_map_property const&) = delete;
    xtop_map_property(xtop_map_property&&) = default;
    xtop_map_property& operator=(xtop_map_property&&) = default;
    ~xtop_map_property() override = default;

    explicit xtop_map_property(std::string const& name, xcontract_face_t * contract)
        : xbasic_property_t{name, state_accessor::properties::xproperty_type_t::map , make_observer(contract)} {
    }

    explicit xtop_map_property(std::string const & name, std::unique_ptr<xcontract_state_t> state_owned) : xbasic_property_t {name, state_accessor::properties::xproperty_type_t::map, std::move(state_owned)} {
    }

    void add(KeyT const & key, ValueT const & value, std::error_code & ec) {
        assert(m_associated_contract != nullptr);
        assert(associated_state() != nullptr);

        associated_state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key), top::to_bytes(value), ec);
    }

    void add(KeyT const & key, ValueT const & value) {
        associated_state()->xcontract_state_t::set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key), top::to_bytes(value));
    }

    void remove(KeyT const & key, std::error_code & ec) {
        associated_state()->remove_property_cell<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key), ec);
    }

    void remove(KeyT const & key) {
        associated_state()->xcontract_state_t::remove_property_cell<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key));
    }

    void set(KeyT const & key, ValueT const & value, std::error_code & ec) {
        associated_state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key), top::to_bytes(value), ec);
    }

    void set(KeyT const & key, ValueT const & value) {
        associated_state()->xcontract_state_t::set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key), top::to_bytes(value));
    }

    void set(std::map<KeyT, ValueT> const& value, std::error_code& ec) {
        associated_state()->set_property<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_bytes(value), ec);
    }

    void set(std::map<KeyT, ValueT> const& value) {
        associated_state()->set_property<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_bytes(value));
    }

    void clear(std::error_code& ec) {
         associated_state()->clear_property(id(), ec);
    }
    void clear() {
         associated_state()->clear_property(id());
    }

    bool exist(KeyT const & key, std::error_code & ec) const {
        assert(m_associated_contract != nullptr);
        assert(!ec);

        assert(associated_state() != nullptr);
        return associated_state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key), ec);
    }

    bool exist(KeyT const & key) const {
        assert(associated_state() != nullptr);
        return associated_state()->xcontract_state_t::exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key));
    }

    ValueT get(KeyT const & key, std::error_code & ec) const {
        assert(associated_state());
        auto const & bytes = associated_state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key), ec);
        if (ec) {
            return {};
        }

        return from_bytes<ValueT>(bytes, ec);
    }

    ValueT get(KeyT const & key) const {
        assert(associated_state());

        auto const & bytes = associated_state()->xcontract_state_t::get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(typeless_id(), top::to_string(key));

        return from_bytes<ValueT>(bytes);
    }

    std::map<KeyT, ValueT> value(std::error_code& ec) const {
        assert(associated_state() != nullptr);

        std::map<KeyT, ValueT> res;
        auto const & tmp = associated_state()->get_property<state_accessor::properties::xproperty_type_t::map>(typeless_id(), ec);

        for (auto const& pair: tmp) {
            res.insert({pair.first, top::from_bytes<ValueT>(pair.second)});
        }
        return res;
    }

    std::map<KeyT, ValueT> value() const {
        assert(associated_state() != nullptr);

        std::map<KeyT, ValueT> res;
        auto const & tmp = associated_state()->xcontract_state_t::get_property<state_accessor::properties::xproperty_type_t::map>(typeless_id());

        for (auto const& pair: tmp) {
            res.insert({pair.first, top::from_bytes<ValueT>(pair.second)});
        }
        return res;
    }

    size_t size() const {
        assert(associated_state() != nullptr);
        return associated_state()->property_size(id());
    }
};

template <typename KeyT, typename ValueT>
using xmap_property_t = xtop_map_property<KeyT, ValueT>;

NS_END3
