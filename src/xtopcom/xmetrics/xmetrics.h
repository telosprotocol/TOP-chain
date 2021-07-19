// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "metrics_handler/counter_handler.h"
#include "metrics_handler/flow_handler.h"
#include "metrics_handler/timer_handler.h"
#include "metrics_handler/xmetrics_packet_info.h"
#include "xbasic/xrunnable.h"
#include "xbasic/xthreading/xthreadsafe_queue.hpp"
#include "xmetrics_event.h"
#include "xmetrics_unit.h"

#include <chrono>
#include <map>
#include <string>
#include <thread>
NS_BEG2(top, metrics)

enum E_SIMPLE_METRICS_TAG : size_t {
    e_simple_begin = 0,
    blockstore_cache_block_total = e_simple_begin+1,
    dataobject_cur_xbase_type_cons_transaction,
    vhost_recv_msg,
    vhost_recv_callback,
    vnode_recv_msg,
    vnode_recv_callback,
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
    db_key_block_offdata,

    // consensus
    cons_drand_leader_finish_succ,
    cons_drand_backup_finish_succ,
    cons_drand_leader_finish_fail,
    cons_drand_backup_finish_fail,
    cons_tableblock_leader_finish_succ,
    cons_tableblock_backup_finish_succ,
    cons_tableblock_leader_finish_fail,
    cons_tableblock_backup_finish_fail,
    cons_pacemaker_tc_discontinuity,
    cons_fail_make_proposal_table_state,
    cons_fail_make_proposal_consensus_para,
    cons_fail_verify_proposal_blocks_invalid,
    cons_fail_verify_proposal_table_state_get,
    cons_fail_verify_proposal_drand_invalid,
    cons_fail_verify_proposal_consensus_para_get,
    cons_fail_verify_proposal_unit_count,
    cons_fail_make_proposal_table_check_latest_state,
    cons_fail_verify_proposal_table_check_latest_state,
    cons_fail_verify_proposal_table_with_local,
    cons_fail_make_proposal_unit_check_state,
    cons_fail_make_proposal_view_changed,
    cons_view_fire_clock_delay,
    cons_fail_backup_view_not_match,

    // store
    store_db_read,
    store_db_write,
    store_db_delete,
    store_state_read,
    store_state_write,
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
    store_block_index_write,
    store_block_input_write,
    store_block_output_write,
    store_block_delete,
    store_tx_index_self,
    store_tx_index_send,
    store_tx_index_recv,
    store_tx_index_confirm,
    store_tx_origin,

    // vledger dataobject
    dataobject_xvnode_t,
    dataobject_xvexestate_t,
    dataobject_xvnodegroup,
    dataobject_xcscoreobj_t,
    dataobject_xblock_maker_t,
    dataobject_xblockacct_t,
    dataobject_xtxpool_table_info_t,
    dataobject_xacctmeta_t,

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

    message_category_begin,
    message_category_consensus = message_category_begin,
    message_category_timer,
    message_category_txpool,
    message_category_rpc,
    message_category_sync,
    message_block_broadcast,
    message_category_end,
    message_category_unknown = message_category_end,

    // sync 
    xsync_recv_new_block,
    xsync_recv_new_hash,
    xsync_recv_invalid_block,
    xsync_recv_duplicate_block,
    xsync_recv_block_size,

    // txpool
    txpool_received_self_send_receipt_num,
    txpool_received_other_send_receipt_num,
    txpool_recv_tx_retry_send,
    txpool_confirm_tx_retry_send,
    txpool_recv_tx_first_send,
    txpool_confirm_tx_first_send,
    txpool_push_tx_send_fail_pool_full,
    txpool_pull_recv_tx,
    txpool_pull_confirm_tx,
    txpool_push_tx_from_proposal,

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
    blockstore_access_from_txpool_create_confirm_receipt,
    blockstore_access_from_txpool_sync_status,
    blockstore_access_from_txpool_recover,
    blockstore_access_from_txpool_refresh_table,
    blockstore_access_from_txpool_end = blockstore_access_from_txpool_refresh_table,

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
    statestore_access_from_blkmaker_get_target_tablestate,
    statestore_access_end = statestore_access_from_blkmaker_get_target_tablestate,

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

    e_simple_total,
};
using xmetircs_tag_t = E_SIMPLE_METRICS_TAG;

class e_metrics : public top::xbasic_runnable_t<e_metrics> {
public:
    XDECLARE_DELETED_COPY_AND_MOVE_SEMANTICS(e_metrics);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(e_metrics);

    static e_metrics & get_instance() {
        static e_metrics instance;
        return instance;
    }

