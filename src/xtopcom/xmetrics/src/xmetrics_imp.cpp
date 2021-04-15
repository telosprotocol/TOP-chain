// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#include <atomic>
#include <sstream>
#include <iomanip>
#include <memory>
#include <vector>
#include <unordered_map>
#include <iostream>

#include "xmetrics/xmetrics_imp.h"
#include "xbase/xcontext.h"
#include "xbase/xtimer.h"
#include "xbase/xthread.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xlog.h"
#include "xbase/xrwspinlock.h"

NS_BEG2(top, metrics)

#ifdef ENABLE_METRICS

static const int TAG_VALUE_PAD = (128/sizeof(std::atomic_long)) <= 0 ? 1 : (128/sizeof(std::atomic_long));

std::atomic_long tags_vals[(xmetrics::e_max_tags+1)*TAG_VALUE_PAD] = {};

struct metrics_timer_stat
{
    alignas(128) std::atomic<uint64_t> m_min_diff{};
    alignas(128) std::atomic<uint64_t> m_max_diff{};
    alignas(128) std::atomic<uint64_t> m_total_diff{};
    alignas(128) std::atomic<uint64_t> m_count{};
};

metrics_timer_stat timer_elapse_vals[xmetrics::e_max_tags+1]{};

#define CASE_RETURN(METRICS_NAME_STR)   case xmetrics::METRICS_NAME_STR: return #METRICS_NAME_STR

