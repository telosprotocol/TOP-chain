// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xmetrics.h"

NS_BEG2(top, metrics)

#define RETURN_METRICS_NAME(TAG) case TAG: return #TAG
char const * matrics_name(xmetircs_tag_t const tag) noexcept {
    switch (tag) {
        RETURN_METRICS_NAME(e_simple_begin);
        RETURN_METRICS_NAME(blockstore_cache_block_total);
        RETURN_METRICS_NAME(dataobject_cur_xbase_type_cons_transaction);
        RETURN_METRICS_NAME(vhost_recv_msg);
        RETURN_METRICS_NAME(vhost_recv_callback);
        RETURN_METRICS_NAME(vnode_recv_msg);
        RETURN_METRICS_NAME(vnode_recv_callback);

        // dataobjec
        RETURN_METRICS_NAME(dataobject_tx_receipt_t);
        RETURN_METRICS_NAME(dataobject_unit_state);
        RETURN_METRICS_NAME(dataobject_xvtxindex);
        RETURN_METRICS_NAME(dataobject_xvbstate);
        RETURN_METRICS_NAME(dataobject_xvproperty);
        RETURN_METRICS_NAME(dataobject_xvaccountobj);
        RETURN_METRICS_NAME(dataobject_exeunit);
        RETURN_METRICS_NAME(dataobject_exegroup);
        RETURN_METRICS_NAME(dataobject_xvexecontxt);
        RETURN_METRICS_NAME(dataobject_xaccount_index);
        RETURN_METRICS_NAME(dataobject_xreceiptid_pair_t);
        RETURN_METRICS_NAME(dataobject_xvbindex_t);
        RETURN_METRICS_NAME(dataobject_xtransaction_t);
        RETURN_METRICS_NAME(dataobject_provcert);
        RETURN_METRICS_NAME(dataobject_xvaccount);
        RETURN_METRICS_NAME(dataobject_xvaction);
        RETURN_METRICS_NAME(dataobject_xvheader);
        RETURN_METRICS_NAME(dataobject_xvqcert);
        RETURN_METRICS_NAME(dataobject_xvblock);
        RETURN_METRICS_NAME(dataobject_xvinput);
        RETURN_METRICS_NAME(dataobject_xvoutput);
        RETURN_METRICS_NAME(dataobject_xventity);

        // dbkeys
        RETURN_METRICS_NAME(db_key_tx);
        RETURN_METRICS_NAME(db_key_block_index);
        RETURN_METRICS_NAME(db_key_block);
        RETURN_METRICS_NAME(db_key_block_object);
        RETURN_METRICS_NAME(db_key_block_input);
        RETURN_METRICS_NAME(db_key_block_input_resource);
        RETURN_METRICS_NAME(db_key_block_output);
        RETURN_METRICS_NAME(db_key_block_output_resource);
        RETURN_METRICS_NAME(db_key_block_state);
        RETURN_METRICS_NAME(db_key_block_offdata);

        // consensus
        RETURN_METRICS_NAME(cons_drand_leader_finish_succ);
        RETURN_METRICS_NAME(cons_drand_backup_finish_succ);
        RETURN_METRICS_NAME(cons_drand_leader_finish_fail);
        RETURN_METRICS_NAME(cons_drand_backup_finish_fail);
        RETURN_METRICS_NAME(cons_tableblock_leader_finish_succ);
        RETURN_METRICS_NAME(cons_tableblock_backup_finish_succ);
        RETURN_METRICS_NAME(cons_tableblock_leader_finish_fail);
        RETURN_METRICS_NAME(cons_tableblock_backup_finish_fail);
        RETURN_METRICS_NAME(cons_pacemaker_tc_discontinuity);
        RETURN_METRICS_NAME(cons_fail_make_proposal_table_state);
        RETURN_METRICS_NAME(cons_fail_make_proposal_consensus_para);
        RETURN_METRICS_NAME(cons_fail_verify_proposal_blocks_invalid);
        RETURN_METRICS_NAME(cons_fail_verify_proposal_table_state_get);
        RETURN_METRICS_NAME(cons_fail_verify_proposal_drand_invalid);
        RETURN_METRICS_NAME(cons_fail_verify_proposal_consensus_para_get);
        RETURN_METRICS_NAME(cons_fail_verify_proposal_unit_count);
        RETURN_METRICS_NAME(cons_fail_make_proposal_table_check_latest_state);
        RETURN_METRICS_NAME(cons_fail_verify_proposal_table_check_latest_state);
        RETURN_METRICS_NAME(cons_fail_verify_proposal_table_with_local);
        RETURN_METRICS_NAME(cons_fail_make_proposal_unit_check_state);
        RETURN_METRICS_NAME(cons_fail_make_proposal_view_changed);
        RETURN_METRICS_NAME(cons_view_fire_clock_delay);
        RETURN_METRICS_NAME(cons_fail_backup_view_not_match);

        // store
        RETURN_METRICS_NAME(store_db_read);
        RETURN_METRICS_NAME(store_db_write);
        RETURN_METRICS_NAME(store_db_delete);
        RETURN_METRICS_NAME(store_state_read);
        RETURN_METRICS_NAME(store_state_write);
        RETURN_METRICS_NAME(store_state_delete);
        RETURN_METRICS_NAME(store_block_table_read);
        RETURN_METRICS_NAME(store_block_unit_read);
        RETURN_METRICS_NAME(store_block_other_read);
        RETURN_METRICS_NAME(store_block_index_read);
        RETURN_METRICS_NAME(store_block_input_read);
        RETURN_METRICS_NAME(store_block_output_read);
        RETURN_METRICS_NAME(store_block_call);
        RETURN_METRICS_NAME(store_block_table_write);
        RETURN_METRICS_NAME(store_block_unit_write);
        RETURN_METRICS_NAME(store_block_other_write);
        RETURN_METRICS_NAME(store_block_index_write);
        RETURN_METRICS_NAME(store_block_input_write);
        RETURN_METRICS_NAME(store_block_output_write);
        RETURN_METRICS_NAME(store_block_delete);
        RETURN_METRICS_NAME(store_tx_index_self);
        RETURN_METRICS_NAME(store_tx_index_send);
        RETURN_METRICS_NAME(store_tx_index_recv);
        RETURN_METRICS_NAME(store_tx_index_confirm);
        RETURN_METRICS_NAME(store_tx_origin);

        // vledger dataobject
        RETURN_METRICS_NAME(dataobject_xvnode_t);
        RETURN_METRICS_NAME(dataobject_xvexestate_t);
        RETURN_METRICS_NAME(dataobject_xvnodegroup);
        RETURN_METRICS_NAME(dataobject_xcscoreobj_t);
        RETURN_METRICS_NAME(dataobject_xblock_maker_t);
        RETURN_METRICS_NAME(dataobject_xblockacct_t);
        RETURN_METRICS_NAME(dataobject_xtxpool_table_info_t);
        RETURN_METRICS_NAME(dataobject_xacctmeta_t);

        // message category
        RETURN_METRICS_NAME(message_category_consensus_contains_duplicate);
        RETURN_METRICS_NAME(message_category_timer_contains_duplicate);
        RETURN_METRICS_NAME(message_category_txpool_contains_duplicate);
        RETURN_METRICS_NAME(message_category_rpc_contains_duplicate);
        RETURN_METRICS_NAME(message_category_sync_contains_duplicate);
        RETURN_METRICS_NAME(message_block_broadcast_contains_duplicate);
        RETURN_METRICS_NAME(message_category_unknown_contains_duplicate);

        RETURN_METRICS_NAME(message_category_consensus);
        RETURN_METRICS_NAME(message_category_timer);
        RETURN_METRICS_NAME(message_category_txpool);
        RETURN_METRICS_NAME(message_category_rpc);
        RETURN_METRICS_NAME(message_category_sync);
        RETURN_METRICS_NAME(message_block_broadcast);
        RETURN_METRICS_NAME(message_category_unknown);

        // sync 
        RETURN_METRICS_NAME(xsync_recv_new_block);
        RETURN_METRICS_NAME(xsync_recv_new_hash);
        RETURN_METRICS_NAME(xsync_recv_invalid_block);
        RETURN_METRICS_NAME(xsync_recv_duplicate_block);
        RETURN_METRICS_NAME(xsync_recv_block_size);

        // txpool
        RETURN_METRICS_NAME(txpool_received_self_send_receipt_num);
        RETURN_METRICS_NAME(txpool_received_other_send_receipt_num);
        RETURN_METRICS_NAME(txpool_recv_tx_retry_send);
        RETURN_METRICS_NAME(txpool_confirm_tx_retry_send);
        RETURN_METRICS_NAME(txpool_recv_tx_first_send);
        RETURN_METRICS_NAME(txpool_confirm_tx_first_send);
        RETURN_METRICS_NAME(txpool_push_tx_send_fail_pool_full);
        RETURN_METRICS_NAME(txpool_pull_recv_tx);
        RETURN_METRICS_NAME(txpool_pull_confirm_tx);
        RETURN_METRICS_NAME(txpool_push_tx_from_proposal);

        // blockstore
        RETURN_METRICS_NAME(blockstore_index_load);
        RETURN_METRICS_NAME(blockstore_blk_load);

        // blockstore accessing
        RETURN_METRICS_NAME(blockstore_access_from_account_context);
        RETURN_METRICS_NAME(blockstore_access_from_contract_runtime);

        // access from mbus
        RETURN_METRICS_NAME(blockstore_access_from_mbus);
        RETURN_METRICS_NAME(blockstore_access_from_mbus_onchain_loader_t_update);
        RETURN_METRICS_NAME(blockstore_access_from_mbus_contract_db_on_block);
        RETURN_METRICS_NAME(blockstore_access_from_mbus_txpool_db_event_on_block);
        RETURN_METRICS_NAME(blockstore_access_from_mbus_xelect_process_elect);
        RETURN_METRICS_NAME(blockstore_access_from_mbus_grpc_process_event);

        // rpc access
        RETURN_METRICS_NAME(blockstore_access_from_rpc);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_block);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_committed_block);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_chain_info);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_latest_tables);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_cert_blk);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_timer_clock);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_unit);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_block_committed_block);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_block_full_block);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_block_by_height);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_block_load_object);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_block_committed_height);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_block_query_propery);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_block_set_table);
        RETURN_METRICS_NAME(blockstore_access_from_rpc_get_block_json);

        RETURN_METRICS_NAME(blockstore_access_from_store);

        // txpool access
        RETURN_METRICS_NAME(blockstore_access_from_txpool);
        RETURN_METRICS_NAME(blockstore_access_from_txpool_on_block_event);
        RETURN_METRICS_NAME(blockstore_access_from_txpool_create_confirm_receipt);
        RETURN_METRICS_NAME(blockstore_access_from_txpool_sync_status);
        RETURN_METRICS_NAME(blockstore_access_from_txpool_recover);
        RETURN_METRICS_NAME(blockstore_access_from_txpool_refresh_table);

        // blockstore access statestore
        RETURN_METRICS_NAME(blockstore_access_from_statestore);
        RETURN_METRICS_NAME(blockstore_access_from_statestore_rebuild_state);
        RETURN_METRICS_NAME(blockstore_access_from_statestore_fullblock);
        RETURN_METRICS_NAME(blockstore_access_from_statestore_load_state);
        RETURN_METRICS_NAME(blockstore_access_from_statestore_load_lastest_state);
        RETURN_METRICS_NAME(blockstore_access_from_statestore_get_block_state);
        RETURN_METRICS_NAME(blockstore_access_from_statestore_get_block_index_state);
        RETURN_METRICS_NAME(blockstore_access_from_statestore_get_connect_state);
        RETURN_METRICS_NAME(blockstore_access_from_statestore_get_commit_state);
        
        RETURN_METRICS_NAME(blockstore_access_from_application);

        // sync access
        RETURN_METRICS_NAME(blockstore_access_from_sync);
        RETURN_METRICS_NAME(blockstore_access_from_sync_blk);
        RETURN_METRICS_NAME(blockstore_access_from_sync_get_latest_connected_block);
        RETURN_METRICS_NAME(blockstore_access_from_sync_get_latest_committed_block);
        RETURN_METRICS_NAME(blockstore_access_from_sync_get_latest_locked_block);
        RETURN_METRICS_NAME(blockstore_access_from_sync_get_latest_cert_block);
        RETURN_METRICS_NAME(blockstore_access_from_sync_existed_blk);
        RETURN_METRICS_NAME(blockstore_access_from_sync_update_latest_genesis_connected_block);
        RETURN_METRICS_NAME(blockstore_access_from_sync_get_latest_committed_full_block);
        RETURN_METRICS_NAME(blockstore_access_from_sync_get_latest_executed_block);
        RETURN_METRICS_NAME(blockstore_access_from_sync_get_genesis_block);
        RETURN_METRICS_NAME(blockstore_access_from_sync_load_block_objects);
        RETURN_METRICS_NAME(blockstore_access_from_sync_load_block_objects_input);
        RETURN_METRICS_NAME(blockstore_access_from_sync_load_block_objects_output);
        RETURN_METRICS_NAME(blockstore_access_from_sync_load_tx);
        RETURN_METRICS_NAME(blockstore_access_from_sync_load_tx_input);
        RETURN_METRICS_NAME(blockstore_access_from_sync_load_tx_output);
        RETURN_METRICS_NAME(blockstore_access_from_sync_store_blk);
        RETURN_METRICS_NAME(blockstore_access_from_sync_query_blk);
        RETURN_METRICS_NAME(blockstore_access_from_sync_load_block_object);
        
        RETURN_METRICS_NAME(blockstore_access_from_sync_index);

        // blockstore_access_from_blk_mk
        RETURN_METRICS_NAME(blockstore_access_from_blk_mk);
        RETURN_METRICS_NAME(blockstore_access_from_blk_mk_ld_and_cache);
        RETURN_METRICS_NAME(blockstore_access_from_blk_mk_proposer_verify_proposal);
        RETURN_METRICS_NAME(blockstore_access_from_blk_mk_proposer_verify_proposal_drand);
        RETURN_METRICS_NAME(blockstore_access_from_blk_mk_table);
        RETURN_METRICS_NAME(blockstore_access_from_blk_mk_unit_ld_last_blk);
        RETURN_METRICS_NAME(blockstore_access_from_blk_mk_unit_chk_last_state);

        RETURN_METRICS_NAME(blockstore_access_from_us);
        RETURN_METRICS_NAME(blockstore_access_from_us_on_view_fire);
        RETURN_METRICS_NAME(blockstore_access_from_us_on_timer_fire);
        RETURN_METRICS_NAME(blockstore_access_from_us_on_proposal_finish);
        RETURN_METRICS_NAME(blockstore_access_from_us_timer_blk_maker);
        RETURN_METRICS_NAME(blockstore_access_from_us_timer_picker_constructor);
        RETURN_METRICS_NAME(blockstore_access_from_us_dispatcher_load_tc);

        RETURN_METRICS_NAME(blockstore_access_from_bft);
        RETURN_METRICS_NAME(blockstore_access_from_bft_check_proposal);
        RETURN_METRICS_NAME(blockstore_access_from_bft_on_clock_fire);
        RETURN_METRICS_NAME(blockstore_access_from_bft_pdu_event_down);
        RETURN_METRICS_NAME(blockstore_access_from_bft_consaccnt_on_proposal_start);
        RETURN_METRICS_NAME(blockstore_access_from_bft_consdriv_on_proposal_start);
        RETURN_METRICS_NAME(blockstore_access_from_bft_get_commit_blk);
        RETURN_METRICS_NAME(blockstore_access_from_bft_get_lock_blk);
        RETURN_METRICS_NAME(blockstore_access_from_bft_sync);
        RETURN_METRICS_NAME(blockstore_access_from_bft_init_blk);

        RETURN_METRICS_NAME(blockstore_access_from_vnodesrv);

        // state store
        RETURN_METRICS_NAME(statestore_access);
        RETURN_METRICS_NAME(statestore_access_from_blk_ctx);
        RETURN_METRICS_NAME(statestore_access_from_vledger_load_state);
        RETURN_METRICS_NAME(statestore_access_from_vnodesrv_load_state);
        RETURN_METRICS_NAME(statestore_access_from_store_tgas);
        RETURN_METRICS_NAME(statestore_access_from_store_backup_tgas);
        RETURN_METRICS_NAME(statestore_access_from_store_bstate);
        RETURN_METRICS_NAME(statestore_access_from_xelect);
        RETURN_METRICS_NAME(statestore_access_from_xconfig_update);
        RETURN_METRICS_NAME(statestore_access_from_rpc_get_fullbock);
        RETURN_METRICS_NAME(statestore_access_from_rpc_query_propery);
        RETURN_METRICS_NAME(statestore_access_from_rpc_set_addition);
        RETURN_METRICS_NAME(statestore_access_from_rpc_set_fullunit);
        RETURN_METRICS_NAME(statestore_access_from_sync_chain_snapshot);
        RETURN_METRICS_NAME(statestore_access_from_sync_handle_chain_snapshot_meta);
        RETURN_METRICS_NAME(statestore_access_from_sync_query_offchain);
        RETURN_METRICS_NAME(statestore_access_from_application_isbeacon);
        RETURN_METRICS_NAME(statestore_access_from_application_load_election);
        RETURN_METRICS_NAME(statestore_access_from_blkmaker_update_account_state);
        RETURN_METRICS_NAME(statestore_access_from_blkmaker_unit_connect_state);
        RETURN_METRICS_NAME(statestore_access_from_txpool_get_accountstate);
        RETURN_METRICS_NAME(statestore_access_from_txpool_refreshtable);
        RETURN_METRICS_NAME(statestore_access_from_blkmaker_get_target_tablestate);
        
        RETURN_METRICS_NAME(state_load_blk_state_suc);
        RETURN_METRICS_NAME(state_load_blk_state_cache_suc);
        RETURN_METRICS_NAME(state_load_blk_state_fail);
        RETURN_METRICS_NAME(state_load_blk_state_table_suc);
        RETURN_METRICS_NAME(state_load_blk_state_table_fail);
        RETURN_METRICS_NAME(state_load_blk_state_table_cache_suc);
        RETURN_METRICS_NAME(state_load_blk_state_unit_suc);
        RETURN_METRICS_NAME(state_load_blk_state_unit_fail);
        RETURN_METRICS_NAME(state_load_blk_state_unit_cache_suc);

        RETURN_METRICS_NAME(data_table_unpack_units);
        RETURN_METRICS_NAME(data_table_unpack_one_unit);        

        default: assert(false); return nullptr;
    }
}
#undef RETURN_METRICS_NAME

