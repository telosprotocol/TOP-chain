// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xcodec/xmessage_flags.h"

#include <type_traits>

NS_BEG3(top, network, codec)

xmessage_flags_t
operator |(xmessage_flags_t const l, xmessage_flags_t const r) noexcept {
    using numeric_type = std::underlying_type<xmessage_flags_t>::type;
    auto const lhs = static_cast<numeric_type>(l);
    auto const rhs = static_cast<numeric_type>(r);

    return static_cast<xmessage_flags_t>(lhs | rhs);
}

xmessage_flags_t
operator &(xmessage_flags_t const l, xmessage_flags_t const r) noexcept {
    using numeric_type = std::underlying_type<xmessage_flags_t>::type;
    auto const lhs = static_cast<numeric_type>(l);
    auto const rhs = static_cast<numeric_type>(r);

    return static_cast<xmessage_flags_t>(lhs & rhs);
}

NS_END3