static char const * xmetrics_map(int const metrics) noexcept {
    auto const ec = static_cast<xmetrics>(metrics);
    switch (ec) {
        CASE_RETURN(e_mbus);
        CASE_RETURN(e_mbus_push);

        CASE_RETURN(e_store_set);
        CASE_RETURN(e_store_get);
        CASE_RETURN(e_store_get_serialization);

        CASE_RETURN(e_store_block_retry);
        CASE_RETURN(e_store_transaction_retry);
        CASE_RETURN(e_store_block_set);
        CASE_RETURN(e_store_block_execution);

        CASE_RETURN(e_blockstore_load_block);
        CASE_RETURN(e_blockstore_store_block);
        CASE_RETURN(e_blockstore_sync_store_block);
        CASE_RETURN(e_blockstore_cache_block_total);

        CASE_RETURN(e_block_empty_table_total);
        CASE_RETURN(e_block_normal_table_total);
        CASE_RETURN(e_block_empty_unit_total);
        CASE_RETURN(e_block_normal_unit_total);

        CASE_RETURN(e_cons_start_leader);
        CASE_RETURN(e_cons_verify_backup);
        CASE_RETURN(e_cons_finish_succ);
        CASE_RETURN(e_cons_finish_fail);
        CASE_RETURN(e_cons_backup_finish_succ);
        CASE_RETURN(e_cons_backup_finish_fail);
        CASE_RETURN(e_cons_elected_leader);
        CASE_RETURN(e_cons_invoke_sync);
        CASE_RETURN(e_cons_succ_time_consuming);
        CASE_RETURN(e_cons_view_change_time_consuming);
        CASE_RETURN(e_cons_commit_delay_too_much);

        CASE_RETURN(e_txpool_push_tx_send_cur);
        CASE_RETURN(e_txpool_push_tx_recv_cur);
        CASE_RETURN(e_txpool_push_tx_confirm_cur);
        CASE_RETURN(e_txpool_push_tx_send_total);
        CASE_RETURN(e_txpool_push_tx_recv_total);
        CASE_RETURN(e_txpool_push_tx_confirm_total);
        CASE_RETURN(e_txpool_pop_tx_send_total);
        CASE_RETURN(e_txpool_pop_tx_recv_total);
        CASE_RETURN(e_txpool_pop_tx_confirm_total);
        CASE_RETURN(e_txpool_succ_send_residence_time);
        CASE_RETURN(e_txpool_succ_recv_residence_time);
        CASE_RETURN(e_txpool_succ_confirm_residence_time);
        CASE_RETURN(e_txpool_push_tx_fail);
        CASE_RETURN(e_txpool_receipt_first_send);
        CASE_RETURN(e_txpool_receipt_retry_send);
        CASE_RETURN(e_txpool_receipt_recv_total);
        CASE_RETURN(e_txpool_receipt_deweight_cache);
        CASE_RETURN(e_txpool_unconfirm_cache);
        CASE_RETURN(e_txpool_receipt_retry_cache);

        CASE_RETURN(e_http_request);
        CASE_RETURN(e_http_response);
        CASE_RETURN(e_edge_request);

        CASE_RETURN(e_adv_http);
        CASE_RETURN(e_con_tx);
        CASE_RETURN(e_con_query);
        CASE_RETURN(e_ws_connect);
        CASE_RETURN(e_ws_close);
        CASE_RETURN(e_ws_request);
        CASE_RETURN(e_ws_response);

        CASE_RETURN(e_vhost_on_data_ready_called);
        CASE_RETURN(e_vhost_handle_data_ready_called);
        CASE_RETURN(e_vhost_handle_data_callback);
        CASE_RETURN(e_vhost_handle_data_filter);
        CASE_RETURN(e_vhost_received_valid);
        CASE_RETURN(e_vhost_received_invalid);
        CASE_RETURN(e_vhost_discard_addr_not_match);
        CASE_RETURN(e_vhost_discard_validation_failure);
        CASE_RETURN(e_vhost_discard_other);

        CASE_RETURN(e_vnetwork_driver_received);
        CASE_RETURN(e_vnetwork_lru_cache);

        CASE_RETURN(e_data_transaction_new);
        CASE_RETURN(e_data_transaction_delete);
        CASE_RETURN(e_data_block_new);
        CASE_RETURN(e_data_block_delete);
        CASE_RETURN(e_data_receipt_new);
        CASE_RETURN(e_data_receipt_delete);
        CASE_RETURN(e_data_lightunit_txinfo_new);
        CASE_RETURN(e_data_lightunit_txinfo_delete);
        CASE_RETURN(e_data_blockbody_new);
        CASE_RETURN(e_data_blockbody_delete);

        CASE_RETURN(e_transport_packet_send);
        CASE_RETURN(e_transport_packet_recv);
        CASE_RETURN(e_transport_bandwidth_send);
        CASE_RETURN(e_transport_bandwidth_recv);
        CASE_RETURN(e_transport_broadcast_packet_send);
        CASE_RETURN(e_transport_broadcast_packet_recv);
        CASE_RETURN(e_transport_broadcast_bandwidth_send);
        CASE_RETURN(e_transport_broadcast_bandwidth_recv);
        CASE_RETURN(e_transport_heartbeat_packet_send);
        CASE_RETURN(e_transport_heartbeat_packet_recv);
        CASE_RETURN(e_transport_heartbeat_bandwidth_send);
        CASE_RETURN(e_transport_heartbeat_bandwidth_recv);
        CASE_RETURN(e_transport_gossipsync_packet_send);
        CASE_RETURN(e_transport_gossipsync_packet_recv);
        CASE_RETURN(e_transport_gossipsync_bandwidth_send);
        CASE_RETURN(e_transport_gossipsync_bandwidth_recv);
        CASE_RETURN(e_transport_p2pchain_packet_send);
        CASE_RETURN(e_transport_p2pchain_packet_recv);
        CASE_RETURN(e_transport_p2pchain_bandwidth_send);
        CASE_RETURN(e_transport_p2pchain_bandwidth_recv);
        CASE_RETURN(e_transport_gossipchain_packet_send);
        CASE_RETURN(e_transport_gossipchain_packet_recv);
        CASE_RETURN(e_transport_gossipchain_bandwidth_send);
        CASE_RETURN(e_transport_gossipchain_bandwidth_recv);
        CASE_RETURN(e_transport_platsys_packet_send);
        CASE_RETURN(e_transport_platsys_packet_recv);
        CASE_RETURN(e_transport_platsys_bandwidth_send);
        CASE_RETURN(e_transport_platsys_bandwidth_recv);
        CASE_RETURN(e_transport_p2pchain_afterfilter_packet_recv);
        CASE_RETURN(e_transport_p2pchain_afterfilter_bandwidth_recv);
        CASE_RETURN(e_transport_gossipchain_afterfilter_packet_recv);
        CASE_RETURN(e_transport_gossipchain_afterfilter_bandwidth_recv);
        CASE_RETURN(e_transport_gossipchain_root_afterfilter_packet_recv);
        CASE_RETURN(e_transport_gossipchain_root_afterfilter_bandwidth_recv);
        CASE_RETURN(e_transport_p2pchain_afterfilter_real_packet_recv);
        CASE_RETURN(e_transport_p2pchain_afterfilter_real_bandwidth_recv);
        CASE_RETURN(e_transport_gossipchain_afterfilter_real_packet_recv);
        CASE_RETURN(e_transport_gossipchain_afterfilter_real_bandwidth_recv);

        CASE_RETURN(e_net_iothread_dispatch_transactions_service);
        CASE_RETURN(e_net_iothread_dispatch_shard_rpc_handler);
        CASE_RETURN(e_net_iothread_dispatch_rpc_edge_vhost);
        CASE_RETURN(e_net_iothread_dispatch_cluster_rpc_handler);
        CASE_RETURN(e_net_iothread_dispatch_consensus_pbft);
        CASE_RETURN(e_net_iothread_dispatch_archive_service);
        CASE_RETURN(e_net_iothread_dispatch_elect_client);
        CASE_RETURN(e_net_iothread_dispatch_zec_election);
        CASE_RETURN(e_net_iothread_dispatch_beacon_xelect);
        CASE_RETURN(e_net_iothread_dispatch_beacon_bookload);

        CASE_RETURN(e_sync_event_total);
        CASE_RETURN(e_sync_event_behind_origin);
        CASE_RETURN(e_sync_event_behind_known);
        CASE_RETURN(e_sync_event_lack);
        CASE_RETURN(e_sync_event_store);
        CASE_RETURN(e_sync_event_account);
        CASE_RETURN(e_sync_event_add_role);
        CASE_RETURN(e_sync_event_remove_role);
        CASE_RETURN(e_sync_event_consensus);
        CASE_RETURN(e_sync_event_chain_timer);
        CASE_RETURN(e_sync_event_downloader);

        CASE_RETURN(e_sync_module_eventdispatcher_event_count);
        CASE_RETURN(e_sync_module_downloader_request);
        CASE_RETURN(e_sync_module_downloader_response);
        CASE_RETURN(e_sync_module_downloader_chain_count);
        CASE_RETURN(e_sync_module_downloader_block_behind);
        CASE_RETURN(e_sync_module_downloader_behind_from_consensus);
        CASE_RETURN(e_sync_module_downloader_behind_from_gossip);
        CASE_RETURN(e_sync_module_downloader_behind_from_newblock);
        CASE_RETURN(e_sync_module_downloader_behind_from_newblockhash);
        CASE_RETURN(e_sync_module_downloader_event_count);
        CASE_RETURN(e_sync_module_blockfetcher_request);
        CASE_RETURN(e_sync_module_blockfetcher_response);
        CASE_RETURN(e_sync_module_blockfetcher_event_count);
        CASE_RETURN(e_sync_module_aggregate_pending_block);
        CASE_RETURN(e_sync_module_gossip_send);
        CASE_RETURN(e_sync_module_gossip_origin_behind_source_cold);
        CASE_RETURN(e_sync_module_gossip_origin_behind_source_consensus);
        CASE_RETURN(e_sync_module_gossip_origin_behind_source_nodehouse);
        CASE_RETURN(e_sync_module_handler_blocks);
        CASE_RETURN(e_sync_module_entire_blocks);

        CASE_RETURN(e_sync_pkgs_in);
        CASE_RETURN(e_sync_pkgs_out);
        CASE_RETURN(e_sync_pkgs_getblocks_send);
        CASE_RETURN(e_sync_pkgs_getblocks_recv);
        CASE_RETURN(e_sync_pkgs_newblock_send);
        CASE_RETURN(e_sync_pkgs_newblock_recv);
        CASE_RETURN(e_sync_pkgs_newblockhash_send);
        CASE_RETURN(e_sync_pkgs_newblockhash_recv);
        CASE_RETURN(e_sync_pkgs_gossip_send);
        CASE_RETURN(e_sync_pkgs_gossip_recv);
        CASE_RETURN(e_sync_pkgs_blocks_send);
        CASE_RETURN(e_sync_pkgs_blocks_recv);

        CASE_RETURN(e_sync_bytes_in);
        CASE_RETURN(e_sync_bytes_out);
        CASE_RETURN(e_sync_bytes_getblocks_send);
        CASE_RETURN(e_sync_bytes_getblocks_recv);
        CASE_RETURN(e_sync_bytes_newblock_send);
        CASE_RETURN(e_sync_bytes_newblock_recv);
        CASE_RETURN(e_sync_bytes_newblockhash_send);
        CASE_RETURN(e_sync_bytes_newblockhash_recv);
        CASE_RETURN(e_sync_bytes_gossip_send);
        CASE_RETURN(e_sync_bytes_gossip_recv);
        CASE_RETURN(e_sync_bytes_blocks_send);
        CASE_RETURN(e_sync_bytes_blocks_recv);

        CASE_RETURN(e_sync_cost_event_queue);
        CASE_RETURN(e_sync_cost_event_process);
        CASE_RETURN(e_sync_cost_role_add_event);
        CASE_RETURN(e_sync_cost_role_remove_event);
        CASE_RETURN(e_sync_cost_chain_add_role_event);
        CASE_RETURN(e_sync_cost_chain_remove_role_event);
        CASE_RETURN(e_sync_cost_find_block_event);
        CASE_RETURN(e_sync_cost_response_event);
        CASE_RETURN(e_sync_cost_lack_event);
        CASE_RETURN(e_sync_cost_behind_event);
        CASE_RETURN(e_sync_cost_gossip_timer_event);
        CASE_RETURN(e_sync_cost_gossip_chain_timer_event);
        CASE_RETURN(e_sync_cost_gossip_behind_event);
        CASE_RETURN(e_sync_cost_gossip_add_role_event);
        CASE_RETURN(e_sync_cost_peer_response);
        CASE_RETURN(e_sync_cost_blockfetcher_timer);
        CASE_RETURN(e_sync_cost_blockfetcher_self);
        CASE_RETURN(e_sync_cost_blockfetcher_response);
        CASE_RETURN(e_sync_cost_blockfetcher_connect);

        CASE_RETURN(e_sync_total_discard);

        CASE_RETURN(e_election_rec_elect_archive_on_timer_all_time);
        CASE_RETURN(e_election_rec_elect_archive_get_property_time);
        CASE_RETURN(e_election_rec_elect_archive_set_property_time);
        CASE_RETURN(e_election_rec_elect_archive_get_property_size);
        CASE_RETURN(e_election_rec_elect_archive_set_property_size);
        CASE_RETURN(e_election_rec_elect_edge_on_timer_all_time);
        CASE_RETURN(e_election_rec_elect_edge_get_property_time);
        CASE_RETURN(e_election_rec_elect_edge_set_property_time);
        CASE_RETURN(e_election_rec_elect_edge_get_property_size);
        CASE_RETURN(e_election_rec_elect_edge_set_property_size);
        CASE_RETURN(e_election_rec_elect_rec_on_timer_all_time);
        CASE_RETURN(e_election_rec_elect_rec_get_property_time);
        CASE_RETURN(e_election_rec_elect_rec_set_property_time);
        CASE_RETURN(e_election_rec_elect_rec_get_property_size);
        CASE_RETURN(e_election_rec_elect_rec_set_property_size);
        CASE_RETURN(e_election_rec_elect_zec_on_timer_all_time);
        CASE_RETURN(e_election_rec_elect_zec_get_property_time);
        CASE_RETURN(e_election_rec_elect_zec_set_property_time);
        CASE_RETURN(e_election_rec_elect_zec_get_property_size);
        CASE_RETURN(e_election_rec_elect_zec_set_property_size);
        CASE_RETURN(e_election_zec_elect_consensus_on_timer_all_time);
        CASE_RETURN(e_election_zec_elect_consensus_get_property_time);
        CASE_RETURN(e_election_zec_elect_consensus_set_property_time);
        CASE_RETURN(e_election_zec_elect_consensus_get_property_size);
        CASE_RETURN(e_election_zec_elect_consensus_set_property_size);
        CASE_RETURN(e_election_zec_group_association_get_property_time);
        CASE_RETURN(e_election_zec_group_association_set_property_time);
        CASE_RETURN(e_election_zec_group_association_get_property_size);
        CASE_RETURN(e_election_zec_group_association_set_property_size);

        CASE_RETURN(e_election_vnode_group_count);
        CASE_RETURN(e_election_vnode_count);

        CASE_RETURN(e_rec_standby_pool_add_node_all_time);
        CASE_RETURN(e_rec_standby_pool_on_timer_all_time);
        CASE_RETURN(e_rec_standby_pool_get_property_time);
        CASE_RETURN(e_rec_standby_pool_set_property_time);
        CASE_RETURN(e_rec_standby_pool_get_property_size);
        CASE_RETURN(e_rec_standby_pool_set_property_size);
        CASE_RETURN(e_zec_standby_pool_on_timer_all_time);
        CASE_RETURN(e_zec_standby_pool_get_property_time);
        CASE_RETURN(e_zec_standby_pool_set_property_time);
        CASE_RETURN(e_zec_standby_pool_get_property_size);
        CASE_RETURN(e_zec_standby_pool_set_property_size);

        CASE_RETURN(e_reg_node_register_all_time);
        CASE_RETURN(e_reg_node_deregister_all_time);
        CASE_RETURN(e_reg_node_update_all_time);
        CASE_RETURN(e_reg_set_dividend_rate_all_time);
        CASE_RETURN(e_reg_set_nickname_all_time);
        CASE_RETURN(e_reg_update_batch_stake_all_time);
        CASE_RETURN(e_reg_redeem_all_time);
        CASE_RETURN(e_reg_update_node_type_all_time);
        CASE_RETURN(e_reg_stake_deposit_all_time);
        CASE_RETURN(e_reg_unstake_deposit_all_time);
        CASE_RETURN(e_reg_update_sign_key_all_time);
        CASE_RETURN(e_reg_slash_unqualified_node_all_time);
        CASE_RETURN(e_zec_workload_on_receive_workload2_all_time);
        CASE_RETURN(e_zec_workload_clear_workload_all_time);
        CASE_RETURN(e_zec_vote_on_receive_shard_votes_all_time);
        CASE_RETURN(e_zec_reward_calc_nodes_rewards_all_time);
        CASE_RETURN(e_zec_reward_dispatch_all_reward_all_time);
        CASE_RETURN(e_zec_reward_execute_task_all_time);
        CASE_RETURN(e_zec_seo_request_issuance_all_time);
        CASE_RETURN(e_zec_slash_summarize_slash_info_all_time);
        CASE_RETURN(e_zec_slash_do_unqualified_node_slash_all_time);
        CASE_RETURN(e_reg_XPORPERTY_CONTRACT_REG_KEY_get_property_time);
        CASE_RETURN(e_reg_XPORPERTY_CONTRACT_REG_KEY_set_property_time);
        CASE_RETURN(e_reg_XPORPERTY_CONTRACT_TICKETS_KEY_get_property_time);
        CASE_RETURN(e_reg_XPORPERTY_CONTRACT_TICKETS_KEY_set_property_time);
        CASE_RETURN(e_reg_XPORPERTY_CONTRACT_REFUND_KEY_get_property_time);
        CASE_RETURN(e_reg_XPORPERTY_CONTRACT_REFUND_KEY_set_property_time);
        CASE_RETURN(e_reg_XPORPERTY_CONTRACT_GENESIS_STAGE_KEY_get_property_time);
        CASE_RETURN(e_reg_XPORPERTY_CONTRACT_GENESIS_STAGE_KEY_set_property_time);
        CASE_RETURN(e_reg_XPROPERTY_CONTRACT_SLASH_INFO_KEY_get_property_time);
        CASE_RETURN(e_reg_XPROPERTY_CONTRACT_SLASH_INFO_KEY_set_property_time);
        CASE_RETURN(e_zec_workload_XPORPERTY_CONTRACT_WORKLOAD_KEY_get_property_time);
        CASE_RETURN(e_zec_workload_XPORPERTY_CONTRACT_WORKLOAD_KEY_set_property_time);
        CASE_RETURN(e_zec_workload_XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY_get_property_time);
        CASE_RETURN(e_zec_workload_XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY_set_property_time);
        CASE_RETURN(e_zec_workload_STRING_CREATE_XPORPERTY_CONTRACT_TGAS_KEY_get_property_time);
        CASE_RETURN(e_zec_workload_STRING_CREATE_XPORPERTY_CONTRACT_TGAS_KEY_set_property_time);
        CASE_RETURN(e_zec_vote_XPORPERTY_CONTRACT_TICKETS_KEY_get_property_time);
        CASE_RETURN(e_zec_vote_XPORPERTY_CONTRACT_TICKETS_KEY_set_property_time);
        CASE_RETURN(e_zec_reward_XPORPERTY_CONTRACT_TASK_KEY_get_property_time);
        CASE_RETURN(e_zec_reward_XPORPERTY_CONTRACT_TASK_KEY_set_property_time);
        CASE_RETURN(e_zec_reward_XPORPERTY_CONTRACT_TIME_KEY_get_property_time);
        CASE_RETURN(e_zec_reward_XPORPERTY_CONTRACT_TIME_KEY_set_property_time);
        CASE_RETURN(e_zec_reward_XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_get_property_time);
        CASE_RETURN(e_zec_reward_XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_set_property_time);
        CASE_RETURN(e_zec_slash_XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY_get_property_time);
        CASE_RETURN(e_zec_slash_XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY_set_property_time);
        CASE_RETURN(e_zec_slash_XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY_get_property_time);
        CASE_RETURN(e_zec_slash_XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY_set_property_time);

        CASE_RETURN(e_table_workload_on_timer);
        CASE_RETURN(e_table_workload_get_property_tableblk_height_time);
        CASE_RETURN(e_table_workload_set_property_tableblk_height_time);
        CASE_RETURN(e_table_workload_get_property_logic_time_time);
        CASE_RETURN(e_table_workload_set_property_logic_time_time);
        CASE_RETURN(e_table_workload_get_property_max_logic_time_time);
        CASE_RETURN(e_table_workload_set_property_max_logic_time_time);
        CASE_RETURN(e_table_slash_on_timer);
        CASE_RETURN(e_table_slash_get_property_tableblk_height_time);
        CASE_RETURN(e_table_slash_set_property_tableblk_height_time);
        CASE_RETURN(e_tcc_proposal_add_proposal_time_cost);
        CASE_RETURN(e_tcc_proposal_withdraw_proposal_time_cost);
        CASE_RETURN(e_tcc_proposal_cosign_proposal_time_cost);
        CASE_RETURN(e_tcc_proposal_vote_proposal_time_cost);
        CASE_RETURN(e_tcc_proposal_list_property_get_time_cost);

        CASE_RETURN(e_table_reward_claiming_recv_node_reward_time_cost);
        CASE_RETURN(e_table_reward_claiming_recv_voter_dividend_time_cost);
        CASE_RETURN(e_table_reward_claiming_claim_node_reward_time_cost);
        CASE_RETURN(e_table_reward_claiming_claim_voter_dividend_time_cost);
        CASE_RETURN(e_table_reward_claiming_get_property_reward_key_base_time_cost);
        CASE_RETURN(e_table_reward_claiming_set_property_reward_key_base_time_cost);
        CASE_RETURN(e_table_reward_claiming_get_property_node_reward_time_cost);
        CASE_RETURN(e_table_reward_claiming_set_property_node_reward_time_cost);
        CASE_RETURN(e_table_reward_claiming_get_property_voter_key_base_time_cost);
        CASE_RETURN(e_table_reward_claiming_get_property_pollable_time_cost);

        CASE_RETURN(e_table_vote_vote_node_time_cost);
        CASE_RETURN(e_table_vote_unvote_node_time_cost);
        CASE_RETURN(e_table_vote_set_property_voter_key_base_time_cost);
        CASE_RETURN(e_table_vote_get_property_voter_key_base_time_cost);
        CASE_RETURN(e_table_vote_set_property_pollable_time_cost);
        CASE_RETURN(e_table_vote_get_property_pollable_time_cost);

        CASE_RETURN(e_contract_manager_counter);
        CASE_RETURN(e_contract_role_context_counter);

    default:
        assert(false);
        return u8"unknown metrics enum";
    }
};


