// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbasic/xrunnable.h"
#include "xbasic/xthreading/xthreadsafe_queue.hpp"
#ifdef ENABLE_METRICS
#include "metrics_handler/basic_handler.h"
#include "metrics_handler/array_counter_handler.h"
#include "metrics_handler/counter_handler.h"
#include "metrics_handler/flow_handler.h"
#include "metrics_handler/timer_handler.h"
#include "metrics_handler/xmetrics_packet_info.h"
#include "xmetrics_event.h"
#include "xmetrics_unit.h"
#endif

#include <chrono>
#include <map>
#include <string>
#include <thread>
NS_BEG2(top, metrics)

enum E_SIMPLE_METRICS_TAG : size_t {
    e_simple_begin = 0,
    blockstore_cache_block_total = e_simple_begin+1,

    vhost_recv_msg,
    vhost_recv_callback,
    vnode_recv_msg,
    vnode_recv_callback,

    dataobject_cons_transaction,
    dataobject_block_lightunit,
    dataobject_block_fullunit,
    dataobject_block_lighttable,
    dataobject_block_fulltable,
    dataobject_block_empty,
    dataobject_tx_receipt_t,
    dataobject_unit_state,
    dataobject_xvtxindex,
    dataobject_xvbstate,
    dataobject_xvproperty,
    dataobject_xvaccountobj,
    dataobject_exeunit,
    dataobject_exegroup,
    dataobject_xvexecontxt,
    dataobject_xaccount_index,
    dataobject_xreceiptid_pair_t,
    dataobject_xvbindex_t,
    dataobject_xtransaction_t,
    dataobject_provcert,
    dataobject_xvaccount,
    dataobject_xvaction,
    dataobject_xvheader,
    dataobject_xvqcert,
    dataobject_xvblock,
    dataobject_xvinput,
    dataobject_xvoutput,
    dataobject_xventity,
    dataobject_xvnode_t,
    dataobject_xvexestate_t,
    dataobject_xvnodegroup,
    dataobject_xcscoreobj_t,
    dataobject_xblock_maker_t,
    dataobject_xblockacct_t,
    dataobject_xtxpool_table_info_t,    
    dataobject_xacctmeta_t,
    // db bock key, see xvdbkey for specific info
    // 't/', 'i/', 'b/'
    // 'b/.../h', 'b/.../i', 'b/.../ir', 'b/.../o', 'b/.../or', 'b/.../s', 'b/.../d'
    db_key_tx,
    db_key_block_index,
    db_key_block,
    db_key_block_object,
    db_key_block_input,
    db_key_block_input_resource,
    db_key_block_output,
    db_key_block_output_resource,
    db_key_block_state,
    db_read,
    db_write,
    db_delete,
    db_read_size,
    db_write_size,
    db_read_tick,
    db_write_tick,
    db_delete_tick,

    // consensus
    cons_drand_leader_finish_succ,// TODO(jimmy) delete future
    cons_drand_backup_finish_succ,// TODO(jimmy) delete future
    cons_drand_leader_finish_fail,// TODO(jimmy) delete future
    cons_drand_backup_finish_fail,// TODO(jimmy) delete future
    cons_tableblock_leader_finish_succ,  // TODO(jimmy) delete future
    cons_tableblock_backup_finish_succ, // TODO(jimmy) delete future
    cons_tableblock_leader_finish_fail, // TODO(jimmy) delete future
    cons_tableblock_backup_finish_fail, // TODO(jimmy) delete future
    cons_drand_leader_succ,
    cons_drand_backup_succ,
    cons_tableblock_leader_succ,
    cons_tableblock_backup_succ,
    cons_tableblock_total_succ,
    cons_pacemaker_tc_discontinuity,

    cons_table_leader_make_proposal_succ,
    cons_fail_make_proposal_table_state,
    cons_fail_make_proposal_consensus_para,
    cons_fail_make_proposal_table_check_latest_state,
    cons_fail_make_proposal_unit_check_state,
    cons_fail_make_proposal_view_changed,

    cons_table_backup_verify_proposal_succ,
    cons_fail_verify_proposal_blocks_invalid,
    cons_fail_verify_proposal_table_state_get,
    cons_fail_verify_proposal_drand_invalid,
    cons_fail_verify_proposal_consensus_para_get,
    cons_fail_verify_proposal_unit_count,
    cons_fail_verify_proposal_table_check_latest_state,
    cons_fail_verify_proposal_table_with_local,

    cons_view_fire_clock_delay,
    cons_view_fire_succ,
    cons_view_fire_is_leader,
    cons_fail_backup_view_not_match,
    cons_make_proposal_tick,
    cons_verify_proposal_tick,
    cons_make_fulltable_tick,
    cons_make_lighttable_tick,
    cons_verify_lighttable_tick,
    cons_make_unit_tick,
    cons_unitbuilder_lightunit_tick,
    cons_unitbuilder_fullunit_tick,
    cons_unitmaker_check_state_tick,
    cons_tablebuilder_lighttable_tick,
    cons_tablebuilder_fulltable_tick,
    cons_tablemaker_verify_proposal_tick,
    cons_tablemaker_make_proposal_tick,
    cons_tablemaker_check_state_tick,
    cons_tablemaker_refresh_cache,

