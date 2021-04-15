// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>

NS_BEG3(top, network, codec)

enum class xenum_message_flags : std::uint8_t
{
    none = 0
};

using xmessage_flags_t = xenum_message_flags;

#if defined __cplusplus && __cplusplus >= 201703L
inline
#endif
constexpr xmessage_flags_t xmessage_flags_df{ static_cast<xmessage_flags_t>(0x01) };    // Don't Fragment

#if defined __cplusplus && __cplusplus >= 201703L
inline
#endif
constexpr xmessage_flags_t xmessage_flags_mf{ static_cast<xmessage_flags_t>(0x02) };    // More Fragments

xmessage_flags_t
operator |(xmessage_flags_t const l, xmessage_flags_t const r) noexcept;

xmessage_flags_t
operator &(xmessage_flags_t const l, xmessage_flags_t const r) noexcept;

NS_END3
