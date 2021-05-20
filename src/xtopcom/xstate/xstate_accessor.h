// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xstate/xaccess_control_data.h"
#include "xstate/xerror/xerror.h"
#include "xstate/xproperties/xproperty_identifier.h"
#include "xstate/xtoken.h"
#include "xvledger/xvcanvas.h"
#include "xvledger/xvstate.h"

#include <memory>
#include <string>
#include <system_error>
#include <type_traits>

namespace top {
namespace state {

enum class xenum_integer_property_update_operation {
    invalid,
    increase,
    decrease
};
using xinteger_property_update_operation_t = xenum_integer_property_update_operation;

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

    /// @brief Update property.
    /// @param proprty_id Property ID.
    /// @param value New value.
    /// @param ec Log the error code in the operation.
    template <properties::xproperty_type_t PropertyTypeV>
    void update_property(properties::xtyped_property_identifier_t<PropertyTypeV> const & proprty_id, typename properties::xelement_type_of_t<PropertyTypeV>::type const & value, std::error_code & ec);

    template <properties::xproperty_type_t PropertyTypeV, typename std::enable_if<PropertyTypeV == properties::xproperty_type_t::uint8  ||
                                                                                  PropertyTypeV == properties::xproperty_type_t::uint16 ||
                                                                                  PropertyTypeV == properties::xproperty_type_t::uint32 ||
                                                                                  PropertyTypeV == properties::xproperty_type_t::uint64>::type * = nullptr>
    void update_integer_property(properties::xtyped_property_identifier_t<PropertyTypeV> const & property_id, typename properties::xtype_of_t<PropertyTypeV>::type const change_amount, xinteger_property_update_operation_t const op, std::error_code & ec) {
        switch (op) {
        case xinteger_property_update_operation_t::increase:
            increase_int_property<PropertyTypeV>(property_id, change_amount, ec);
            break;

        case xinteger_property_update_operation_t::decrease:
            decrease_int_property<PropertyTypeV>(property_id, change_amount, ec);
            break;

        default:
            assert(false);
            break;
        }
    }

    /// @brief Get property.
    /// @param property_id Property ID.
    /// @param ec Log the error code in the operation.
    /// @return Property value.
    template <properties::xproperty_type_t PropertyTypeV>
    typename properties::xtype_of_t<PropertyTypeV>::type get_property(properties::xtyped_property_identifier_t<PropertyTypeV> const & property_id, std::error_code & ec) const;

    std::string src_code(properties::xproperty_identifier_t const & prop_id, std::error_code & ec) const;
    std::string src_code(properties::xproperty_identifier_t const & prop_id) const;

    void deploy_src_code(properties::xproperty_identifier_t const & prop_id, std::string src_code, std::error_code & ec);
    void deploy_src_code(properties::xproperty_identifier_t const & prop_id, std::string src_code);

    xbyte_buffer_t bin_code(properties::xproperty_identifier_t const & prop_id, std::error_code & ec) const;
    xbyte_buffer_t bin_code(properties::xproperty_identifier_t const & prop_id) const;

    void deploy_bin_code(properties::xproperty_identifier_t const & prop_id, xbyte_buffer_t bin_code, std::error_code & ec);
    void deploy_bin_code(properties::xproperty_identifier_t const & prop_id, xbyte_buffer_t bin_code);

    bool property_exist(common::xaccount_address_t const & user, properties::xproperty_identifier_t const & prop_id, std::error_code & ec) const;
    bool property_exist(common::xaccount_address_t const & user, properties::xproperty_identifier_t const & prop_id) const;

    bool system_property(properties::xproperty_identifier_t const & property_id) const;

    common::xaccount_address_t address() const;

    uint64_t blockchain_height() const;

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

    /// @brief Increase property of integer type.
    /// @param property_id Property ID.
    /// @param change_amount Change amount.
    /// @param ec Log error in the process.
    template <properties::xproperty_type_t PropertyTypeV>
    void increase_int_property(properties::xtyped_property_identifier_t<PropertyTypeV> const & property_id, typename std::make_unsigned<typename properties::xtype_of_t<PropertyTypeV>::type>::type const change_amount, std::error_code & ec);

    /// @brief Decrease property of integer type.
    /// @param property_id Property ID.
    /// @param change_amount Change amount.
    /// @param ec Log error in the process.
    template <properties::xproperty_type_t PropertyTypeV>
    void decrease_int_property(properties::xtyped_property_identifier_t<PropertyTypeV> const & property_id, typename std::make_unsigned<typename properties::xtype_of_t<PropertyTypeV>::type>::type const change_amount, std::error_code & ec);
};
using xstate_accessor_t = xtop_state_accessor;

}
}

namespace top {
namespace state {

template <>
void xstate_accessor_t::do_create_int_property<properties::xproperty_type_t::uint64>(std::string const & property_name, std::error_code & ec);

template <>
void xstate_accessor_t::update_property<properties::xproperty_type_t::string>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::string> const & proprty_id,
                                                                              properties::xelement_type_of_t<properties::xproperty_type_t::string>::type const & value,
                                                                              std::error_code & ec);

template <>
void xstate_accessor_t::update_property<properties::xproperty_type_t::map>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::map> const & proprty_id,
                                                                           properties::xelement_type_of_t<properties::xproperty_type_t::map>::type const & value,
                                                                           std::error_code & ec);

#define DECLARE_INCREASE_INT_PROPERTY(INT_TYPE)\
    template <>\
    void xstate_accessor_t::increase_int_property<properties::xproperty_type_t::INT_TYPE>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::INT_TYPE> const & property_id, std::make_unsigned<properties::xtype_of_t<properties::xproperty_type_t::INT_TYPE>::type>::type const change_amount, std::error_code & ec)

DECLARE_INCREASE_INT_PROPERTY(uint64);

#undef DECLARE_INCREASE_INT_PROPERTY

#define DECLARE_DECREASE_INT_PROPERTY(INT_TYPE)\
    template <>\
    void xstate_accessor_t::increase_int_property<properties::xproperty_type_t::INT_TYPE>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::INT_TYPE> const & property_id, std::make_unsigned<properties::xtype_of_t<properties::xproperty_type_t::INT_TYPE>::type>::type const change_amount, std::error_code & ec)

DECLARE_DECREASE_INT_PROPERTY(uint64);

#undef DECLARE_DECREASE_INT_PROPERTY

template <>
properties::xtype_of_t<properties::xproperty_type_t::string>::type xstate_accessor_t::get_property<properties::xproperty_type_t::string>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::string> const & proprty_id, std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::map>::type xstate_accessor_t::get_property<properties::xproperty_type_t::map>(properties::xtyped_property_identifier_t<properties::xproperty_type_t::map> const & proprty_id, std::error_code & ec) const;

}
}
