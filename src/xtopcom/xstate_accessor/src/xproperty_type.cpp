// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_accessor/xproperties/xproperty_type.h"

#if !defined(XCXX14_OR_ABOVE)
#    include <type_traits>
#endif

NS_BEG3(top, state_accessor, properties)

std::string to_string(xproperty_type_t const type) {
    switch (type) {
    case xproperty_type_t::uint8: return "u8";
    case xproperty_type_t::uint16: return "u16";
    case xproperty_type_t::uint32: return "u32";
    case xproperty_type_t::uint64: return "u64";
    case xproperty_type_t::int8: return "i8";
    case xproperty_type_t::int16: return "i16";
    case xproperty_type_t::int32: return "i32";
    case xproperty_type_t::int64: return "i64";
    case xproperty_type_t::token: return "token";
    case xproperty_type_t::nonce: return "nonce";
    case xproperty_type_t::bytes: return "bytes";
    case xproperty_type_t::string: return "string";
    case xproperty_type_t::bin_code: return "byte-code";
    case xproperty_type_t::map: return "map";
    case xproperty_type_t::deque: return "deque";
    default: return "null";
    }
}

NS_END3


#if !defined(XCXX14_OR_ABOVE)
NS_BEG1(std)

size_t hash<top::state_accessor::properties::xproperty_type_t>::operator()(top::state_accessor::properties::xproperty_type_t const property_type) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::state_accessor::properties::xproperty_type_t>::type>(property_type));
}

NS_END1
#endif

