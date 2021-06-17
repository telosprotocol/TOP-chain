// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate_accessor/xproperties/xproperty_type.h"

NS_BEG3(top, state_accessor, properties)

class xtop_property_identifier;
using xproperty_identifier_t = xtop_property_identifier;

class xtop_typeless_property_identifier;
using xtypeless_property_identifier_t = xtop_typeless_property_identifier;

//template <xproperty_type_t PropertyTypeV, typename std::enable_if<PropertyTypeV != xproperty_type_t::invalid>::type * = nullptr>
//class xtop_typed_property_identifier;
//
//template <xproperty_type_t PropertyTypeV>
//using xtyped_property_identifier_t = xtop_typed_property_identifier<PropertyTypeV>;

NS_END3

