// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xstate_accessor/xaccess_control_data.h"
#include "xstate_accessor/xerror/xerror.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"
#include "xstate_accessor/xtoken.h"
#include "xvledger/xvcanvas.h"
#include "xvledger/xvstate.h"

#include <memory>
#include <string>
#include <system_error>
#include <type_traits>

namespace top {
namespace state_accessor {

class xtop_state_accessor {
public:
    static constexpr size_t property_name_max_length{ 17 };
    static constexpr size_t property_name_min_length{ 2 };

private:
    top::observer_ptr<top::base::xvbstate_t> bstate_;
    top::xobject_ptr_t<top::base::xvcanvas_t> canvas_;
    xstate_access_control_data_t ac_data_;

public:
    xtop_state_accessor(xtop_state_accessor const &) = delete;
    xtop_state_accessor & operator=(xtop_state_accessor const &) = delete;
    xtop_state_accessor(xtop_state_accessor &&) = default;
    xtop_state_accessor & operator=(xtop_state_accessor &&) = default;
    ~xtop_state_accessor() = default;

    explicit xtop_state_accessor(top::observer_ptr<top::base::xvbstate_t> bstate, xstate_access_control_data_t ac_data);

    uint64_t nonce(properties::xproperty_identifier_t const & property_id, std::error_code & ec) const;

    /// @brief Withdraw token.
    /// @param property_id Property ID.
    /// @param symbol Token symbol.
    /// @param amount Amount to withdraw.
    /// @param ec Log the error code in the operation.
    /// @return Amount of token withdrew.
    xtoken_t withdraw(properties::xproperty_identifier_t const & property_id, std::string const & symbol, uint64_t amount, std::error_code & ec);

    /// @brief Deposit token.
    /// @param property_id Property ID.
    /// @param symbol Token symblol.
    /// @param amount Amount to deposit.
    /// @param ec Log the error code in the operation.
    void deposit(properties::xproperty_identifier_t const & property_id, std::string const & symbol, xtoken_t & amount, std::error_code & ec);

    /// @brief Get balance.
    /// @param property_id Name of balance property.
    /// @param symbol Token symbol.
    /// @param ec Log the error code in the operation.
    /// @return The balance.
    uint64_t balance(properties::xproperty_identifier_t const & property_id, std::string const & symbol, std::error_code & ec) const;

    /// @brief Create property.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    void create_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec);

