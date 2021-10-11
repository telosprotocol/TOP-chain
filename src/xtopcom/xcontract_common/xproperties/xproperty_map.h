// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xenable_to_bytes.h"
#include "xbasic/xenable_to_string.h"
#include "xcontract_common/xproperties/xbasic_property.h"
#include "xcontract_common/xbasic_contract.h"
#include "xcontract_common/xcontract_state.h"

#include <map>

NS_BEG3(top, contract_common, properties)

template <typename KeyT,
          typename ValueT,
          typename Enabled = void>
class xtop_map_property : public xbasic_property_t {
public:
    xtop_map_property(xtop_map_property const&) = delete;
    xtop_map_property& operator=(xtop_map_property const&) = delete;
    xtop_map_property(xtop_map_property&&) = default;
    xtop_map_property& operator=(xtop_map_property&&) = default;
    ~xtop_map_property() override = default;

    explicit xtop_map_property(std::string const& prop_name, contract_common::xbasic_contract_t*  contract)
        : xbasic_property_t{prop_name, state_accessor::properties::xproperty_type_t::map , make_observer(contract)} {
    }

    void create() final {
        std::error_code ec;
        m_associated_contract->state()->create_property(m_id, ec);
        top::error::throw_error(ec);
    }

    void add(KeyT const & key, ValueT const & value, std::error_code & ec) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string(), value.to_bytes(), ec);
    }

    void add(KeyT const & key, ValueT const & value) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string(), value.to_bytes());
    }

    void remove(KeyT const & key, std::error_code & ec) {
        // m_associated_contract->state()->access_control()->map_prop_erase<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
    }

    void remove(KeyT const & key) {
        // m_associated_contract->state()->access_control()->map_prop_erase<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
    }

    void update(KeyT const & key, ValueT const & value, std::error_code & ec) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string(), value.to_bytes(), ec);
    }

    void update(KeyT const & key, ValueT const & value) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string(), value.to_bytes());
    }

    void reset(std::map<std::string, std::string> const& prop_value) {
        // m_associated_contract->state()->access_control()->map_prop_update<std::string, std::string>(accessor(), m_id, prop_value);
    }

    void clear() {
        // m_associated_contract->state()->access_control()->map_prop_clear<std::string, std::string>(accessor(), m_id);
    }

    bool exist(KeyT const & key, std::error_code & ec) const {
        assert(m_associated_contract != nullptr);
        assert(!ec);

        return m_associated_contract->state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(m_id, key, ec);
    }

    bool exist(KeyT const & key) {
        return m_associated_contract->state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(m_id, key);
    }

    ValueT get(KeyT const & key, std::error_code & ec) const {
        auto const & bytes = m_associated_contract->state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string(), ec);
        if (ec) {
            return {};
        }

        return from_bytes<ValueT>(bytes, ec);
    }

    ValueT get(KeyT const & key) const {
        return from_bytes<ValueT>(m_associated_contract->state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string()));
    }

    std::map<std::string, std::string> clone() {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), m_id);
        assert(false);
        return {};
    }

    std::map<std::string, std::string> clone(common::xaccount_address_t const & contract) {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), contract, m_id);
        assert(false);
        return {};
    }
};

template <typename KeyT>
class xtop_map_property<KeyT, xbytes_t, typename std::enable_if<std::is_base_of<xenable_to_string_t<KeyT>, KeyT>::value, KeyT>::type> : public xbasic_property_t {
public:
    xtop_map_property(xtop_map_property const &) = delete;
    xtop_map_property & operator=(xtop_map_property const &) = delete;
    xtop_map_property(xtop_map_property &&) = default;
    xtop_map_property & operator=(xtop_map_property &&) = default;
    ~xtop_map_property() override = default;

    explicit xtop_map_property(std::string const & prop_name, xbasic_contract_t * contract)
      : xbasic_property_t{prop_name, state_accessor::properties::xproperty_type_t::map, make_observer(contract)} {
    }

    void create() final {
        std::error_code ec;
        m_associated_contract->state()->create_property(m_id, ec);
        top::error::throw_error(ec);
    }