    cons_table_leader_get_txpool_tx_count,
    cons_table_leader_get_txpool_sendtx_count,
    cons_table_leader_get_txpool_recvtx_count,
    cons_table_leader_get_txpool_confirmtx_count,
    cons_table_leader_make_tx_count,
    cons_table_leader_make_unit_count,
    cons_table_total_process_tx_count,
    cons_table_total_process_unit_count,

    cons_packtx_succ,
    cons_packtx_sendtx_succ,
    cons_packtx_recvtx_succ,
    cons_packtx_confirmtx_succ,
    cons_packtx_fail_unit_check_state,
    cons_packtx_fail_fullunit_limit,
    cons_packtx_fail_receiptid_contious,
    cons_packtx_fail_total_unconfirm_limit,
    cons_packtx_fail_table_unconfirm_limit,
    cons_packtx_fail_nonce_contious,
    cons_packtx_fail_transfer_limit, // TODO(jimmy) need delete limit
    cons_packtx_fail_load_origintx,

    clock_aggregate_height,
    clock_leader_broadcast_height,
    clock_received_height,

    // store
    store_state_read,
    store_state_table_write,
    store_state_unit_write,
    store_state_delete,
    store_block_table_read,
    store_block_unit_read,
    store_block_other_read,
    store_block_index_read,
    store_block_input_read,
    store_block_output_read,
    store_block_call,
    store_block_table_write,
    store_block_unit_write,
    store_block_other_write,
    store_block_index_table_write,
    store_block_index_unit_write,
    store_block_index_other_write,
    store_block_input_table_write,
    store_block_input_unit_write,
    store_block_output_table_write,
    store_block_output_unit_write,
    store_block_delete,
    store_tx_index_self,
    store_tx_index_send,
    store_tx_index_recv,
    store_tx_index_confirm,
    store_tx_origin,
    store_block_meta_write,
    store_block_meta_read,

    store_dbsize_block_unit_empty,
    store_dbsize_block_unit_light,
    store_dbsize_block_unit_full,
    store_dbsize_block_table_empty,
    store_dbsize_block_table_light,
    store_dbsize_block_table_full,
    store_dbsize_block_other,

    // message category
    message_category_begin_contains_duplicate,
    message_category_consensus_contains_duplicate = message_category_begin_contains_duplicate,
    message_category_timer_contains_duplicate,
    message_category_txpool_contains_duplicate,
    message_category_rpc_contains_duplicate,
    message_category_sync_contains_duplicate,
    message_block_broadcast_contains_duplicate,
    message_category_end_contains_duplicate,
    message_category_unknown_contains_duplicate = message_category_end_contains_duplicate,

    message_category_recv,
    message_category_begin,
    message_category_consensus = message_category_begin,
    message_category_timer,
    message_category_txpool,
    message_category_rpc,
    message_category_sync,
    message_block_broadcast,
    message_category_unknown,
    message_category_end = message_category_unknown,

    message_category_send,
    message_send_category_begin,
    message_send_category_consensus = message_send_category_begin,
    message_send_category_timer,
    message_send_category_txpool,
    message_send_category_rpc,
    message_send_category_sync,
    message_send_block_broadcast,
    message_send_category_unknown,
    message_send_category_end = message_send_category_unknown,

    message_category_broad,
    message_broad_category_begin,
    message_broad_category_consensus = message_broad_category_begin,
    message_broad_category_timer,
    message_broad_category_txpool,
    message_broad_category_rpc,
    message_broad_category_sync,
    message_broad_block_broadcast,
    message_broad_category_unknown,
    message_broad_category_end = message_broad_category_unknown,

    message_category_rumor,
    message_rumor_category_begin,
    message_rumor_category_consensus = message_rumor_category_begin,
    message_rumor_category_timer,
    message_rumor_category_txpool,
    message_rumor_category_rpc,
    message_rumor_category_sync,
    message_rumor_block_broadcast,
    message_rumor_category_unknown,
    message_rumor_category_end = message_rumor_category_unknown,

    message_transport_recv,
    message_transport_send,

    // sync 
    xsync_recv_new_block,
    xsync_recv_new_hash,
    xsync_recv_invalid_block,
    xsync_recv_duplicate_block,
    xsync_recv_block_size,
    xsync_getblocks_recv_req,
    xsync_getblocks_send_resp,
    xsync_recv_broadcast_newblockhash,
    xsync_recv_blocks,
    xsync_recv_blocks_size,
    xsync_handler_blocks,
    xsync_recv_gossip_recv,
    xsync_bytes_gossip_recv,
    xsync_recv_get_on_demand_blocks,
    xsync_recv_get_on_demand_blocks_bytes,
    xsync_recv_on_demand_blocks,
    xsync_recv_on_demand_blocks_bytes,
    xsync_recv_broadcast_chain_state,
    xsync_recv_broadcast_chain_state_bytes,
    xsync_recv_response_chain_state,
    xsync_recv_response_chain_state_bytes,
    xsync_recv_cross_cluster_chain_state,
    xsync_recv_cross_cluster_chain_state_bytes,
    xsync_recv_blocks_by_hashes,
    xsync_recv_blocks_by_hashes_bytes,
    xsync_handler_blocks_by_hashes,
    xsync_cost_role_add_event,
    xsync_cost_role_remove_event,
    xsync_handle_chain_snapshot_request,
    xsync_handler_chain_snapshot_reponse,
    xsync_handle_ondemand_chain_snapshot_request,
    xsync_handle_ondemand_chain_snapshot_reponse,
    xsync_recv_on_demand_by_hash_blocks_req,
    xsync_recv_on_demand_by_hash_blocks_req_bytes,
    xsync_recv_on_demand_by_hash_blocks_resp,
    xsync_recv_on_demand_by_hash_blocks_resp_bytes,
    xsync_behind_download,
    xsync_behind_check,
    xsync_behind_on_demand,
    xsync_behind_on_demand_by_hash,
    xsync_recv_get_blocks_by_hashes,
    xsync_recv_get_blocks_by_hashes_bytes,
    xsync_store_block_units,
    xsync_store_block_tables,
    xsync_unit_proof_sync_req_send,
    xsync_unit_proof_sync_req_recv,