void e_metrics::start() {
    if (running()) {
        return;
    }
    XMETRICS_CONFIG_GET("dump_interval", m_dump_interval);
    XMETRICS_CONFIG_GET("queue_procss_behind_sleep_time", m_queue_procss_behind_sleep_time);
    running(true);
    // auto self = shared_from_this();
    // threading::xbackend_thread::spawn([this, self] { run_process(); });
    m_process_thread = std::thread(&e_metrics::run_process, this);
    m_process_thread.detach();
}

void e_metrics::stop() {
    assert(running());
    running(false);
}

void e_metrics::run_process() {
    for (size_t index = e_simple_begin; index < e_simple_total; index++) {
        s_metrics[index] = std::make_shared<metrics_counter_unit>(matrics_name(static_cast<xmetircs_tag_t>(index)), 0);
    }

    while (running()) {
        process_message_queue();
        std::this_thread::sleep_for(m_queue_procss_behind_sleep_time);
        update_dump();
    }
}

void e_metrics::process_message_queue() {
    auto message_v = m_message_queue.wait_and_pop_all();
    if (message_v.empty())
        return;
    if (message_v.size() > 50000) {
        xkinfo("[xmetrics]alarm metrics_queue_size %zu", message_v.size());
    }
    for (auto & msg_event : message_v) {
        if (msg_event.metrics_name.empty())
            continue;
        metrics::metrics_variant_ptr metrics_ptr;
        auto metrics_real_name = msg_event.metrics_name.substr(0, msg_event.metrics_name.find("&0x", 0));
        if (m_metrics_hub.find(metrics_real_name) == m_metrics_hub.end()) {
            switch (msg_event.major_id) {
            case e_metrics_major_id::count:
                metrics_ptr = m_counter_handler.init_new_metrics(msg_event);
                break;
            case e_metrics_major_id::timer:
                metrics_ptr = m_timer_handler.init_new_metrics(msg_event);
                break;
            case e_metrics_major_id::flow:
                metrics_ptr = m_flow_handler.init_new_metrics(msg_event);
                break;
            default:
                assert(false);
                break;
            }
            m_metrics_hub.insert({metrics_real_name, metrics_ptr});
            assert(m_metrics_hub.count(metrics_real_name));
        } else {
            metrics_ptr = m_metrics_hub[metrics_real_name];
            auto index = static_cast<metrics::e_metrics_major_id>(metrics_ptr.GetType());
            switch (index) {
            case metrics::e_metrics_major_id::count:
                m_counter_handler.process_message_event(metrics_ptr, msg_event);
                break;
            case metrics::e_metrics_major_id::timer:
                m_timer_handler.process_message_event(metrics_ptr, msg_event);
                break;
            case metrics::e_metrics_major_id::flow:
                m_flow_handler.process_message_event(metrics_ptr, msg_event);
                break;
            default:
                break;
            }
        }
    }
}