    void add(KeyT const & key, xbytes_t value, std::error_code & ec) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string(), std::move(value), ec);
    }

    void add(KeyT const & key, xbytes_t value) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string(), std::move(value));
    }

    void remove(KeyT const & key, std::error_code & ec) {
        // m_associated_contract->state()->access_control()->map_prop_erase<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
        assert(false);
    }

    void remove(KeyT const & key) {
        // m_associated_contract->state()->access_control()->map_prop_erase<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
        assert(false);
    }

    void update(KeyT const & key, xbytes_t value, std::error_code & ec) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string(), std::move(value), ec);
    }

    void update(KeyT const & key, xbytes_t value) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            state_accessor::properties::xtypeless_property_identifier_t{m_id}, key.to_string(), std::move(value));
    }

    void reset(std::map<std::string, std::string> const &) {
        // m_associated_contract->state()->access_control()->map_prop_update<std::string, std::string>(accessor(), m_id, prop_value);
        assert(false);
    }

    void clear() {
        // m_associated_contract->state()->access_control()->map_prop_clear<std::string, std::string>(accessor(), m_id);
        assert(false);
    }

    bool exist(KeyT const & key, std::error_code & ec) const {
        assert(m_associated_contract != nullptr);
        assert(!ec);

        return m_associated_contract->state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(m_id, key, ec);
    }

    bool exist(KeyT const & key) const {
        return m_associated_contract->state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(m_id, key);
    }

    xbytes_t get(KeyT const & key, std::error_code & ec) const {
        auto bytes = m_associated_contract->state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string(), ec);
        if (ec) {
            return {};
        }

        return bytes;
    }

    xbytes_t get(KeyT const & key) const {
        return m_associated_contract->state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key.to_string());
    }

    std::map<std::string, std::string> clone() const {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), m_id);
        assert(false);
        return {};
    }

    std::map<std::string, std::string> clone(common::xaccount_address_t const & contract) const {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), contract, m_id);
        assert(false);
        return {};
    }
};

template <typename ValueT>
class xtop_map_property<std::string, ValueT, typename std::enable_if<std::is_base_of<xenable_to_bytes_t<ValueT>, ValueT>::value, ValueT>::type> : public xbasic_property_t {
public:
    xtop_map_property(xtop_map_property const &) = delete;
    xtop_map_property & operator=(xtop_map_property const &) = delete;
    xtop_map_property(xtop_map_property &&) = default;
    xtop_map_property & operator=(xtop_map_property &&) = default;
    ~xtop_map_property() override = default;

    explicit xtop_map_property(std::string const & prop_name, xbasic_contract_t * contract)
      : xbasic_property_t{prop_name, state_accessor::properties::xproperty_type_t::map, make_observer(contract)} {
    }

    void create() final {
        std::error_code ec;
        m_associated_contract->state()->create_property(m_id, ec);
        top::error::throw_error(ec);
    }

    void add(std::string const & key, ValueT const & value, std::error_code & ec) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key, value.to_bytes(), ec);
    }

    void add(std::string const & key, ValueT const & value) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key, value.to_bytes());
    }

    void remove(std::string const & /*key*/, std::error_code & /*ec*/) {
        assert(false);
    }

    void remove(std::string const & /*key*/) {
        assert(false);
    }

    void update(std::string const & key, ValueT const & value, std::error_code & ec) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key, value.to_bytes(), ec);
    }

    void update(std::string const & key, ValueT const & value) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            state_accessor::properties::xtypeless_property_identifier_t{m_id}, key, value.to_bytes());
    }

    void reset(std::map<std::string, std::string> const &) {
        // m_associated_contract->state()->access_control()->map_prop_update<std::string, std::string>(accessor(), m_id, prop_value);
        // assert(false);
    }

    void clear() {
        // m_associated_contract->state()->access_control()->map_prop_clear<std::string, std::string>(accessor(), m_id);
        assert(false);
    }

    bool exist(std::string const & key, std::error_code & ec) const {
        assert(m_associated_contract != nullptr);
        assert(!ec);

        return m_associated_contract->state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(m_id, key, ec);
    }

    bool exist(std::string const & key) const {
        return m_associated_contract->state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(m_id, key);
    }

    ValueT get(std::string const & key, std::error_code & ec) const {
        auto const & bytes = m_associated_contract->state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key, ec);
        if (ec) {
            return {};
        }

        return from_bytes<ValueT>(bytes, ec);
    }

    ValueT get(std::string const & key) const {
        return from_bytes<ValueT>(m_associated_contract->state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(m_id, key));
    }

    std::map<std::string, std::string> clone() const {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), m_id);
        assert(false);
        return {};
    }

    std::map<std::string, std::string> clone(common::xaccount_address_t const & /*contract*/) const {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), contract, m_id);
        assert(false);
        return {};
    }
};

