// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xvledger/xvstate.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbase/xns_macro.h"

#include <map>
#include <vector>
#include <string>
#include <cstdint>

NS_BEG3(top, contract_common, properties)

enum class xenum_property_type {
    invalid,
    uint8,
    uint16,
    uint32,
    uint64,
    int8,
    int16,
    int32,
    int64,
    token,
    nonce,
    src_code,
    bin_code,
    bytes,
    string,
    map,
    vector,
};
using xproperty_type_t = xenum_property_type;

//template <xproperty_type_t TypeV>
//struct xtop_type_of;
//
//template <xproperty_type_t TypeV, typename ValueT>
//struct xtop_type_of;

template <xproperty_type_t TypeV, typename T = void, typename U = void>
struct xtop_type_of;

template <xproperty_type_t TypeV, typename T = void, typename U = void>
using xtype_of_t = xtop_type_of<TypeV, T, U>;

template <typename KeyT, typename ValueT>
struct xtop_type_of<xproperty_type_t::map, KeyT, ValueT> {
    using type = typename std::map<KeyT, ValueT>;
};

template <typename T, typename U>
struct xtop_type_of<xproperty_type_t::vector, T, U> {
    using type = std::vector<T>;
};

template <typename T, typename U>
struct xtop_type_of<xproperty_type_t::string, T, U> {
    using type = std::string;
};

template <typename T, typename U>
struct xtop_type_of<xproperty_type_t::token, T, U> {
    using type = base::vtoken_t;
};

NS_END3

#if !defined(XCXX14_OR_ABOVE)
NS_BEG1(std)

template <>
struct hash<top::contract_common::properties::xproperty_type_t> {
    size_t operator()(top::contract_common::properties::xproperty_type_t const property_type) const noexcept;
};

NS_END1
#endif
