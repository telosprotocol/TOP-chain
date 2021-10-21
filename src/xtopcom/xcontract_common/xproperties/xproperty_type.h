// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xproperties/xproperty_bytes.h"
#include "xcontract_common/xproperties/xproperty_integer.h"
#include "xcontract_common/xproperties/xproperty_map.h"
#include "xcontract_common/xproperties/xproperty_string.h"
#include "xcontract_common/xproperties/xproperty_token.h"
#include "xstate_accessor/xproperties/xproperty_type.h"

NS_BEG3(top, contract_common, properties)

template <state_accessor::properties::xproperty_type_t PropertyTypeV, typename ... Ts>
struct xtop_property_type_of;

template <state_accessor::properties::xproperty_type_t PropertyTypeV, typename ... Ts>
using xproperty_type_of_t = xtop_property_type_of<PropertyTypeV, Ts...>;

template <>
struct xtop_property_type_of<state_accessor::properties::xproperty_type_t::bytes> {
    using type = xbytes_property_t;
};

template <>
struct xtop_property_type_of<state_accessor::properties::xproperty_type_t::int64> {
    using type = xint64_property_t;
};

template <>
struct xtop_property_type_of<state_accessor::properties::xproperty_type_t::uint64> {
    using type = xuint64_property_t;
};

template <typename KeyT, typename ValueT>
struct xtop_property_type_of<state_accessor::properties::xproperty_type_t::map, KeyT, ValueT> {
    using type = xmap_property_t<KeyT, ValueT>;
};

template <>
struct xtop_property_type_of<state_accessor::properties::xproperty_type_t::string> {
    using type = xstring_property_t;
};

template <>
struct xtop_property_type_of<state_accessor::properties::xproperty_type_t::token> {
    using type = xtoken_property_t;
};

NS_END3
