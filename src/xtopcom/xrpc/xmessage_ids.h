// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xcommon/xmessage_id.h"

NS_BEG2(top, xrpc)

XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_request, 0x00000001);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_response, 0x00000002);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_query_request, 0x00000003);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_eth_request, 0x00000004);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_eth_response, 0x00000005);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_eth_query_request, 0x00000006);

NS_END2