    // txpool
    txpool_received_self_send_receipt_num,
    txpool_received_other_send_receipt_num,
    txpool_recv_tx_retry_send,
    txpool_confirm_tx_retry_send,
    txpool_recv_tx_first_send,
    txpool_confirm_tx_first_send,
    txpool_request_origin_tx,
    txpool_receipt_tx,
    txpool_pull_recv_tx,
    txpool_pull_confirm_tx,
    txpool_push_tx_from_proposal,
    txpool_send_tx_cur,
    txpool_recv_tx_cur,
    txpool_confirm_tx_cur,
    txpool_unconfirm_tx_cur,
    txpool_recv_tx_first_send_fail,
    txpool_confirm_tx_first_send_fail,
    txpool_drop_send_receipt_msg,
    txpool_drop_receive_receipt_msg,
    txpool_drop_push_receipt_msg,
    txpool_drop_pull_recv_receipt_msg,
    txpool_drop_pull_confirm_receipt_msg_v2,
    txpool_drop_receipt_id_state_msg,
    txpool_try_sync_table_block,
    txpool_receipt_recv_num_by_1_clock,
    txpool_receipt_recv_num_by_2_clock,
    txpool_receipt_recv_num_by_3_clock,
    txpool_receipt_recv_num_by_4_clock,
    txpool_receipt_recv_num_by_5_clock,
    txpool_receipt_recv_num_by_6_clock,
    txpool_receipt_recv_num_7to12_clock,
    txpool_receipt_recv_num_13to30_clock,
    txpool_receipt_recv_num_exceed_30_clock,
    txpool_push_send_fail_table_limit,
    txpool_push_send_fail_role_limit,
    txpool_push_send_fail_repeat,
    txpool_push_send_fail_unconfirm_limit,
    txpool_push_send_fail_nonce_limit,
    txpool_push_send_fail_account_fall_behind,
    txpool_push_send_fail_account_not_in_charge,
    txpool_push_send_fail_nonce_expired,
    txpool_push_send_fail_nonce_duplicate,
    txpool_push_send_fail_replaced,
    txpool_push_send_fail_other,
    txpool_send_tx_timeout,
    txpool_tx_delay_from_push_to_pack_send,
    txpool_tx_delay_from_push_to_pack_recv,
    txpool_tx_delay_from_push_to_pack_confirm,
    txpool_tx_delay_from_push_to_commit_send,
    txpool_tx_delay_from_push_to_commit_recv,
    txpool_tx_delay_from_push_to_commit_confirm,
    txpool_receipt_id_state_msg_send_num,
    txpool_alarm_confirm_tx_reached_upper_limit,
    txpool_alarm_recv_tx_reached_upper_limit,
    txpool_alarm_send_tx_reached_upper_limit,

    // txstore
    txstore_request_origin_tx,
    txstore_cache_origin_tx,

    // blockstore
    blockstore_index_load,
    blockstore_blk_load,

    // blockstore accessing
    blockstore_access_from_account_context,
    blockstore_access_from_contract_runtime,

    // assess from bus
    blockstore_access_from_mbus,
    blockstore_access_from_mbus_begin,
    blockstore_access_from_mbus_onchain_loader_t_update = blockstore_access_from_mbus_begin,
    blockstore_access_from_mbus_contract_db_on_block,
    blockstore_access_from_mbus_txpool_db_event_on_block,
    blockstore_access_from_mbus_xelect_process_elect,
    blockstore_access_from_mbus_grpc_process_event,
    blockstore_access_from_mbus_end = blockstore_access_from_mbus_grpc_process_event,

    // access from rpc
    blockstore_access_from_rpc,
    blockstore_access_from_rpc_begin,
    blockstore_access_from_rpc_get_block = blockstore_access_from_rpc_begin,
    blockstore_access_from_rpc_get_committed_block,
    blockstore_access_from_rpc_get_chain_info,
    blockstore_access_from_rpc_get_latest_tables,
    blockstore_access_from_rpc_get_cert_blk,
    blockstore_access_from_rpc_get_timer_clock,
    blockstore_access_from_rpc_get_unit,
    blockstore_access_from_rpc_get_block_committed_block,
    blockstore_access_from_rpc_get_block_full_block,
    blockstore_access_from_rpc_get_block_by_height,
    blockstore_access_from_rpc_get_block_load_object,
    blockstore_access_from_rpc_get_block_committed_height,
    blockstore_access_from_rpc_get_block_query_propery,
    blockstore_access_from_rpc_get_block_set_table,
    blockstore_access_from_rpc_get_block_json,
    blockstore_access_from_rpc_end = blockstore_access_from_rpc_get_block_json,


