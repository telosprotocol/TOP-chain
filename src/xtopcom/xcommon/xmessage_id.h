// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xmessage_category.h"

#include <cstdint>

#define XDEFINE_MSG_ID(MSG_CATEGORY_ID, MSG_NAME, MSG_ID)                                                                                                                               \
    XINLINE_CONSTEXPR top::common::xmessage_id_t MSG_NAME                                                                                                                               \
    {                                                                                                                                                                                   \
        static_cast<top::common::xmessage_id_t>((static_cast<std::uint32_t>(static_cast<std::uint16_t>(MSG_CATEGORY_ID)) << 16) | (0x0000FFFF & static_cast<std::uint32_t>(MSG_ID)))    \
    }


NS_BEG2(top, common)

enum class xenum_message_id : std::uint32_t
{
    invalid = 0x00000000
};

using xmessage_id_t = xenum_message_id;

xmessage_category_t
get_message_category(xmessage_id_t const message_id) noexcept;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::common::xmessage_id_t> final
{
    std::size_t
    operator()(top::common::xmessage_id_t const message_id) const noexcept {
        return static_cast<std::size_t>(message_id);
    }
};


NS_END1
