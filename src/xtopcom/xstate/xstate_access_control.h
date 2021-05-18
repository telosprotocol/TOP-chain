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

class xtop_state_access_control {
private:
    top::observer_ptr<top::base::xvbstate_t> bstate_;
    top::xobject_ptr_t<top::base::xvcanvas_t> canvas_;
    xstate_access_control_data_t ac_data_;

public:
    xtop_state_access_control(xtop_state_access_control const &) = delete;
    xtop_state_access_control & operator=(xtop_state_access_control const &) = delete;
    xtop_state_access_control(xtop_state_access_control &&) = default;
    xtop_state_access_control & operator=(xtop_state_access_control &&) = default;
    ~xtop_state_access_control() = default;

    explicit xtop_state_access_control(top::observer_ptr<top::base::xvbstate_t> bstate, xstate_access_control_data_t ac_data);

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

    void create_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec);

    template <properties::xproperty_type_t PropertyTypeV>
    void set_property(std::string const & property_name, properties::xproperty_category_t const property_category, typename properties::xtype_of_t<PropertyTypeV>::type const & value, std::error_code & ec);

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
    /// @param property_id Property ID.
    /// @param ec Log error in the process.
    void do_create_string_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec);

    /// @brief Create map property.
    /// @param property_id Property ID.
    /// @param ec Log error in the process.
    void do_create_map_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec);
};
using xstate_access_control_t = xtop_state_access_control;

}
}

namespace top {
namespace state {

template <>
void xstate_access_control_t::set_property<properties::xproperty_type_t::string>(std::string const & property_name,
                                                                                 properties::xproperty_category_t const property_category,
                                                                                 std::string const & value,
                                                                                 std::error_code & ec) {

}

template <>
void xstate_access_control_t::set_property<properties::xproperty_type_t::map>(std::string const & property_name,
                                                                              properties::xproperty_category_t const property_category,
                                                                              std::map<std::string, xbyte_buffer_t> const & value,
                                                                              std::error_code & ec) {
}

}
}