    blockstore_access_from_store,
    // txpool
    blockstore_access_from_txpool,
    blockstore_access_from_txpool_begin,
    blockstore_access_from_txpool_on_block_event = blockstore_access_from_txpool_begin,
    blockstore_access_from_txpool_id_state,
    blockstore_access_from_txpool_get_nonce,
    blockstore_access_from_txpool_refresh_table,
    blockstore_access_from_txpool_create_receipt,
    blockstore_access_from_txpool_pull_lacking_receipts,
    blockstore_access_from_txpool_end = blockstore_access_from_txpool_pull_lacking_receipts,

    // blockstore access statestore
    blockstore_access_from_statestore,
    blockstore_access_from_statestore_begin,
    blockstore_access_from_statestore_rebuild_state = blockstore_access_from_statestore_begin,
    blockstore_access_from_statestore_fullblock,
    blockstore_access_from_statestore_load_state,
    blockstore_access_from_statestore_load_lastest_state,
    blockstore_access_from_statestore_get_block_state,
    blockstore_access_from_statestore_get_block_index_state,
    blockstore_access_from_statestore_get_connect_state,
    blockstore_access_from_statestore_get_commit_state,
    blockstore_access_from_statestore_end = blockstore_access_from_statestore_get_commit_state,

    blockstore_access_from_application,
    // sync access
    blockstore_access_from_sync,
    blockstore_access_from_sync_begin,
    blockstore_access_from_sync_blk = blockstore_access_from_sync_begin,
    blockstore_access_from_sync_get_latest_connected_block,
    blockstore_access_from_sync_get_latest_committed_block,
    blockstore_access_from_sync_get_latest_locked_block,
    blockstore_access_from_sync_get_latest_cert_block,
    blockstore_access_from_sync_existed_blk,
    blockstore_access_from_sync_update_latest_genesis_connected_block,
    blockstore_access_from_sync_get_latest_committed_full_block,
    blockstore_access_from_sync_get_latest_executed_block,
    blockstore_access_from_sync_get_genesis_block,
    blockstore_access_from_sync_load_block_objects,
    blockstore_access_from_sync_load_block_objects_input,
    blockstore_access_from_sync_load_block_objects_output,
    blockstore_access_from_sync_load_tx,
    blockstore_access_from_sync_load_tx_input,
    blockstore_access_from_sync_load_tx_output,
    blockstore_access_from_sync_store_blk,
    blockstore_access_from_sync_query_blk,
    blockstore_access_from_sync_load_block_object,
    blockstore_access_from_sync_end = blockstore_access_from_sync_load_block_object,


    blockstore_access_from_sync_index,

    // blockstore_access_from_blk_mk
    blockstore_access_from_blk_mk,
    blockstore_access_from_blk_mk_begin,
    blockstore_access_from_blk_mk_ld_and_cache = blockstore_access_from_blk_mk_begin,
    blockstore_access_from_blk_mk_proposer_verify_proposal,
    blockstore_access_from_blk_mk_proposer_verify_proposal_drand,
    blockstore_access_from_blk_mk_table,
    blockstore_access_from_blk_mk_unit_ld_last_blk,
    blockstore_access_from_blk_mk_unit_chk_last_state,
    blockstore_access_from_blk_mk_end = blockstore_access_from_blk_mk_unit_chk_last_state,

    // blockstore_access_from_us
    blockstore_access_from_us,
    blockstore_access_from_us_begin,
    blockstore_access_from_us_on_view_fire = blockstore_access_from_us_begin,
    blockstore_access_from_us_on_timer_fire,
    blockstore_access_from_us_on_proposal_finish,
    blockstore_access_from_us_timer_blk_maker,
    blockstore_access_from_us_timer_picker_constructor,
    blockstore_access_from_us_dispatcher_load_tc,
    blockstore_access_from_us_end = blockstore_access_from_us_dispatcher_load_tc,

    // blockstore_access_from_bft
    blockstore_access_from_bft,
    blockstore_access_from_bft_begin,
    blockstore_access_from_bft_check_proposal = blockstore_access_from_bft_begin,
    blockstore_access_from_bft_on_clock_fire,
    blockstore_access_from_bft_pdu_event_down,
    blockstore_access_from_bft_consaccnt_on_proposal_start,
    blockstore_access_from_bft_consdriv_on_proposal_start,
    blockstore_access_from_bft_get_commit_blk,
    blockstore_access_from_bft_get_lock_blk,
    blockstore_access_from_bft_sync,
    blockstore_access_from_bft_init_blk,
    blockstore_access_from_bft_end = blockstore_access_from_bft_init_blk,

