// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xmessage_id.h"

NS_BEG2(top, common)

xmessage_category_t
get_message_category(xmessage_id_t const message_id) noexcept {
    return static_cast<xmessage_category_t>(static_cast<std::uint16_t>((static_cast<std::uint32_t>(message_id) & 0xFFFF0000) >> 16));
}


NS_END2