    void start() override;
    void stop() override;

private:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(e_metrics);
    void run_process();
    void process_message_queue();
    void update_dump();
    void gauge_dump();

public:
    void timer_start(std::string metrics_name, time_point value);
    void timer_end(std::string metrics_name, time_point value, metrics_appendant_info info = 0, microseconds timed_out = DEFAULT_TIMED_OUT);
    void counter_increase(std::string metrics_name, int64_t value);
    void counter_decrease(std::string metrics_name, int64_t value);
    void counter_set(std::string metrics_name, int64_t value);
    void flow_count(std::string metrics_name, int64_t value, time_point timestamp);
    void gauge(E_SIMPLE_METRICS_TAG tag, int64_t value);

private:
    std::thread m_process_thread;
    std::size_t m_dump_interval;
    std::chrono::microseconds m_queue_procss_behind_sleep_time;
    handler::counter_handler_t m_counter_handler;
    handler::timer_handler_t m_timer_handler;
    handler::flow_handler_t m_flow_handler;
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

#ifdef ENABLE_METRICS
#define XMETRICS_INIT()                                                                                                                                                            \
    {                                                                                                                                                                              \
        auto & ins = top::metrics::e_metrics::get_instance();                                                                                                                      \
        ins.start();                                                                                                                                                               \
    }

#define SSTR(x) static_cast<std::ostringstream &>((std::ostringstream()  << x)).str()
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

#define XMETRICS_TIMER_START(metrics_name) top::metrics::e_metrics::get_instance().timer_start(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now());

#define XMETRICS_TIMER_STOP(metrics_name) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now());

#define XMETRICS_TIMER_STOP_KEY(metrics_name, key) top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now(), key);

#define XMETRICS_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)                                                                                                              \
    top::metrics::e_metrics::get_instance().timer_end(ADD_THREAD_HASH(metrics_name), std::chrono::system_clock::now(), key, MICROSECOND(timeout));

#define XMETRICS_COUNTER_INCREMENT(metrics_name, value) top::metrics::e_metrics::get_instance().counter_increase(metrics_name, value)
#define XMETRICS_COUNTER_DECREMENT(metrics_name, value) top::metrics::e_metrics::get_instance().counter_decrease(metrics_name, value)
#define XMETRICS_COUNTER_SET(metrics_name, value) top::metrics::e_metrics::get_instance().counter_set(metrics_name, value)

#define XMETRICS_FLOW_COUNT(metrics_name, value) top::metrics::e_metrics::get_instance().flow_count(metrics_name, value, std::chrono::system_clock::now());

#define XMETRICS_PACKET_INFO(metrics_name, ...)                                                                                                                                    \
    top::metrics::handler::metrics_pack_unit STR_CONCAT(packet_info_auto_, __LINE__){metrics_name};                                                                                                       \
    top::metrics::handler::metrics_packet_impl(STR_CONCAT(packet_info_auto_, __LINE__), __VA_ARGS__);

#define XMETRICS_XBASE_DATA_CATEGORY_NEW(key) XMETRICS_COUNTER_INCREMENT("dataobject_cur_xbase_type" + std::to_string(key), 1);
#define XMETRICS_XBASE_DATA_CATEGORY_DELETE(key) XMETRICS_COUNTER_INCREMENT("dataobject_cur_xbase_type" + std::to_string(key), -1);
#define XMETRICS_GAUGE(TAG, value) top::metrics::e_metrics::get_instance().gauge(TAG, value)
#else
#define XMETRICS_INIT()
#define XMETRICS_TIME_RECORD(metrics_name)
#define XMETRICS_TIME_RECORD_KEY(metrics_name, key)
#define XMETRICS_TIME_RECORD_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_TIMER_START(metrics_name)
#define XMETRICS_TIMER_STOP(metrics_name)
#define XMETRICS_TIMER_STOP_KEY(metrics_name, key)
#define XMETRICS_TIMER_STOP_KEY_WITH_TIMEOUT(metrics_name, key, timeout)
#define XMETRICS_COUNTER_INCREMENT(metrics_name, value)
#define XMETRICS_COUNTER_DECREMENT(metrics_name, value)
#define XMETRICS_COUNTER_SET(metrics_name, value)
#define XMETRICS_FLOW_COUNT(metrics_name, value)
#define XMETRICS_PACKET_INFO(metrics_name, ...)
#define XMETRICS_XBASE_DATA_CATEGORY_NEW(key)
#define XMETRICS_XBASE_DATA_CATEGORY_DELETE(key)
#define XMETRICS_GAUGE(TAG, value)
#endif

NS_END2