    blockstore_access_from_vnodesrv,
    blockstore_tick,
    // statestore
    statestore_access,
    statestore_access_begin,
    statestore_access_from_blk_ctx = statestore_access_begin,
    statestore_access_from_vledger_load_state,
    statestore_access_from_vnodesrv_load_state,
    statestore_access_from_store_tgas,
    statestore_access_from_store_backup_tgas,
    statestore_access_from_store_bstate,
    statestore_access_from_xelect,
    statestore_access_from_xconfig_update,
    statestore_access_from_rpc_get_fullbock,
    statestore_access_from_rpc_query_propery,
    statestore_access_from_rpc_set_addition,
    statestore_access_from_rpc_set_fullunit,
    statestore_access_from_sync_chain_snapshot,
    statestore_access_from_sync_handle_chain_snapshot_meta,
    statestore_access_from_sync_query_offchain,
    statestore_access_from_application_isbeacon,
    statestore_access_from_application_load_election,
    statestore_access_from_blkmaker_update_account_state,
    statestore_access_from_blkmaker_unit_connect_state,
    statestore_access_from_txpool_get_accountstate,
    statestore_access_from_txpool_refreshtable,
    statestore_access_from_blockstore,
    statestore_access_from_blkmaker_get_target_tablestate,
    statestore_access_end = statestore_access_from_blkmaker_get_target_tablestate,

    statestore_get_unit_state_succ,
    statestore_get_unit_state_from_cache,
    statestore_get_unit_state_with_unit_count,
    statestore_get_table_state_succ,
    statestore_get_table_state_from_cache,
    statestore_get_table_state_with_table_count,

    state_load_blk_state_suc,
    state_load_blk_state_cache_suc,
    state_load_blk_state_fail,
    state_load_blk_state_table_suc,
    state_load_blk_state_table_fail,
    state_load_blk_state_table_cache_suc,
    state_load_blk_state_unit_suc,
    state_load_blk_state_unit_fail,
    state_load_blk_state_unit_cache_suc,

    // data structure
    data_table_unpack_units,
    data_table_unpack_one_unit,

    txexecutor_total_system_contract_count,
    txexecutor_system_contract_failed_count,

    // event
    xevent_begin,
    xevent_major_type_none = xevent_begin,
    xevent_major_type_timer,
    xevent_major_type_chain_timer,
    xevent_major_type_store,
    xevent_major_type_sync_executor,
    xevent_major_type_network,
    xevent_major_type_dispatch,      
    xevent_major_type_deceit,
    xevent_major_type_consensus,
    xevent_major_type_transaction,
    xevent_major_type_behind,
    xevent_major_type_vnode,
    xevent_major_type_account,
    xevent_major_type_role,
    xevent_major_type_blockfetcher,
    xevent_major_type_sync,
    xevent_end=xevent_major_type_sync,

    // rpc
    rpc_edge_tx_request,
    rpc_edge_query_request,
    rpc_auditor_tx_request,
    rpc_auditor_query_request,
    rpc_auditor_forward_request,
    rpc_validator_tx_request,
    // contract
    contract_table_fullblock_event,
    contract_table_statistic_exec_fullblock,
    contract_table_statistic_report_fullblock,
    contract_zec_slash_summarize_fullblock,

    // mailbox
    mailbox_grpc_total,
    mailbox_block_fetcher_total,
    mailbox_downloader_total,
    mailbox_xsync_total,
    mailbox_rpc_auditor_total,
    mailbox_rpc_validator_total,
    mailbox_txpool_fast_total,
    mailbox_txpool_slow_total,
    mailbox_us_total,
    mailbox_grpc_cur,
    mailbox_block_fetcher_cur,
    mailbox_downloader_cur,
    mailbox_xsync_cur,
    mailbox_rpc_auditor_cur,
    mailbox_rpc_validator_cur,
    mailbox_txpool_fast_cur,
    mailbox_txpool_slow_cur,
    mailbox_us_cur,

    //txdelay
    txdelay_client_timestamp_unmatch,
    txdelay_from_client_to_edge,
    txdelay_from_client_to_auditor,
    txdelay_from_client_to_validator,
    txdelay_from_client_to_sendtx_exec,
    txdelay_from_client_to_recvtx_exec,
    txdelay_from_client_to_confirmtx_exec,

    //cpu
    cpu_hash_256_calc,
    cpu_ca_merge_sign_xbft,
    cpu_ca_merge_sign_tc,
    cpu_ca_do_sign_xbft,
    cpu_ca_do_sign_tc,
    cpu_ca_verify_sign_xbft,
    cpu_ca_verify_sign_tc,
    cpu_ca_verify_multi_sign_txreceipt,
    cpu_ca_verify_multi_sign_sync,
    cpu_ca_verify_multi_sign_xbft,
    cpu_ca_verify_multi_sign_tc,
    cpu_ca_verify_multi_sign_blockstore,
    cpu_merkle_hash_calc,
    cpu_hash_256_xecprikey_calc,
    cpu_hash_256_XudpSocket_calc,
    cpu_hash_256_GetRootKadmliaKey_calc,
    cpu_hash_256_handle_register_node_calc,
    cpu_hash_256_xtransaction_v1_calc,
    cpu_hash_256_xtransaction_v2_calc,
    cpu_hash_256_receiptid_bin_calc,
    cpu_hash_256_xvproperty_prove_t_leafs_calc,
    cpu_hash_256_xvproperty_property_bin_calc,
    cpu_hash_256_xhashplugin_t_calc,

