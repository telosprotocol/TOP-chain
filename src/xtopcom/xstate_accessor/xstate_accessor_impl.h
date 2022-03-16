// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

template <>
properties::xtype_of_t<properties::xproperty_type_t::int64>::type xstate_accessor_t::get_property<properties::xproperty_type_t::int64>(
    properties::xtypeless_property_identifier_t const & property_id,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::uint64>::type xstate_accessor_t::get_property<properties::xproperty_type_t::uint64>(
    properties::xtypeless_property_identifier_t const & property_id,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::bytes>::type xstate_accessor_t::get_property<properties::xproperty_type_t::bytes>(
    properties::xtypeless_property_identifier_t const & property_id,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::string>::type xstate_accessor_t::get_property<properties::xproperty_type_t::string>(
    properties::xtypeless_property_identifier_t const & property_id,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::map>::type xstate_accessor_t::get_property<properties::xproperty_type_t::map>(
    properties::xtypeless_property_identifier_t const & property_id,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::deque>::type xstate_accessor_t::get_property<properties::xproperty_type_t::deque>(
    properties::xtypeless_property_identifier_t const & property_id,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::int64>::type xstate_accessor_t::get_property<properties::xproperty_type_t::int64>(
    properties::xtypeless_property_identifier_t const & property_id,
    common::xaccount_address_t const & address,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::uint64>::type xstate_accessor_t::get_property<properties::xproperty_type_t::uint64>(
    properties::xtypeless_property_identifier_t const & property_id,
    common::xaccount_address_t const & address,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::bytes>::type xstate_accessor_t::get_property<properties::xproperty_type_t::bytes>(
    properties::xtypeless_property_identifier_t const & property_id,
    common::xaccount_address_t const & address,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::string>::type xstate_accessor_t::get_property<properties::xproperty_type_t::string>(
    properties::xtypeless_property_identifier_t const & property_id,
    common::xaccount_address_t const & address,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::map>::type xstate_accessor_t::get_property<properties::xproperty_type_t::map>(
    properties::xtypeless_property_identifier_t const & property_id,
    common::xaccount_address_t const & address,
    std::error_code & ec) const;

template <>
properties::xtype_of_t<properties::xproperty_type_t::deque>::type xstate_accessor_t::get_property<properties::xproperty_type_t::deque>(
    properties::xtypeless_property_identifier_t const & property_id,
    common::xaccount_address_t const & address,
    std::error_code & ec) const;

template <>
void xstate_accessor_t::set_property<properties::xproperty_type_t::int64>(properties::xtypeless_property_identifier_t const & property_id,
                                                                           properties::xtype_of_t<properties::xproperty_type_t::int64>::type const & value,
                                                                           std::error_code & ec);
template <>
void xstate_accessor_t::set_property<properties::xproperty_type_t::uint64>(properties::xtypeless_property_identifier_t const & property_id,
                                                                           properties::xtype_of_t<properties::xproperty_type_t::uint64>::type const & value,
                                                                           std::error_code & ec);

template <>
void xstate_accessor_t::set_property<properties::xproperty_type_t::bytes>(properties::xtypeless_property_identifier_t const & property_id,
                                                                           properties::xtype_of_t<properties::xproperty_type_t::bytes>::type const & value,
                                                                           std::error_code & ec);

template <>
void xstate_accessor_t::set_property<properties::xproperty_type_t::string>(properties::xtypeless_property_identifier_t const & property_id,
                                                                           properties::xtype_of_t<properties::xproperty_type_t::string>::type const & value,
                                                                           std::error_code & ec);

template <>
void xstate_accessor_t::set_property_cell_value<properties::xproperty_type_t::map>(properties::xtypeless_property_identifier_t const & property_id,
                                                                                   properties::xkey_type_of_t<properties::xproperty_type_t::map>::type const & key,
                                                                                   properties::xvalue_type_of_t<properties::xproperty_type_t::map>::type const & value,
                                                                                   std::error_code & ec);

template <>
void xstate_accessor_t::set_property_cell_value<properties::xproperty_type_t::deque>(properties::xtypeless_property_identifier_t const & property_id,
                                                                                     properties::xkey_type_of_t<properties::xproperty_type_t::deque>::type const & key,
                                                                                     properties::xvalue_type_of_t<properties::xproperty_type_t::deque>::type const & value,
                                                                                     std::error_code & ec);

template <>
properties::xvalue_type_of_t<properties::xproperty_type_t::map>::type
xstate_accessor_t::get_property_cell_value<properties::xproperty_type_t::map>(properties::xtypeless_property_identifier_t const & property_id,
                                                                              properties::xkey_type_of_t<properties::xproperty_type_t::map>::type const & key,
                                                                              std::error_code & ec) const;

template <>
properties::xvalue_type_of_t<properties::xproperty_type_t::deque>::type
xstate_accessor_t::get_property_cell_value<properties::xproperty_type_t::deque>(properties::xtypeless_property_identifier_t const & property_id,
                                                                                properties::xkey_type_of_t<properties::xproperty_type_t::deque>::type const & key,
                                                                                std::error_code & ec) const;

template <>
void xstate_accessor_t::remove_property_cell<properties::xproperty_type_t::map>(properties::xtypeless_property_identifier_t const & property_id,
                                                                                typename properties::xkey_type_of_t<properties::xproperty_type_t::map>::type const & key,
                                                                                std::error_code & ec);

template <>
void xstate_accessor_t::remove_property_cell<properties::xproperty_type_t::deque>(properties::xtypeless_property_identifier_t const & property_id,
                                                                                  typename properties::xkey_type_of_t<properties::xproperty_type_t::deque>::type const & key,
                                                                                  std::error_code & ec);

template <>
void xstate_accessor_t::do_create_int_property<properties::xproperty_type_t::uint64>(std::string const & property_name, std::error_code & ec);

template <>
void xstate_accessor_t::do_create_int_property<properties::xproperty_type_t::int64>(std::string const & property_name, std::error_code & ec);