class xtop_metrics {
public:
    static std::string metrics_name(int metrics) { return xmetrics_map(metrics); }
};
using xmetrics_t = xtop_metrics;

const char* xmetrics_to_string(int metric) {
    return xmetrics_map(metric);
}

void xmetrics_default_appender::dump(const std::vector<std::string>& tag_values) {
    for (auto& tag_value : tag_values) {
        xkinfo("[xmetrics] %s", tag_value.c_str());
    }
}

xmetrics_default_appender default_appender;
simple_counter counter;

struct category_metric;
using category_metric_ptr_t = std::shared_ptr<category_metric>;

base::xrwspinlock g_category_lock;

// TODO free the memory occupied by g_category_values when xchain finished
std::unordered_map<std::string, category_metric_ptr_t> g_category_values{};

struct category_metric {
    static void incr_counter(const std::string& category, const std::string& key, long val) {
        base::xrwspinlock::upgraded_holder ug(&g_category_lock);
        auto it = g_category_values.find(category);
        if (it != g_category_values.end()) {
            base::xrwspinlock::read_holder rg(std::move(ug));
            it->second->add_value(key, val);
        } else {
            category_metric_ptr_t cm{std::make_shared<struct category_metric>(category)};
            {
                base::xrwspinlock::write_holder wg(std::move(ug));
                g_category_values.insert(std::make_pair(category, cm));
            }
            cm->add_value(key, val);
        }
    }

