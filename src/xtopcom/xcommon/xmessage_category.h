// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <functional>
#include <type_traits>

#define XDEFINE_MSG_CATEGORY(MSG_CATEGORY_NAME, MSG_CATEGORY_ID)                                                                                    \
    constexpr top::common::xmessage_category_t MSG_CATEGORY_NAME                                                                            \
    {                                                                                                                                               \
        static_cast<top::common::xmessage_category_t>(static_cast<std::underlying_type<top::common::xmessage_category_t>::type>(MSG_CATEGORY_ID))   \
    }

NS_BEG2(top, common)

enum class xenum_message_category : std::uint16_t
{
    invalid = 0x0000
};
using xmessage_category_t = xenum_message_category;

NS_END2

XDEFINE_MSG_CATEGORY(xmessage_category_consensus,   0x0001);
XDEFINE_MSG_CATEGORY(xmessage_category_timer,       0x0002);
XDEFINE_MSG_CATEGORY(xmessage_category_txpool,      0x0003);
XDEFINE_MSG_CATEGORY(xmessage_category_rpc,         0x0004);
XDEFINE_MSG_CATEGORY(xmessage_category_sync,        0x0005);
XDEFINE_MSG_CATEGORY(xmessage_block_broadcast,      0x0006);
XDEFINE_MSG_CATEGORY(xmessage_category_relay,       0x0007);
XDEFINE_MSG_CATEGORY(xmessage_category_state_sync,  0x0008);

NS_BEG1(std)

template <>
struct hash<top::common::xmessage_category_t> final
{
    std::size_t
    operator()(top::common::xmessage_category_t const message_category) const noexcept {
        return static_cast<std::size_t>(message_category);
    }
};

NS_END1