void e_metrics::update_dump() {
    if (--m_dump_interval)
        return;
    auto const & map_ref = m_metrics_hub;
    for (auto const & pair : map_ref) {
        auto const & metrics_ptr = pair.second;
        auto index = static_cast<metrics::e_metrics_major_id>(pair.second.GetType());
        switch (index) {
        case metrics::e_metrics_major_id::count:
            m_counter_handler.dump_metrics_info(metrics_ptr);
            break;
        case metrics::e_metrics_major_id::timer:
            m_timer_handler.dump_metrics_info(metrics_ptr);
            break;
        case metrics::e_metrics_major_id::flow:
            m_flow_handler.dump_metrics_info(metrics_ptr);
            break;
        default:
            assert(false);
            break;
        }
    }

    // simpe metrics dump
    gauge_dump();
    XMETRICS_CONFIG_GET("dump_interval", m_dump_interval);
}
/*
 * one funtion for one [major_id-minor_id] message_event
 */

void e_metrics::timer_start(std::string metrics_name, time_point time) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::timer, metrics::e_metrics_minor_id::timestart, metrics_name, time));
}
void e_metrics::timer_end(std::string metrics_name, time_point time, metrics_appendant_info info, microseconds timed_out) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::timer, metrics::e_metrics_minor_id::timeend, metrics_name, time, info, timed_out));
}