    static void set_counter(const std::string& category, const std::string& key, long val) {
        base::xrwspinlock::upgraded_holder ug(&g_category_lock);
        auto it = g_category_values.find(category);
        if (it != g_category_values.end()) {
            base::xrwspinlock::read_holder rg(std::move(ug));
            it->second->set_value(key, val);
        } else {
            category_metric_ptr_t cm{std::make_shared<struct category_metric>(category)};
            {
                base::xrwspinlock::write_holder wg(std::move(ug));
                g_category_values.insert(std::make_pair(category, cm));
            }
            cm->set_value(key, val);
        }
    }

    static std::vector<std::vector<std::string> > category_values() {
        std::vector<std::vector<std::string> > dump_values;
        base::xrwspinlock::read_holder rg(&g_category_lock);
        for (auto& entry : g_category_values) {
            dump_values.push_back(entry.second->values());
        }
        return dump_values;
    }

    category_metric(const std::string& category) : m_category(category) {
    }

    void add_value(const std::string& key, long value) {
        base::xrwspinlock::upgraded_holder ug(&m_lock);
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            base::xrwspinlock::read_holder rg(std::move(ug));
            m_values[key] += value;
        } else {
            base::xrwspinlock::write_holder wg(std::move(ug));
            m_values[key] = value;
        }
    }

    void set_value(const std::string& key, long value) {
        base::xrwspinlock::upgraded_holder ug(&m_lock);
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            base::xrwspinlock::read_holder rg(std::move(ug));
            m_values[key] = value;
        } else {
            base::xrwspinlock::write_holder wg(std::move(ug));
            m_values[key] = value;
        }
    }

    std::vector<std::string> values() {

        std::vector<std::string> dump_values;
        base::xrwspinlock::read_holder rg(&m_lock);
        for (auto& entry : m_values) {

            std::stringstream ss;
            ss.setf(std::ios::left, std::ios::adjustfield);
            ss.fill(' ');
            ss << std::setw(20) << m_category
                << " " << std::setw(20) << entry.first
                << ": " << std::setw(15) << entry.second;
            dump_values.push_back(ss.str());
        }
        return dump_values;
    }

    base::xrwspinlock m_lock;
    std::string m_category;
    std::unordered_map<std::string, std::atomic_long> m_values;
};