    //bft
    bft_verify_vote_msg_fail,

    e_simple_total,
};
using xmetrics_tag_t = E_SIMPLE_METRICS_TAG;

enum E_ARRAY_COUNTER_TAG : size_t {
    e_array_counter_begin = 0,

    blockstore_sharding_table_block_commit,
    blockstore_beacon_table_block_commit,
    blockstore_zec_table_block_commit,
    blockstore_sharding_table_block_full,
    blockstore_beacon_table_block_full,
    blockstore_zec_table_block_full,
    blockstore_sharding_table_block_genesis_connect,
    blockstore_beacon_table_block_genesis_connect,
    blockstore_zec_table_block_genesis_connect,

    e_array_counter_total,
};
using xmetrics_array_tag_t = E_ARRAY_COUNTER_TAG;

#ifdef ENABLE_METRICS
// ! attention. here the copy is not atomic.
template <typename T>
struct copiable_atomwrapper {
    std::atomic<T> _a;

    copiable_atomwrapper() : _a() {
    }

    copiable_atomwrapper(const std::atomic<T> & a) : _a(a.load()) {
    }

    copiable_atomwrapper(const copiable_atomwrapper & other) : _a(other._a.load()) {
    }

    copiable_atomwrapper & operator=(const copiable_atomwrapper & other) {
        _a.store(other._a.load());
        return *this;
    }

    copiable_atomwrapper & operator=(const T & val) {
        _a.store(val);
        return *this;
    }

    copiable_atomwrapper & operator++() {
        _a.store(_a.load() + 1);
        return *this;
    }
    copiable_atomwrapper operator++(int) {
        auto ret = *this;
        _a.store(_a.load() + 1);
        return ret;
    }
    copiable_atomwrapper operator+=(const T & val) {
        _a.store(_a.load() + val);
        return *this;
    }
    copiable_atomwrapper operator-=(const T & val) {
        _a.store(_a.load() - val);
        return *this;
    }

    T val() {
        return _a.load();
    }
};

class e_metrics : public top::xbasic_runnable_t<e_metrics> {
public:
    XDECLARE_DELETED_COPY_AND_MOVE_SEMANTICS(e_metrics);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(e_metrics);

    static e_metrics & get_instance() {
        static e_metrics instance;
        return instance;
    }

    void start(const std::string& log_path);
    void start() override;
    void stop() override;

private:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(e_metrics);
    void run_process();
    void process_message_queue();
    void update_dump();
    void gauge_dump();
    void array_count_dump();

public:
    void timer_start(std::string metrics_name, time_point value);
    void timer_end(std::string metrics_name, time_point value, metrics_appendant_info info = 0, microseconds timed_out = DEFAULT_TIMED_OUT);
    void counter_increase(std::string metrics_name, int64_t value);
    void counter_decrease(std::string metrics_name, int64_t value);
    void counter_set(std::string metrics_name, int64_t value);
    void flow_count(std::string metrics_name, int64_t value, time_point timestamp);
    void gauge(E_SIMPLE_METRICS_TAG tag, int64_t value);
    void gauge_set_value(E_SIMPLE_METRICS_TAG tag, int64_t value);
    int64_t gauge_get_value(E_SIMPLE_METRICS_TAG tag);
    void array_counter_increase(E_ARRAY_COUNTER_TAG tag, std::size_t index, int64_t value);
    void array_counter_decrease(E_ARRAY_COUNTER_TAG tag, std::size_t index, int64_t value);
    void array_counter_set(E_ARRAY_COUNTER_TAG tag, std::size_t index, int64_t value);

private:
    std::thread m_process_thread;
    std::size_t m_dump_interval;
    std::chrono::microseconds m_queue_procss_behind_sleep_time;
    handler::counter_handler_t m_counter_handler;
    handler::timer_handler_t m_timer_handler;
    handler::flow_handler_t m_flow_handler;
    handler::array_counter_handler_t m_array_counter_handler;
    constexpr static std::size_t message_queue_size{500000};
    top::threading::xthreadsafe_queue<event_message, std::vector<event_message>> m_message_queue{message_queue_size};
    std::map<std::string, metrics_variant_ptr> m_metrics_hub;  // {metrics_name, metrics_vaiant_ptr}
protected:
    struct simple_counter{
        std::atomic_long value;
        std::atomic_long call_count;
    };
    simple_counter s_counters[e_simple_total]; // simple counter counter
    metrics_variant_ptr s_metrics[e_simple_total]; // simple metrics dump info

    struct array_counter{
        std::vector<copiable_atomwrapper<int64_t>> arr_value;
        std::vector<copiable_atomwrapper<uint64_t>> arr_count;
    };
    array_counter a_counters[e_array_counter_total];
    metrics_variant_ptr a_metrics[e_array_counter_total];
};