void e_metrics::counter_increase(std::string metrics_name, int64_t value) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::count, metrics::e_metrics_minor_id::increase, metrics_name, value));
}
void e_metrics::counter_decrease(std::string metrics_name, int64_t value) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::count, metrics::e_metrics_minor_id::decrease, metrics_name, value));
}
void e_metrics::counter_set(std::string metrics_name, int64_t value) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::count, metrics::e_metrics_minor_id::set, metrics_name, value));
}
void e_metrics::flow_count(std::string metrics_name, int64_t value, time_point timestamp) {
    m_message_queue.push(event_message(metrics::e_metrics_major_id::flow, metrics::e_metrics_minor_id::flow_count, metrics_name, value, metrics_appendant_info{timestamp}));
}
void e_metrics::gauge(E_SIMPLE_METRICS_TAG tag, int64_t value) {
    if (tag >= e_simple_total || tag <= e_simple_begin ) {
        return;
    }
    s_counters[tag].value += value;
    s_counters[tag].call_count++;
}

struct xsimple_merics_category
{
    E_SIMPLE_METRICS_TAG category;
    E_SIMPLE_METRICS_TAG start;
    E_SIMPLE_METRICS_TAG end;
};

// categories
xsimple_merics_category g_cates[] = {
    {blockstore_access_from_txpool, blockstore_access_from_txpool_begin, blockstore_access_from_txpool_end},
    {blockstore_access_from_statestore, blockstore_access_from_statestore_begin, blockstore_access_from_statestore_end},
    {blockstore_access_from_sync, blockstore_access_from_sync_begin, blockstore_access_from_sync_end},
    {blockstore_access_from_blk_mk, blockstore_access_from_blk_mk_begin, blockstore_access_from_blk_mk_end},
    {blockstore_access_from_us, blockstore_access_from_us_begin, blockstore_access_from_us_end},
    {blockstore_access_from_bft, blockstore_access_from_bft_begin, blockstore_access_from_bft_end},
    {statestore_access, statestore_access_begin, statestore_access_end},
    {blockstore_access_from_mbus, blockstore_access_from_mbus_begin, blockstore_access_from_mbus_end},
    {blockstore_access_from_rpc, blockstore_access_from_rpc_begin, blockstore_access_from_rpc_end}
};