void incr_counter(xmetrics metrics, int32_t val) {
    tags_vals[(uint32_t)metrics*TAG_VALUE_PAD] += val;
}

void set_counter(xmetrics metrics, int32_t val) {
    tags_vals[(uint32_t)metrics*TAG_VALUE_PAD] = val;
}

void incr_category_counter(const std::string& category, const std::string& key, long val) {
    category_metric::incr_counter(category, key, val);
}

void set_category_counter(const std::string& category, const std::string& key, long val) {
    category_metric::set_counter(category, key, val);
}

void set_timer_elapse(xmetrics metrics, uint64_t val) {
    ++timer_elapse_vals[metrics].m_count;
    timer_elapse_vals[metrics].m_total_diff.fetch_add(val);
    if (timer_elapse_vals[metrics].m_min_diff > val
        || timer_elapse_vals[metrics].m_min_diff == 0) {
        timer_elapse_vals[metrics].m_min_diff = val;
    }
    if (timer_elapse_vals[metrics].m_max_diff < val) {
        timer_elapse_vals[metrics].m_max_diff = val;
    }
}

void init(xmetrics_appender * appender) {
    counter.init(appender);
}

void init_with_interval(uint64_t interval) {
    counter.set_interval(interval);
    counter.init(nullptr);
}