class metrics_time_auto {
public:
    metrics_time_auto(std::string metrics_name, metrics_appendant_info key = std::string{"NOT SET appendant_info"}, microseconds timed_out = DEFAULT_TIMED_OUT)
      : m_metrics_name{metrics_name}, m_key{key}, m_timed_out{timed_out} {
        char c[15] = {0};
        snprintf(c, 14, "%p", (void*)(this));
        m_metrics_name = m_metrics_name + "&" + c;
        e_metrics::get_instance().timer_start(m_metrics_name, std::chrono::system_clock::now());
    }
    ~metrics_time_auto() { e_metrics::get_instance().timer_end(m_metrics_name, std::chrono::system_clock::now(), m_key, m_timed_out); }

    std::string m_metrics_name;
    metrics_appendant_info m_key;
    microseconds m_timed_out;
};

static uint64_t cpu_time() {
    struct timespec time;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
    return uint64_t(time.tv_sec) * 1000000 + uint64_t(time.tv_nsec) / 1000;
}

class metrics_cpu_time_auto {
public:
    metrics_cpu_time_auto(std::string metrics_name, metrics_appendant_info key = std::string{"NOT SET appendant_info"}, microseconds timed_out = DEFAULT_TIMED_OUT)
      : m_metrics_name{metrics_name}, m_key{key}, m_timed_out{timed_out} {
        char c[15] = {0};
        snprintf(c, 14, "%p", (void*)(this));
        m_metrics_name = m_metrics_name + "&" + c;
        e_metrics::get_instance().timer_start(m_metrics_name, time_point(std::chrono::microseconds{cpu_time()}));
    }
    ~metrics_cpu_time_auto() { e_metrics::get_instance().timer_end(m_metrics_name, time_point(std::chrono::microseconds{cpu_time()}), m_key, m_timed_out); }

    std::string m_metrics_name;
    metrics_appendant_info m_key;
    microseconds m_timed_out;
};

#define XMETRICS_INIT()                                                                                                                                                            \
    {                                                                                                                                                                              \
        auto & ins = top::metrics::e_metrics::get_instance();                                                                                                                      \
        ins.start();                                                                                                                                                               \
    }

#define XMETRICS_INIT2(log_path)                       \
    {                                                   \
        auto & ins = top::metrics::e_metrics::get_instance();\
        ins.start(log_path);                                          \
    }

#define XMETRICS_UNINT()                                                                                                                                                            \
    {                                                                                                                                                                              \
        auto & ins = top::metrics::e_metrics::get_instance();                                                                                                                      \
        ins.stop();                                                                                                                                                               \
    }

#define SSTR(x) static_cast<const std::ostringstream&>(std::ostringstream()  << x).str()
#define ADD_THREAD_HASH(metrics_name) SSTR(metrics_name) + "&0x" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()))
#define STR_CONCAT_IMPL(a, b) a##b
#define STR_CONCAT(str_a, str_b) STR_CONCAT_IMPL(str_a, str_b)

#define MICROSECOND(timeout)                                                                                                                                                       \
    std::chrono::microseconds { timeout }

#define XMETRICS_TIME_RECORD(metrics_name)                                                                                                                                         \
    top::metrics::metrics_time_auto STR_CONCAT(metrics_time_auto, __LINE__) { metrics_name }
#define XMETRICS_TIME_RECORD_KEY(metrics_name, key)                                                                                                                                \
    top::metrics::metrics_time_auto STR_CONCAT(metrics_time_auto, __LINE__) { metrics_name, key }
#define XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT(metrics_name, key, timeout)                                                                                                             \
    top::metrics::metrics_time_auto STR_CONCAT(metrics_time_auto, __LINE__) { metrics_name, key, MICROSECOND(timeout) }

#define XMETRICS_CPU_TIME_RECORD(metrics_name)                                                                                                                                         \
    top::metrics::metrics_cpu_time_auto STR_CONCAT(metrics_cpu_time_auto, __LINE__) { metrics_name }
#define XMETRICS_CPU_TIME_RECORD_KEY(metrics_name, key)                                                                                                                                \
    top::metrics::metrics_cpu_time_auto STR_CONCAT(metrics_cpu_time_auto, __LINE__) { metrics_name, key }
#define XMETRICS_CPU_TIME_RECORD_KEY_WITH_TIMEOUT(metrics_name, key, timeout)                                                                                                             \
    top::metrics::metrics_cpu_time_auto STR_CONCAT(metrics_cpu_time_auto, __LINE__) { metrics_name, key, MICROSECOND(timeout) }

#define XMETRICS_TIMER_START(metrics_name) top::metrics::e_metrics::get_instance().timer_start(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now());

#define XMETRICS_TIMER_STOP(metrics_name) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now());

#define XMETRICS_TIMER_STOP_KEY(metrics_name, key) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now(), key);

#define XMETRICS_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)                                                                                                              \
    top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now(), key, MICROSECOND(timeout));

#define XMETRICS_CPU_TIMER_START(metrics_name) top::metrics::e_metrics::get_instance().timer_start(ADD_THREAD_HASH(metrics_name), time_point(std::chrono::microseconds{cpu_time()}));

#define XMETRICS_CPU_TIMER_STOP(metrics_name) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), time_point(std::chrono::microseconds{cpu_time()}));

#define XMETRICS_CPU_TIMER_STOP_KEY(metrics_name, key) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), time_point(std::chrono::microseconds{cpu_time()}), key);

#define XMETRICS_CPU_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)                                                                                                              \
    top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), time_point(std::chrono::microseconds{cpu_time()}), key, MICROSECOND(timeout));


