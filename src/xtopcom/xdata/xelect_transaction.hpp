// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xserializable_based_on.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_to_string_helper.h"
#include "xdata/xchain_param.h"
#include "xdata/xrootblock.h"

#include <map>
#include <string>
#include <vector>

NS_BEG2(top, data)

class xtcc_transaction_t : public xserializable_based_on<void> {
public:
    xtcc_transaction_t() {
#define XADD_ONCHAIN_GOVERNANCE_PARAMETER(NAME)                                                                                                                                    \
    m_initial_values.insert(                                                                                                                                                       \
        {top::config::x##NAME##_onchain_goverance_parameter_t::name,                                                                                                               \
         top::config::xto_string_helper_t<top::config::x##NAME##_onchain_goverance_parameter_t::type>::to_string(top::config::x##NAME##_onchain_goverance_parameter_t::value)})

#define XADD_ONCHAIN_GOVERNANCE_PARAMETER2(NAME, VALUE) m_initial_values.insert({top::config::x##NAME##_onchain_goverance_parameter_t::name, (VALUE)})

        // election:
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(rec_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(zec_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(zone_election_trigger_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(edge_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(archive_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(election_rotation_count_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_minimum_rotation_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_rotation_count);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_edge_group_size);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size);

        XADD_ONCHAIN_GOVERNANCE_PARAMETER(rec_standby_pool_update_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(zec_standby_pool_update_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_height_step_limitation);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_logic_timeout_limitation);

        // slash related
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(punish_collection_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_publishment_threshold_value);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_credit);                                 // default minimun 0.1
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(backward_validator_slash_credit);            // validator credit score 0.1
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(backward_auditor_slash_credit);              // auditor credit score 0.1
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_reward_threshold_value);          // award node persent
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_reward_threshold_value);  // award node vote
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(award_validator_credit);                     // validator credit score 0.03;
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(award_auditor_credit);                       // auditor credit score 0.03
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(backward_node_lock_duration_increment);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_nodedeposit_lock_duration);

        // stake:
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_stake);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(auditor_nodes_per_segment);

        // consensus
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_balance);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(free_gas);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cpu_gas_exchange_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(unlock_gas_staked_delay_time);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_name_max_len);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_max_number);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(application_contract_code_max_len);

        // tcc
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_tcc_proposal_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(tcc_proposal_expire_time);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER2(tcc_member_number, get_tcc_onchain_committee_list());
        // whitelist
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER2(whitelist, get_genesis_whitelist());
        // XADD_ONCHAIN_GOVERNANCE_PARAMETER(whitelist);

        // mainnet node conditions
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_auditors);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_validators);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_edges);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_archives);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_votes);

        // xstake contracts
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_edge_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_archive_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_deposit);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(dividend_ratio_change_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_stake_votes_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_votes_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(max_vote_nodes_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(additional_issue_year_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_ratio_annual_total_reward);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(vote_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(validator_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(edge_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(archive_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(governance_reward_ratio);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(reward_update_interval);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tx);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tableblock);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(min_voter_dividend);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(auditor_group_zero_workload);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_zero_workload);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_height_step_limitation);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_logic_timeout_limitation);

        XADD_ONCHAIN_GOVERNANCE_PARAMETER(contract_call_contracts_num);
        XADD_ONCHAIN_GOVERNANCE_PARAMETER(workload_collection_interval);

        std::vector<std::string> committee_addrs = xrootblock_t::get_tcc_initial_committee_addr();
        for (auto & v : committee_addrs) {
            m_initial_committee.push_back(v);
            xdbg("xtcc_transaction_t initial committee account %s", v.c_str());
        }

#undef XADD_ONCHAIN_GOVERNANCE_PARAMETER
    }

    int32_t do_write(base::xstream_t & stream) const override {
        const int32_t begin_size = stream.size();
        MAP_SERIALIZE_SIMPLE(stream, m_initial_values);
        VECTOR_SERIALIZE_SIMPLE(stream, m_initial_committee);
        const int32_t end_size = stream.size();
        return (end_size - begin_size);
    }

    int32_t do_read(base::xstream_t & stream) override {
        const int32_t begin_size = stream.size();
        MAP_DESERIALIZE_SIMPLE(stream, m_initial_values);
        VECTOR_DESERIALZE_SIMPLE(stream, m_initial_committee, std::string);
        const int32_t end_size = stream.size();
        return (begin_size - end_size);
    }

public:
    std::string get_genesis_whitelist() const;
    std::string get_tcc_onchain_committee_list() const;
    std::map<std::string, std::string> m_initial_values;
    std::vector<std::string> m_initial_committee;
};
using xtcc_transaction_ptr_t = std::shared_ptr<xtcc_transaction_t>;

NS_END2