xcounter_timer::xcounter_timer(base::xcontext_t & _context, //NOLINT
                            int32_t timer_thread_id,
                            xmetrics_appender * appender) //NOLINT
   : base::xxtimer_t(_context, timer_thread_id), m_ext_appender(appender) {
        m_default_appender = &default_appender;
   }

bool xcounter_timer::on_timer_fire(const int32_t thread_id,
                                const int64_t timer_id,
                                const int64_t current_time_ms,
                                const int32_t start_timeout_ms,
                                int32_t & in_out_cur_interval_ms) {
    counter.update_metrics();
    if (m_ext_appender != nullptr) {
        m_ext_appender->dump(counter.values());
        for (auto& entry : category_metric::category_values()) {
            m_default_appender->dump(entry);
        }
    }
    m_default_appender->dump(counter.values());
    for (auto& entry : category_metric::category_values()) {
        m_default_appender->dump(entry);
    }
    return true;
}

void simple_counter::incr(xmetrics metrics, int32_t val) {
    tags_vals[(uint32_t)metrics*TAG_VALUE_PAD] += val;
}

void simple_counter::init(xmetrics_appender * appender) {
    m_timer_thread = base::xiothread_t::create_thread(
                    base::xcontext_t::instance(), 0, -1);
    m_timer = make_object_ptr<xcounter_timer>(
                        top::base::xcontext_t::instance(),
                        m_timer_thread->get_thread_id(), appender);
    m_timer->start(0, m_interval);
}