bool is_category(E_SIMPLE_METRICS_TAG tag) {
    for(size_t index = 0; index < sizeof(g_cates)/sizeof(g_cates[0]); index++) {
        if(tag == g_cates[index].category) {
            return true;
        }
    }
    return false;
}

void e_metrics::gauge_dump() {
    // detail metrics dump
    for(auto index = (int32_t)e_simple_begin + 1; index < (int32_t)e_simple_total; index++) {
        if(is_category((E_SIMPLE_METRICS_TAG)index)) {
            continue;
        }
        auto metrics_ptr = s_metrics[index];
        auto ptr = metrics_ptr.GetRef<metrics_counter_unit_ptr>();
        ptr->inner_val = s_counters[index].value;
        ptr->count = s_counters[index].call_count;
        m_counter_handler.dump_metrics_info(ptr);
    }
    
    // summary of category as defined
    for(size_t index = 0; index < sizeof(g_cates)/sizeof(g_cates[0]); index++) {
        uint64_t cate_val = 0;
        uint64_t cate_count = 0;
        auto cate = g_cates[index];
        for(auto cate_index = (int)cate.start; cate_index <= (int)cate.end; cate_index++) {
            cate_val += s_counters[cate_index].value;
            cate_count += s_counters[cate_index].call_count;
        }
        auto metrics_ptr = s_metrics[cate.category];
        auto ptr = metrics_ptr.GetRef<metrics_counter_unit_ptr>();
        ptr->inner_val = cate_val;
        ptr->count = cate_count;
        m_counter_handler.dump_metrics_info(ptr);
    }
}
NS_END2
