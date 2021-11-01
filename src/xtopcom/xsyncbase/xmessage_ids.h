#pragma once

#include "xcommon/xmessage_id.h"

NS_BEG2(top, sync)

XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_blocks, 0x1);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_blocks, 0x2);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_push_newblock, 0x3);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_gossip, 0x5);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_frozen_gossip, 0x6);
#if 0
// discard
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_latest_block_info, 0x7);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_latest_blocks, 0x8);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_latest_blocks, 0x9);
#endif
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_on_demand_blocks, 0xa);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_on_demand_blocks, 0xb);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_broadcast_chain_state, 0xc);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_response_chain_state, 0xd);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_frozen_broadcast_chain_state, 0xe);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_frozen_response_chain_state, 0xf);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_cross_cluster_chain_state, 0x10);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_blocks_by_hashes, 0x11);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_blocks_by_hashes, 0x12);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_push_newblockhash, 0x13);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_broadcast_newblockhash, 0x14);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_chain_snapshot_request, 0x15);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_chain_snapshot_response, 0x16);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_ondemand_chain_snapshot_request, 0x17);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_ondemand_chain_snapshot_response, 0x18);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_on_demand_by_hash_blocks, 0x19);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_on_demand_by_hash_blocks, 0x20);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_on_demand_blocks_with_proof, 0x21);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_on_demand_blocks_with_proof, 0x22);

NS_END2
