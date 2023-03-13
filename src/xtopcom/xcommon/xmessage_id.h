// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xmessage_category.h"

#include <cstdint>

#define XDEFINE_MSG_ID(MSG_CATEGORY_ID, MSG_NAME, MSG_ID)                                                                                                                               \
    constexpr top::common::xmessage_id_t MSG_NAME                                                                                                                               \
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

// message ids for us
XDEFINE_MSG_ID(xmessage_category_consensus, xBFT_msg, 0x00000000);
XDEFINE_MSG_ID(xmessage_category_timer, xTimer_msg, 0x00000000);
XDEFINE_MSG_ID(xmessage_category_relay, xrelay_BFT_msg, 0x00000000);

// message ids for txpool
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_send_receipt, 0x00000001);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_recv_receipt, 0x00000002);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_pull_recv_receipt, 0x00000003);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_pull_confirm_receipt, 0x00000004);  // keep it for compatibility
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_push_receipt, 0x00000005);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_pull_confirm_receipt_v2, 0x00000006);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_receipt_id_state, 0x00000007);

// message ids for rpc
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_request, 0x00000001);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_response, 0x00000002);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_query_request, 0x00000003);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_eth_request, 0x00000004);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_eth_response, 0x00000005);
XDEFINE_MSG_ID(xmessage_category_rpc, rpc_msg_eth_query_request, 0x00000006);

// message ids for sync
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_blocks, 0x1);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_blocks, 0x2);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_push_newblock, 0x3);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_gossip, 0x5);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_frozen_gossip, 0x6);
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
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_archive_height, 0x23);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_archive_blocks, 0x24);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_query_archive_height, 0x25);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_archive_height_list, 0x26);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_get_on_demand_blocks_with_hash, 0x27);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_on_demand_blocks_with_hash, 0x28);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_newblock_push, 0x29);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_block_request, 0x30);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_block_response, 0x31);
XDEFINE_MSG_ID(xmessage_category_sync, xmessage_id_sync_block_response_bigpack, 0x32);

//restrict sync message:  increase message id 0x100 when packet size > 16m, and reduce 0x100 after unserialize 
#define sync_big_pack_increase_index  (0x100)

// message ids for block broadcast
XDEFINE_MSG_ID(xmessage_block_broadcast, xmessage_block_broadcast_id, 0x01);

// message ids for state sync
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_trie_request, 0x01);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_trie_response, 0x02);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_table_request, 0x03);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_table_response, 0x04);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_unit_request, 0x05);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_unit_response, 0x06);

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
