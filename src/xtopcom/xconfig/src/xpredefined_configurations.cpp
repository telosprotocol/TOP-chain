// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#if !defined XCXX14_OR_ABOVE

#    include "xconfig/xpredefined_configurations.h"

NS_BEG2(top, config)

#    define XDEFINE_CONFIGURATION(NAME)                                                                                                                                            \
        constexpr char const * xtop_##NAME##_configuration ::name;                                                                                                                 \
        constexpr xtop_##NAME##_configuration::type xtop_##NAME##_configuration::value

#    define XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(NAME)                                                                                                                             \
        constexpr char const * xtop_##NAME##_onchain_goverance_parameter::name;                                                                                                    \
        constexpr top::xrange_t<xtop_##NAME##_onchain_goverance_parameter::type> xtop_##NAME##_onchain_goverance_parameter::range;                                                 \
        constexpr xonchain_goverance_parameter_classification_t xtop_##NAME##_onchain_goverance_parameter::classification;                                                         \
        constexpr xtop_##NAME##_onchain_goverance_parameter::type xtop_##NAME##_onchain_goverance_parameter::value

XDEFINE_CONFIGURATION(chain_name);
XDEFINE_CONFIGURATION(root_hash);
XDEFINE_CONFIGURATION(global_timer_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(rec_election_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(zec_election_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(zone_election_trigger_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(edge_election_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(archive_election_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(election_rotation_count_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_minimum_rotation_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_group_size);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_rotation_count);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_edge_group_size);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(rec_standby_pool_update_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(zec_standby_pool_update_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_height_step_limitation);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_logic_timeout_limitation);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_stake);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(auditor_nodes_per_segment);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance);
// XDEFINE_CONFIGURATION(receive_tx_cache_time_s);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_tcc_proposal_deposit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(tcc_proposal_expire_time);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(tcc_member);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(free_gas);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(cpu_gas_exchange_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(unlock_gas_staked_delay_time);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token);


XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_name_max_len);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_max_number);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(application_contract_code_max_len);

// whitelist
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist);  // default not open
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(whitelist);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(additional_issue_year_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_ratio_annual_total_reward);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_edge_deposit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_archive_deposit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_deposit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_deposit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(dividend_ratio_change_interval);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_credit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(initial_creditscore);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(reward_update_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(workload_collection_interval);

XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(vote_reward_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(validator_reward_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(edge_reward_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(archive_reward_ratio);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(governance_reward_ratio);

// mainnet node conditions
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_auditors);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_validators);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_edges);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_archives);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_votes);

// xstake contracts
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_stake_votes_num);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_votes_num);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_vote_nodes_num);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tableblock);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tx);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(min_voter_dividend);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(auditor_group_zero_workload);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_zero_workload);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_height_step_limitation);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_logic_timeout_limitation);

// slash related
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(table_statistic_report_schedule_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(punish_collection_interval);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(backward_node_lock_duration_increment);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(max_nodedeposit_lock_duration);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(backward_validator_slash_credit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(backward_auditor_slash_credit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_publishment_threshold_value);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_reward_threshold_value);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_reward_threshold_value);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(award_validator_credit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(award_auditor_credit);
XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER(contract_call_contracts_num);

/* begin of offchain parameters */
XDEFINE_CONFIGURATION(auditor_group_count);
XDEFINE_CONFIGURATION(validator_group_count);
XDEFINE_CONFIGURATION(archive_group_count);
XDEFINE_CONFIGURATION(min_edge_archive_deposit);
XDEFINE_CONFIGURATION(min_account_deposit);
XDEFINE_CONFIGURATION(zone_count);
XDEFINE_CONFIGURATION(cluster_count);
XDEFINE_CONFIGURATION(executor_max_total_sessions_service_counts);
XDEFINE_CONFIGURATION(executor_max_session_service_counts);
XDEFINE_CONFIGURATION(executor_session_time_interval);
XDEFINE_CONFIGURATION(executor_max_sessions);
XDEFINE_CONFIGURATION(recv_tx_cache_window);
XDEFINE_CONFIGURATION(config_property_alias_name_max_len);
XDEFINE_CONFIGURATION(account_send_queue_tx_max_num);
XDEFINE_CONFIGURATION(edge_max_msg_packet_size);
XDEFINE_CONFIGURATION(leader_election_round);
XDEFINE_CONFIGURATION(unitblock_confirm_tx_batch_num);
XDEFINE_CONFIGURATION(unitblock_recv_transfer_tx_batch_num);
XDEFINE_CONFIGURATION(unitblock_send_transfer_tx_batch_num);
XDEFINE_CONFIGURATION(tableblock_batch_tx_max_num);
XDEFINE_CONFIGURATION(tableblock_batch_unitblock_max_num);
XDEFINE_CONFIGURATION(fulltable_interval_block_num);

XDEFINE_CONFIGURATION(local_blacklist);
XDEFINE_CONFIGURATION(local_whitelist);

XDEFINE_CONFIGURATION(slash_fulltable_interval);
XDEFINE_CONFIGURATION(slash_table_split_num);

/* beginning of development parameters */
XDEFINE_CONFIGURATION(http_port);
XDEFINE_CONFIGURATION(grpc_port);
XDEFINE_CONFIGURATION(dht_port);
XDEFINE_CONFIGURATION(msg_port);
XDEFINE_CONFIGURATION(ws_port);
XDEFINE_CONFIGURATION(network_id);
XDEFINE_CONFIGURATION(log_level);
XDEFINE_CONFIGURATION(log_path);
XDEFINE_CONFIGURATION(db_path);
XDEFINE_CONFIGURATION(ip);
/* end of development parameters */

/* end of offchain parameters */
XDEFINE_CONFIGURATION(platform_business_port);
XDEFINE_CONFIGURATION(platform_public_endpoints);
XDEFINE_CONFIGURATION(platform_show_cmd);
XDEFINE_CONFIGURATION(platform_db_path);
XDEFINE_CONFIGURATION(platform_url_endpoints);



#    undef XDEFINE_ONCHAIN_GOVERNANCE_PARAMETER
#    undef XDEFINE_CONFIGURATION

NS_END2

#endif
