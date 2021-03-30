#pragma once

#include "xbasic/xns_macro.h"
#include "xcommon/xmessage_id.h"

NS_BEG2(top, syncbase)

XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_blocks, 0x1);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_blocks, 0x2);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_newblock, 0x3);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_newblockhash, 0x4);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_gossip, 0x5);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_frozen_gossip, 0x6);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_latest_block_info, 0x7);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_latest_blocks, 0x8);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_latest_blocks, 0x9);

NS_END2
