// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstatistic/xclass_type_converter.h"
#include "xcommon/xmessage_id.h"

NS_BEG2(top, xstatistic)

#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)
enum_statistic_class_type message_id_to_class_type(uint32_t msgid) {
    uint32_t message_category = (msgid & 0xFFFF0000) >> 16;

#ifndef CACHE_SIZE_STATISTIC_MORE_DETAIL
    switch (message_category) {
        case (uint32_t)xmessage_category_consensus :
        case (uint32_t)xmessage_category_timer :
        case (uint32_t)xmessage_category_relay :
            return enum_statistic_msg_cons;
        case (uint32_t)xmessage_category_txpool :
            return enum_statistic_msg_txpool;
        case (uint32_t)xmessage_category_rpc :
            return enum_statistic_msg_rpc;
        case (uint32_t)xmessage_category_sync :
            return enum_statistic_msg_sync;
        case (uint32_t)xmessage_block_broadcast :
            return enum_statistic_msg_block_broadcast;
        case (uint32_t)xmessage_category_state_sync :
            return enum_statistic_msg_state;
        default :
            xerror("msgid:%u", msgid);
            return enum_statistic_undetermined;     
    }
#else
    // value of msgid of bft message is added pdu_type, so that here can not directly use xBFT_msg, xTimer_msg and xrelay_BFT_msg.
    switch (message_category) {
        case (uint32_t)xmessage_category_consensus :
            return enum_statistic_msg_bft;
        case (uint32_t)xmessage_category_timer :
            return enum_statistic_msg_timer;
        case (uint32_t)xmessage_category_relay :
            return enum_statistic_msg_relay_bft;
        default :
            break;
    }

    switch (msgid) {
    case (uint32_t)common::xmessage_id_t::invalid:
            return enum_statistic_undetermined;
        case (uint32_t)rpc_msg_request :
            return enum_statistic_msg_rpc_request;
        case (uint32_t)rpc_msg_response :
            return enum_statistic_msg_rpc_response;
        case (uint32_t)rpc_msg_query_request :
            return enum_statistic_msg_rpc_query_request;
        case (uint32_t)rpc_msg_eth_request :
            return enum_statistic_msg_rpc_eth_request;
        case (uint32_t)rpc_msg_eth_response :
            return enum_statistic_msg_rpc_eth_response;
        case (uint32_t)rpc_msg_eth_query_request :
            return enum_statistic_msg_rpc_eth_query_request;

        case (uint32_t)xmessage_id_sync_trie_request :
            return enum_statistic_msg_state_trie_request;
        case (uint32_t)xmessage_id_sync_trie_response :
            return enum_statistic_msg_state_trie_response;
        case (uint32_t)xmessage_id_sync_table_request :
            return enum_statistic_msg_state_table_request;
        case (uint32_t)xmessage_id_sync_table_response :
            return enum_statistic_msg_state_table_response;
        case (uint32_t)xmessage_id_sync_unit_request :
            return enum_statistic_msg_state_unit_request;
        case (uint32_t)xmessage_id_sync_unit_response :
            return enum_statistic_msg_state_unit_response;

        case (uint32_t)xtxpool_msg_send_receipt :
            return enum_statistic_msg_txpool_send_receipt;
        case (uint32_t)xtxpool_msg_recv_receipt :
            return enum_statistic_msg_txpool_recv_receipt;
        case (uint32_t)xtxpool_msg_pull_recv_receipt :
            return enum_statistic_msg_txpool_pull_recv_receipt;
        case (uint32_t)xtxpool_msg_push_receipt :
            return enum_statistic_msg_txpool_push_receipt;
        case (uint32_t)xtxpool_msg_pull_confirm_receipt_v2 :
            return enum_statistic_msg_txpool_pull_confirm_receipt;
        case (uint32_t)xtxpool_msg_receipt_id_state :
            return enum_statistic_msg_txpool_receipt_id_state;

        case (uint32_t)xmessage_block_broadcast_id :
            return enum_statistic_msg_block_broadcast;

        case (uint32_t)xmessage_id_sync_gossip :
            return enum_statistic_msg_sync_gossip;
        case (uint32_t)xmessage_id_sync_frozen_gossip :
            return enum_statistic_msg_sync_frozen_gossip;
        case (uint32_t)xmessage_id_sync_broadcast_chain_state :
            return enum_statistic_msg_sync_broadcast_chain_state;
        case (uint32_t)xmessage_id_sync_frozen_broadcast_chain_state :
            return enum_statistic_msg_sync_frozen_broadcast_chain_state;
        case (uint32_t)xmessage_id_sync_response_chain_state :
            return enum_statistic_msg_sync_response_chain_state;
        case (uint32_t)xmessage_id_sync_frozen_response_chain_state :
            return enum_statistic_msg_sync_frozen_response_chain_state;
        case (uint32_t)xmessage_id_sync_cross_cluster_chain_state :
            return enum_statistic_msg_sync_cross_cluster_chain_state;
        case (uint32_t)xmessage_id_sync_chain_snapshot_request :
            return enum_statistic_msg_sync_chain_snapshot_request;
        case (uint32_t)xmessage_id_sync_chain_snapshot_response :
            return enum_statistic_msg_sync_chain_snapshot_response;
        case (uint32_t)xmessage_id_sync_ondemand_chain_snapshot_request :
            return enum_statistic_msg_sync_ondemand_chain_snapshot_request;
        case (uint32_t)xmessage_id_sync_ondemand_chain_snapshot_response :
            return enum_statistic_msg_sync_ondemand_chain_snapshot_response;
        case (uint32_t)xmessage_id_sync_query_archive_height :
            return enum_statistic_msg_sync_query_archive_height;
        case (uint32_t)xmessage_id_sync_newblock_push :
            return enum_statistic_msg_sync_newblock_push;
        case (uint32_t)xmessage_id_sync_block_request :
            return enum_statistic_msg_sync_block_request;
        case (uint32_t)xmessage_id_sync_block_response :
            return enum_statistic_msg_sync_block_response;
        default: {
            xerror("msgid:%u", msgid);
            return enum_statistic_undetermined;            
        }
    }
#endif
}
#else
enum_statistic_class_type message_id_to_class_type(uint32_t msgid) {
    return enum_statistic_max;
}
#endif

NS_END2
