// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xsimple_message.hpp"

NS_BEG2(top, network)

enum class xenum_message_type{
    invalid = 0,
    normal,
    direct
};
using xmessage_type_t = xenum_message_type;

using xmessage_t = xsimple_message_t<xmessage_type_t>;

NS_END2
