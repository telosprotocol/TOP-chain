// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xvnetwork/xmessage.h"

NS_BEG2(top, xunit_service)

XDEFINE_MSG_ID(xmessage_category_consensus, xBFT_msg, 0x00000000);
XDEFINE_MSG_ID(xmessage_category_timer, xTimer_msg, 0x00000000);
XDEFINE_MSG_ID(xmessage_category_relay, xrelay_BFT_msg, 0x00000000);

NS_END2