#define XMETRICS_COUNTER_INCREMENT(metrics_name, value) top::metrics::e_metrics::get_instance().counter_increase(metrics_name, value)
#define XMETRICS_COUNTER_DECREMENT(metrics_name, value) top::metrics::e_metrics::get_instance().counter_decrease(metrics_name, value)
#define XMETRICS_COUNTER_SET(metrics_name, value) top::metrics::e_metrics::get_instance().counter_set(metrics_name, value)

#define XMETRICS_FLOW_COUNT(metrics_name, value) top::metrics::e_metrics::get_instance().flow_count(metrics_name, value, std::chrono::system_clock::now());

#define XMETRICS_PACKET_INFO(metrics_name, ...)                                                                                                                                \
    top::metrics::handler::metrics_pack_unit STR_CONCAT(packet_info_auto_, __LINE__){metrics_name, "real_time"};                                                               \
    top::metrics::handler::metrics_packet_impl(STR_CONCAT(packet_info_auto_, __LINE__), __VA_ARGS__);

#define XMETRICS_PACKET_ALARM(metrics_name, ...)                                                                                                                               \
    top::metrics::handler::metrics_pack_unit STR_CONCAT(packet_info_auto_, __LINE__){metrics_name, "alarm"};                                                                   \
    top::metrics::handler::metrics_packet_impl(STR_CONCAT(packet_info_auto_, __LINE__), __VA_ARGS__);


#define XMETRICS_GAUGE(TAG, value) top::metrics::e_metrics::get_instance().gauge(TAG, value)
#define XMETRICS_GAUGE_SET_VALUE(TAG, value) top::metrics::e_metrics::get_instance().gauge_set_value(TAG, value)
#define XMETRICS_GAUGE_GET_VALUE(TAG) top::metrics::e_metrics::get_instance().gauge_get_value(TAG)

#ifndef ENABLE_METRICS_DATAOBJECT                                                                             
    #define XMETRICS_GAUGE_DATAOBJECT(TAG, value)
#else
    #define XMETRICS_GAUGE_DATAOBJECT(TAG, value)   XMETRICS_GAUGE(TAG, value) 
#endif

class simple_metrics_tickcounter {
public:
    simple_metrics_tickcounter(E_SIMPLE_METRICS_TAG tag) : m_tag(tag) {
        m_start = cpu_time();
    }

    ~simple_metrics_tickcounter() {
        uint64_t end = cpu_time();
        top::metrics::e_metrics::get_instance().gauge(m_tag, end - m_start);
    }
private:
    E_SIMPLE_METRICS_TAG m_tag;
    uint64_t m_start;
};

#define XMETRICS_TIMER(tag) \
    top::metrics::simple_metrics_tickcounter tick(tag); // micro seconds

#define XMETRICS_ARRCNT_INCR(metrics_name, index, value) top::metrics::e_metrics::get_instance().array_counter_increase(metrics_name, index, value)
#define XMETRICS_ARRCNT_DECR(metrics_name, index, value) top::metrics::e_metrics::get_instance().array_counter_decrease(metrics_name, index, value)
#define XMETRICS_ARRCNT_SET(metrics_name, index, value) top::metrics::e_metrics::get_instance().array_counter_set(metrics_name, index, value)

#else
#define XMETRICS_INIT()
#define XMETRICS_INIT2(log_path) 
#define XMETRICS_UNINT()
#define XMETRICS_TIME_RECORD(metrics_name)
#define XMETRICS_TIME_RECORD_KEY(metrics_name, key)
#define XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_CPU_TIME_RECORD(metrics_name)
#define XMETRICS_CPU_TIME_RECORD_KEY(metrics_name, key)
#define XMETRICS_CPU_TIME_RECORD_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_TIMER_START(metrics_name)
#define XMETRICS_TIMER_STOP(metrics_name)
#define XMETRICS_TIMER_STOP_KEY(metrics_name, key)
#define XMETRICS_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_CPU_TIMER_START(metrics_name)
#define XMETRICS_CPU_TIMER_STOP(metrics_name)
#define XMETRICS_CPU_TIMER_STOP_KEY(metrics_name, key)
#define XMETRICS_CPU_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_COUNTER_INCREMENT(metrics_name, value)
#define XMETRICS_COUNTER_DECREMENT(metrics_name, value)
#define XMETRICS_COUNTER_SET(metrics_name, value)
#define XMETRICS_FLOW_COUNT(metrics_name, value)
#define XMETRICS_PACKET_INFO(metrics_name, ...)
#define XMETRICS_PACKET_ALARM(metrics_name, ...)
#define XMETRICS_GAUGE(TAG, value)
#define XMETRICS_GAUGE_DATAOBJECT(TAG, value)
#define XMETRICS_GAUGE_SET_VALUE(TAG, value)
#define XMETRICS_GAUGE_GET_VALUE(TAG)
#define XMETRICS_ARRCNT_INCR(metrics_name, index, value)
#define XMETRICS_ARRCNT_DECR(metrics_name, index, value)
#define XMETRICS_ARRCNT_SET(metrics_name, index, value)
#define XMETRICS_TIMER(tag)
#endif

NS_END2
