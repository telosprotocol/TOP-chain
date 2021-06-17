// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <functional>

NS_BEG3(top, state_accessor, properties)

enum class xenum_property_category: std::uint8_t {
    invalid,
    system,
    user
};
using xproperty_category_t = xenum_property_category;

char category_character(xproperty_category_t const c) noexcept;

NS_END3

#if !defined(XCXX14_OR_ABOVE)
NS_BEG1(std)

template <>
struct hash<top::state_accessor::properties::xproperty_category_t> {
    size_t operator()(top::state_accessor::properties::xproperty_category_t const property_category) const noexcept;
};

NS_END1
#endif
