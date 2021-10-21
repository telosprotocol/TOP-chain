// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"

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

#include <map>
#include <vector>
#include <string>
#include <cstdint>

namespace top {
namespace state_accessor {
namespace properties {

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
    deque,
};
using xproperty_type_t = xenum_property_type;

std::string to_string(xproperty_type_t const type);

template <xproperty_type_t PropertyTypeV>
struct xtop_type_of;

template <xproperty_type_t PropertyTypeV>
using xtype_of_t = xtop_type_of<PropertyTypeV>;

template <xproperty_type_t PropertyTypeV>
struct xtop_value_type_of;

template <xproperty_type_t PropertyTypeV>
using xvalue_type_of_t = xtop_value_type_of<PropertyTypeV>;

template <xproperty_type_t PropertyTypeV>
struct xtop_key_type_of;

template <xproperty_type_t PropertyTypeV>
using xkey_type_of_t = xtop_key_type_of<PropertyTypeV>;


template <>
struct xtop_type_of<xproperty_type_t::int8> {
    using type = int8_t;
};

template <>
struct xtop_value_type_of<xproperty_type_t::int8> {
    using type = int8_t;
};

template <>
struct xtop_type_of<xproperty_type_t::int16> {
    using type = int16_t;
};

template <>
struct xtop_value_type_of<xproperty_type_t::int16> {
    using type = int16_t;
};

template <>
struct xtop_type_of<xproperty_type_t::int32> {
    using type = int32_t;
};

template <>
struct xtop_value_type_of<xproperty_type_t::int32> {
    using type = int32_t;
};

template <>
struct xtop_type_of<xproperty_type_t::int64> {
    using type = int64_t;
};

template <>
struct xtop_value_type_of<xproperty_type_t::int64> {
    using type = int64_t;
};

template <>
struct xtop_type_of<xproperty_type_t::uint8> {
    using type = uint8_t;
};

template <>
struct xtop_value_type_of<xproperty_type_t::uint8> {
    using type = uint8_t;
};


template <>
struct xtop_type_of<xproperty_type_t::uint16> {
    using type = uint16_t;
};

template <>
struct xtop_value_type_of<xproperty_type_t::uint16> {
    using type = uint16_t;
};

template <>
struct xtop_type_of<xproperty_type_t::uint32> {
    using type = uint32_t;
};

template <>
struct xtop_value_type_of<xproperty_type_t::uint32> {
    using type = uint32_t;
};

template <>
struct xtop_type_of<xproperty_type_t::uint64> {
    using type = uint64_t;
};

template <>
struct xtop_value_type_of<xproperty_type_t::uint64> {
    using type = uint64_t;
};

template <>
struct xtop_type_of<xproperty_type_t::map> {
    using type = std::map<std::string, xbytes_t>;
};

template <>
struct xtop_value_type_of<xproperty_type_t::map> {
    using type = xtype_of_t<xproperty_type_t::map>::type::mapped_type;
};

template <>
struct xtop_key_type_of<xproperty_type_t::map> {
    using type = xtype_of_t<xproperty_type_t::map>::type::key_type;
};

template <>
struct xtop_type_of<xproperty_type_t::deque> {
    using type = std::deque<xbyte_buffer_t>;
};

template <>
struct xtop_value_type_of<xproperty_type_t::deque> {
    using type = xtype_of_t<xproperty_type_t::deque>::type::value_type;
};

template <>
struct xtop_key_type_of<xproperty_type_t::deque> {
    using type = size_t;
};

template <>
struct xtop_type_of<xproperty_type_t::string> {
    using type = std::string;
};

template <>
struct xtop_value_type_of<xproperty_type_t::string> {
    using type = std::string;
};

template <>
struct xtop_type_of<xproperty_type_t::bytes> {
    using type = xbytes_t;
};

template <>
struct xtop_type_of<xproperty_type_t::token> {
    using type = base::vtoken_t;
};

}
}
}

#if !defined(XCXX14_OR_ABOVE)
NS_BEG1(std)

template <>
struct hash<top::state_accessor::properties::xproperty_type_t> {
    size_t operator()(top::state_accessor::properties::xproperty_type_t const property_type) const noexcept;
};

NS_END1
#endif