void simple_counter::update_metrics() {
    // skip e_min_tags & e_max_tags;
    for (auto i = 0; i < (int32_t)xmetrics::e_max_tags; ++i) {
        m_delta_metrics[i] = tags_vals[i*TAG_VALUE_PAD] - m_current_metrics[i];
        m_current_metrics[i] = tags_vals[i*TAG_VALUE_PAD];
    }
}

std::vector<std::string> simple_counter::values() {

    std::vector<std::string> dump_values;

    for (auto i = 0; i < (int32_t)xmetrics::e_max_tags; ++i) {
        if (m_current_metrics[i] == 0 && m_delta_metrics[i] == 0) {
            continue;
        }

        std::stringstream ss;
        ss.setf(std::ios::left, std::ios::adjustfield);
        ss.fill(' ');
        ss << std::setw(30) << xmetrics_t::metrics_name(i)
            << ", current: " << std::setw(15) << m_current_metrics[i]
            << ", delta: " << std::setw(10) << m_delta_metrics[i]
            << ", tps: " << std::setw(10) << std::fixed << std::setprecision(2)
            << (m_delta_metrics[i] * 1.0 / (m_interval / 1000));
        dump_values.push_back(ss.str());
    }

    for (auto i = 0; i < (int32_t)xmetrics::e_max_tags; ++i) {
        if (timer_elapse_vals[i].m_count > 0) {
            std::stringstream ss_time;
            ss_time.setf(std::ios::left, std::ios::adjustfield);
            ss_time.fill(' ');
            ss_time << std::setw(30) << xmetrics_t::metrics_name(i) << " timer";

            uint64_t count = timer_elapse_vals[i].m_count;
            uint64_t min = timer_elapse_vals[i].m_min_diff;
            uint64_t max = timer_elapse_vals[i].m_max_diff;
            uint64_t total = timer_elapse_vals[i].m_total_diff;
            uint64_t avg = total / count;
            ss_time << ", count: " << count << std::setw(20);
            ss_time << ", min: " << min << "us" << std::setw(15);
            ss_time << ", max: " << max << "us" << std::setw(15);
            ss_time << ", avg: " << avg << "us" << std::setw(15);

            dump_values.push_back(ss_time.str());
        }
    }

    return dump_values;
}

#endif

NS_END2

#endif