    /// @brief Get property.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    /// @return Property value.
    template <properties::xproperty_type_t PropertyTypeV>
    typename properties::xtype_of_t<PropertyTypeV>::type get_property(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const;

    /// @brief Set property.
    /// @param property_id Property ID.
    /// @param value Value to be set.
    /// @param ec Log the error code in the operation.
    template <properties::xproperty_type_t PropertyTypeV>
    void set_property(properties::xtypeless_property_identifier_t const & property_id, typename properties::xtype_of_t<PropertyTypeV>::type const & value, std::error_code & ec);

    /// @brief Clear property. This operation liks STL container's clear() API.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    void clear_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec);

    /// @brief Update property cell value. Only map and deque are supported.
    /// @param proprty_id Property ID.
    /// @param key Cell position key.
    /// @param value Cell's new value.
    /// @param ec Log the error code in the operation.
    template <properties::xproperty_type_t PropertyTypeV, typename std::enable_if<PropertyTypeV == properties::xproperty_type_t::map ||
                                                                                  PropertyTypeV == properties::xproperty_type_t::deque>::type * = nullptr>
    void set_property_cell_value(properties::xtypeless_property_identifier_t const & proprty_id,
                                 typename properties::xkey_type_of_t<PropertyTypeV>::type const & key,
                                 typename properties::xvalue_type_of_t<PropertyTypeV>::type const & value,
                                 std::error_code & ec);

    /// @brief Get property cell value. Only map and deque are supported.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @param ec Log the error code in the operation.
    /// @return Cell value.
    template <properties::xproperty_type_t PropertyTypeV, typename std::enable_if<PropertyTypeV == properties::xproperty_type_t::map ||
                                                                                  PropertyTypeV == properties::xproperty_type_t::deque>::type * = nullptr>
    typename properties::xvalue_type_of_t<PropertyTypeV>::type get_property_cell_value(properties::xtypeless_property_identifier_t const & property_id,
                                                                                       typename properties::xkey_type_of_t<PropertyTypeV>::type const & key,
                                                                                       std::error_code & ec) const;

    /// @brief Remove property cell at the position key. Only map and deque are supported.
    /// @param property_id Property ID.
    /// @param key Cell position key.
    /// @param ec Log the error code in the operation.
    template <properties::xproperty_type_t PropertyTypeV, typename std::enable_if<PropertyTypeV == properties::xproperty_type_t::map ||
                                                                                  PropertyTypeV == properties::xproperty_type_t::deque>::type * = nullptr>
    void remove_property_cell(properties::xtypeless_property_identifier_t const & property_id, typename properties::xkey_type_of_t<PropertyTypeV>::type const & key, std::error_code & ec);

    /// @brief Get property size.
    /// @param property_id Property ID.
    /// @param ec Log error code in the operation.
    /// @return Property size.
    size_t property_size(properties::xproperty_identifier_t const & property_id, std::error_code & ec) const;

    /// @brief Get byte-code.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    /// @return Byte-code.
    xbyte_buffer_t bin_code(properties::xproperty_identifier_t const & property_id, std::error_code & ec) const;

    /// @brief Get byte-code. Throw exception when error occurs.
    /// @param property_id Property ID.
    /// @return Byte-code.
    xbyte_buffer_t bin_code(properties::xproperty_identifier_t const & property_id) const;

    /// @brief Deploy byte-code.
    /// @param property_id
    /// @param bin_code Byte-code.
    /// @param ec Log the error code in the operation.
    void deploy_bin_code(properties::xproperty_identifier_t const & property_id, xbyte_buffer_t const & bin_code, std::error_code & ec);

    /// @brief Deploy byte-code. Throw exception when error occurs.
    /// @param property_id Property ID.
    /// @param bin_code Byte-code.
    void deploy_bin_code(properties::xproperty_identifier_t const & property_id, xbyte_buffer_t const & bin_code);

    /// @brief Check if the property identified by the property ID exists or not.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    /// @return 'true' if property exists; otherwise 'false'.
    bool property_exist(properties::xproperty_identifier_t const & property_id, std::error_code & ec) const;

    /// @brief Check if the property identified by the property ID exists or not. Throw exception when error occurs.
    /// @param property_id property_id Property ID.
    /// @return 'true' if property exists; otherwise 'false'.
    bool property_exist(properties::xproperty_identifier_t const & property_id) const;

    /// @brief Get state associated account address.
    /// @return The state associated account address.
    common::xaccount_address_t account_address() const;

    /// @brief Get the state associated chain height.
    /// @return The state associated chain height.
    uint64_t state_height() const;

private:
    bool read_permitted(properties::xproperty_identifier_t const & property_id) const noexcept;
    bool write_permitted(properties::xproperty_identifier_t const & property_id) const noexcept;
    bool read_permitted(std::string const & property_full_name) const noexcept;

    /// @brief Create string property.
    /// @param property_name Property name.
    /// @param ec Log error in the process.
    void do_create_string_property(std::string const & property_name, std::error_code & ec);

    /// @brief Create map property.
    /// @param property_name Property name.
    /// @param ec Log error in the process.
    void do_create_map_property(std::string const & property_name, std::error_code & ec);

    /// @brief Create integer peroperty.
    /// @param property_name Property name.
    /// @param ec Log error in the process.
    template <properties::xproperty_type_t PropertyTypeV>
    void do_create_int_property(std::string const & property_name, std::error_code & ec);
};
using xstate_accessor_t = xtop_state_accessor;

}
}

#include "xstate_accessor/xstate_accessor_impl.h"