template <>
class xtop_map_property<std::string, xbytes_t, void> : public xbasic_property_t {
public:
    xtop_map_property(xtop_map_property const &) = delete;
    xtop_map_property & operator=(xtop_map_property const &) = delete;
    xtop_map_property(xtop_map_property &&) = default;
    xtop_map_property & operator=(xtop_map_property &&) = default;
    ~xtop_map_property() override = default;

    explicit xtop_map_property(std::string const & prop_name, xbasic_contract_t * contract)
      : xbasic_property_t{prop_name, state_accessor::properties::xproperty_type_t::map, make_observer(contract)} {
    }

    void create() final {
        std::error_code ec;
        m_associated_contract->state()->create_property(m_id, ec);
        top::error::throw_error(ec);
    }

    void add(std::string const & key, xbytes_t value, std::error_code & ec) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), key, std::move(value), ec);
    }

    void add(std::string const & key, xbytes_t value) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            static_cast<state_accessor::properties::xtypeless_property_identifier_t>(m_id), key, std::move(value));
    }

    void remove(std::string const & key, std::error_code & ec) {
        // m_associated_contract->state()->access_control()->map_prop_erase<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
        assert(false);
    }

    void remove(std::string const & key) {
        // m_associated_contract->state()->access_control()->map_prop_erase<std::string, std::string>(accessor(), m_id, (std::string)prop_key);
        assert(false);
    }

    void update(std::string const & key, xbytes_t value, std::error_code & ec) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            state_accessor::properties::xtypeless_property_identifier_t{m_id}, key, std::move(value), ec);
    }

    void update(std::string const & key, xbytes_t value) {
        m_associated_contract->state()->set_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            state_accessor::properties::xtypeless_property_identifier_t{m_id}, key, std::move(value));
    }

    void reset(std::map<std::string, std::string> const &) {
        // m_associated_contract->state()->access_control()->map_prop_update<std::string, std::string>(accessor(), m_id, prop_value);
        assert(false);
    }

    void clear() {
        // m_associated_contract->state()->access_control()->map_prop_clear<std::string, std::string>(accessor(), m_id);
        assert(false);
    }

    bool exist(std::string const & key, std::error_code & ec) const {
        assert(m_associated_contract != nullptr);
        assert(!ec);

        return m_associated_contract->state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(
            state_accessor::properties::xtypeless_property_identifier_t{m_id}, key, ec);
    }

    bool exist(std::string const & key) {
        return m_associated_contract->state()->exist_property_cell_key<state_accessor::properties::xproperty_type_t::map>(
            state_accessor::properties::xtypeless_property_identifier_t{m_id}, key);
    }

    xbytes_t get(std::string const & key, std::error_code & ec) const {
        auto bytes = m_associated_contract->state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            state_accessor::properties::xtypeless_property_identifier_t{m_id}, key, ec);
        if (ec) {
            return {};
        }

        return bytes;
    }

    xbytes_t get(std::string const & key) const {
        return m_associated_contract->state()->get_property_cell_value<state_accessor::properties::xproperty_type_t::map>(
            state_accessor::properties::xtypeless_property_identifier_t{m_id}, key);
    }

    std::map<std::string, std::string> clone() const {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), m_id);
        assert(false);
        return {};
    }

    std::map<std::string, std::string> clone(common::xaccount_address_t const & contract) const {
        // return m_associated_contract->state()->access_control()->map_prop_query<std::string, std::string>(accessor(), contract, m_id);
        assert(false);
        return {};
    }
};

template <typename KeyT, typename ValueT>
using xmap_property_t = xtop_map_property<KeyT, ValueT>;

NS_END3
