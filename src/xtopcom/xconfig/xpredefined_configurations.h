// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

// TODO(jimmy) #include "xbase/xvledger.h"
#include "xbasic/xrange.hpp"
#include "xcommon/xlogic_time.h"
#include "xconfig/xchain_names.h"

#include <chrono>
#include <cstdint>
#include <limits>

constexpr uint64_t TOP_UNIT = 1e6; // 1TOP = 1e6 uTOP
constexpr uint64_t TOTAL_ISSUANCE = 200 * 1e8 * TOP_UNIT;
#define ASSET_TOP(num) ((uint64_t)((num)*TOP_UNIT))
#define ASSET_uTOP(num) ((uint64_t)(num))
#define TOP_UNIT_LENGTH 6
#define TOP_MAX_LENGTH 12 // enough for 20,000,000,000 tokens

#ifdef _WIN32
#define OLD_DB_PATH "\\db_v2"
#define DB_PATH "\\db_v3"
#else
#define OLD_DB_PATH "/db_v2"
#define DB_PATH "/db_v3"
#endif

NS_BEG2(top, config)

// declare configuration below and define it in the corresponding cpp file

enum class xtop_enum_onchain_goverance_parameter_classification : int32_t
{
    invalid,
    normal,
    important,
    critical
};
using xonchain_goverance_parameter_classification_t = xtop_enum_onchain_goverance_parameter_classification;

#define XDECLARE_CONFIGURATION(NAME, TYPE, DEFAULT_VALUE) \
    struct xtop_##NAME##_configuration                    \
    {                                                     \
        static constexpr char const *name{#NAME};         \
        using type = TYPE;                                \
        static constexpr type value{DEFAULT_VALUE};       \
    };                                                    \
    using x##NAME##_configuration_t = xtop_##NAME##_configuration

#define XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(NAME, TYPE, CLS, DEFAULT_VALUE, RANGE_BEGIN, RANGE_END)                                      \
    struct xtop_##NAME##_onchain_goverance_parameter                                                                                       \
    {                                                                                                                                      \
        static constexpr char const *name{#NAME};                                                                                          \
        using type = TYPE;                                                                                                                 \
        static constexpr top::xrange_t<type> range{RANGE_BEGIN, RANGE_END};                                                                \
        static constexpr xonchain_goverance_parameter_classification_t classification{xonchain_goverance_parameter_classification_t::CLS}; \
        static constexpr type value{DEFAULT_VALUE};                                                                                        \
    };                                                                                                                                     \
    using x##NAME##_onchain_goverance_parameter_t = xtop_##NAME##_onchain_goverance_parameter

template <typename OnChainGovernanceParameterT>
constexpr top::xrange_t<typename OnChainGovernanceParameterT::type> const &get_onchain_governance_parameter_range() noexcept
{
    return OnChainGovernanceParameterT::range;
}

#define XGET_ONCHAIN_GOVERNANCE_PARAMETER_RANGE(NAME) get_onchain_governance_parameter_range<x##NAME##_onchain_goverance_parameter_t>()

using xinterval_t = std::uint32_t;
using xgroup_size_t = std::uint16_t;

#define XGLOBAL_TIMER_INTERVAL_IN_SECONDS std::chrono::duration_cast<std::chrono::seconds>(top::config::xglobal_timer_interval_configuration_t::value).count()
XDECLARE_CONFIGURATION(global_timer_interval, std::chrono::milliseconds, 10000); // global timer interval 10

// election onchain:
#if defined(XBUILD_DEV) || defined(XBUILD_CI) // for local test, election interval should be small for enabling REC/ZEC/EDGE/ARCHIVE election in testing logic
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(rec_election_interval,
                                      xinterval_t,
                                      normal,
                                      191, // time interval in logic clock unit
                                      1,
                                      std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(zec_election_interval,
                                      xinterval_t,
                                      normal,
                                      111, // time interval in logic clock unit
                                      1,
                                      std::numeric_limits<xinterval_t>::max());

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(edge_election_interval, xinterval_t, normal, 13, 1, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(archive_election_interval, xinterval_t, normal, 17, 1, std::numeric_limits<xinterval_t>::max());
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(rec_election_interval,
                                      xinterval_t,
                                      normal,
                                      259201, // time interval in logic clock unit, 30 days + 1 more logic time.
                                      1,
                                      std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(zec_election_interval,
                                      xinterval_t,
                                      normal,
                                      259183, // time interval in logic clock unit, 30 days.
                                      1,
                                      std::numeric_limits<xinterval_t>::max());

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(edge_election_interval, xinterval_t, normal, 360, 1, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(archive_election_interval, xinterval_t, normal, 360, 1, std::numeric_limits<xinterval_t>::max());
#endif

#if defined(XBUILD_DEV) || defined(XBUILD_CI)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(zone_election_trigger_interval, xinterval_t, normal, 5, 1, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval, xinterval_t, normal, 41, 1, std::numeric_limits<xinterval_t>::max());
#elif defined(XBUILD_GALILEO)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(zone_election_trigger_interval, xinterval_t, normal, 181, 1, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval, xinterval_t, normal, 360, 1, std::numeric_limits<xinterval_t>::max());
#elif defined(XBUILD_BOUNTY)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(zone_election_trigger_interval, xinterval_t, normal, 181, 1, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval, xinterval_t, normal, 360, 1, std::numeric_limits<xinterval_t>::max());
#else
// for mainnet, the followed two intervals will be updated some time by TCC proposal. We want the genesis stage run a little faster
// to fill the group with less time.
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(zone_election_trigger_interval, xinterval_t, normal, 31, 1, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval, xinterval_t, normal, 60, 1, std::numeric_limits<xinterval_t>::max());
#endif

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(election_rotation_count_ratio, std::uint16_t, normal, 8, 1, 33);             // means elects no more than 8% (about 1/12) of the current_group_size
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_minimum_rotation_ratio, std::uint16_t, normal, 66, 1, 100); // means if the consensus_group_size < max_group_size &&
                                                                                                                   // effective_standby_size < 66% of current_group_size. Then
                                                                                                                   // needs to elects out

#if defined(XBUILD_CI)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size, xgroup_size_t, normal, 3, 3, 32);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size, xgroup_size_t, normal, 4, 4, 256);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_group_size, xgroup_size_t, normal, 3, 3, 32);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size, xgroup_size_t, normal, 4, 4, 512);
#elif defined(XBUILD_GALILEO) || defined(XBUILD_DEV)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size, xgroup_size_t, normal, 6, 6, 16);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size, xgroup_size_t, normal, 16, 16, 256);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_group_size, xgroup_size_t, normal, 6, 6, 16);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size, xgroup_size_t, normal, 16, 16, 512);
#elif defined(XBUILD_BOUNTY)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size, xgroup_size_t, normal, 4, 4, 16);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size, xgroup_size_t, normal, 7, 7, 256);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_group_size, xgroup_size_t, normal, 4, 4, 16);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size, xgroup_size_t, normal, 7, 7, 512);
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size, xgroup_size_t, normal, 6, 6, 32);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size, xgroup_size_t, normal, 64, 32, 256);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_group_size, xgroup_size_t, normal, 6, 6, 32);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size, xgroup_size_t, normal, 128, 64, 512);
#endif

#if defined(XBUILD_DEV) || defined(XBUILD_CI)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size, xgroup_size_t, normal, 6, 6, 32);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size, xgroup_size_t, normal, 8, 8, 512);
#elif defined(XBUILD_GALILEO)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size, xgroup_size_t, normal, 32, 8, 32);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size, xgroup_size_t, normal, 32, 32, 512);
#elif defined(XBUILD_BOUNTY)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size, xgroup_size_t, normal, 20, 8, 32);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size, xgroup_size_t, normal, 20, 20, 512);
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size, xgroup_size_t, normal, 32, 8, 32);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size, xgroup_size_t, normal, 256, 128, 512);
#endif

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_rotation_count, std::uint16_t, normal, 2, 1, 62);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_edge_group_size, std::uint16_t, normal, 512, 64, 1022);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size, std::uint16_t, normal, 512, 64, 1022);

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(rec_standby_pool_update_interval, xinterval_t, normal, 11, 1, std::numeric_limits<xinterval_t>::max());

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(zec_standby_pool_update_interval, xinterval_t, normal, 31, 1, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_logic_timeout_limitation,
                                      common::xlogic_time_t,
                                      normal,
                                      67,
                                      1,
                                      std::numeric_limits<common::xlogic_time_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_height_step_limitation, std::uint64_t, normal, 12, 1, std::numeric_limits<xinterval_t>::max());

// stake onchain:
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_stake, std::uint64_t, normal, 5000, 0, std::numeric_limits<std::uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(auditor_nodes_per_segment, std::uint32_t, normal, 27, 1, std::numeric_limits<std::uint32_t>::max());

// node registration:
#ifdef WORKLOAD_TEST
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_edge_deposit, std::uint64_t, normal, 10000, 0, std::numeric_limits<std::uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_deposit, std::uint64_t, normal, 10000, 0, std::numeric_limits<std::uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_deposit, std::uint64_t, normal, 10000, 0, std::numeric_limits<std::uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(dividend_ratio_change_interval, std::uint64_t, normal, 2, 0, std::numeric_limits<std::uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_vote_nodes_num, std::uint32_t, normal, 5, 1, std::numeric_limits<std::uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval, xinterval_t, normal, 10, 1, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval, xinterval_t, normal, 60, 1, std::numeric_limits<xinterval_t>::max());  // 10 minutes
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(reward_update_interval, xinterval_t, normal, 20, 1, std::numeric_limits<xinterval_t>::max()); // 200 seconds
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward, uint64_t, important, 100, 0, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_voter_dividend, uint64_t, important, 100, 0, std::numeric_limits<uint64_t>::max());
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_edge_deposit, std::uint64_t, normal, ASSET_TOP(200000), 0, std::numeric_limits<std::uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_deposit, std::uint64_t, normal, ASSET_TOP(500000), 0, std::numeric_limits<std::uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_deposit, std::uint64_t, normal, ASSET_TOP(1000000), 0, std::numeric_limits<std::uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(dividend_ratio_change_interval, std::uint64_t, normal, 14 * 24 * 3600 / 10, 0, std::numeric_limits<std::uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_vote_nodes_num, std::uint32_t, normal, 10000, 1, std::numeric_limits<std::uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval, xinterval_t, normal, 30, 1, std::numeric_limits<xinterval_t>::max());
#if defined(XBUILD_GALILEO)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval, xinterval_t, normal, 8640, 1, std::numeric_limits<xinterval_t>::max()); // 24 hours
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval, xinterval_t, normal, 4297, 1, std::numeric_limits<xinterval_t>::max()); // 12 hours
#endif
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(reward_update_interval, xinterval_t, normal, 17, 1, std::numeric_limits<xinterval_t>::max());  // 180 seconds
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward, uint64_t, important, 0, 0, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_voter_dividend, uint64_t, important, 0, 0, std::numeric_limits<uint64_t>::max());
#endif
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(workload_collection_interval, xinterval_t, normal, 12, 1, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_archive_deposit, std::uint64_t, normal, ASSET_TOP(1000000), 0, std::numeric_limits<std::uint64_t>::max());
// mainnet node active

#if defined(XBUILD_GALILEO)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_auditors, std::uint32_t, normal, 32, 0, std::numeric_limits<std::uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_validators, std::uint32_t, normal, 64, 0, std::numeric_limits<std::uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_votes, std::uint64_t, normal, 0, 0, std::numeric_limits<std::uint64_t>::max());
#elif defined(XBUILD_BOUNTY)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_auditors, std::uint32_t, normal, 20, 0, std::numeric_limits<std::uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_validators, std::uint32_t, normal, 20, 0, std::numeric_limits<std::uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_votes, std::uint64_t, normal, 0, 0, std::numeric_limits<std::uint64_t>::max());
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_auditors, std::uint32_t, normal, 128, 0, std::numeric_limits<std::uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_validators, std::uint32_t, normal, 512, 0, std::numeric_limits<std::uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_votes, std::uint64_t, normal, 384000000, 0, std::numeric_limits<std::uint64_t>::max());
#endif
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_edges, std::uint32_t, normal, 1, 0, std::numeric_limits<std::uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_mainnet_active_archives, std::uint32_t, normal, 1, 0, std::numeric_limits<std::uint32_t>::max());
// vote:
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_stake_votes_num, uint16_t, normal, 1, 1, std::numeric_limits<uint16_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_votes_num, uint32_t, normal, 1, 1, std::numeric_limits<uint32_t>::max());

// reward:
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(additional_issue_year_ratio, uint32_t, critical, 8, 0, 100);   // mean 8%
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_ratio_annual_total_reward, uint32_t, critical, 2, 0, 100); // mean 2%

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(vote_reward_ratio, uint16_t, critical, 20, 0, 100);      // mean 20%
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(governance_reward_ratio, uint16_t, critical, 4, 0, 100); // mean 4%
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(edge_reward_ratio, uint16_t, critical, 2, 0, 100);       // mean 2%
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio, uint16_t, critical, 10, 0, 100);   // mean 10%
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(validator_reward_ratio, uint16_t, critical, 60, 0, 100); // mean 60%
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(archive_reward_ratio, uint16_t, critical, 4, 0, 100);    // mean 4%

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tx, uint32_t, normal, 1, 1, std::numeric_limits<uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tableblock, uint32_t, normal, 2, 0, std::numeric_limits<uint32_t>::max());

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(auditor_group_zero_workload, uint32_t, normal, 0, 0, std::numeric_limits<uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_zero_workload, uint32_t, normal, 0, 0, std::numeric_limits<uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_height_step_limitation, uint64_t, normal, 12, 1, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_logic_timeout_limitation,
                                      common::xlogic_time_t,
                                      normal,
                                      60,
                                      1,
                                      std::numeric_limits<common::xlogic_time_t>::max());

// credit & slash
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(table_statistic_report_schedule_interval, xinterval_t, normal, 3, 0, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_credit, std::uint64_t, normal, 100000, 100000, 1000000);                     // default minimun 0.1
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(backward_validator_slash_credit, std::uint64_t, normal, 1 * 100000, 0, 1000000); // validator credit score 0.1;
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(backward_auditor_slash_credit, std::uint64_t, normal, 1 * 100000, 0, 1000000);   // auditor credit score 0.1
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_reward_threshold_value, std::uint32_t, normal, 80, 0, 100);           // award node persent
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_reward_threshold_value, std::uint32_t, normal, 0, 0, 100);    // award node vote
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_publishment_threshold_value, std::uint32_t, normal, 0, 0, 100);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(initial_creditscore, std::uint64_t, normal, 330000, 100000, 1000000);           // currently 0.33
#ifdef SLASH_TEST
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(punish_collection_interval, xinterval_t, normal, 30, 0, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block, xinterval_t, normal, 30, 0, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block, std::uint32_t, normal, 16, 0, std::numeric_limits<uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value, std::uint32_t, normal, 30, 0, 100);
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(award_validator_credit, std::uint64_t, normal, 1 * 10000, 0, 1000000); // validator credit score 0.01;
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(award_auditor_credit, std::uint64_t, normal, 1 * 10000, 0, 1000000);   // auditor credit score 0.01
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(backward_node_lock_duration_increment, std::uint64_t, normal, 30, 0, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_nodedeposit_lock_duration, std::uint64_t, normal, 1200, 0, std::numeric_limits<uint64_t>::max());
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(punish_collection_interval, xinterval_t, normal, 66, 0, std::numeric_limits<xinterval_t>::max()); // 11minute(just for a prime time)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block, xinterval_t, normal, 8640, 0, std::numeric_limits<xinterval_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block, std::uint32_t, normal, 368640, 0, std::numeric_limits<uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value, std::uint32_t, normal, 10, 0, 100);
#if defined(XBUILD_GALILEO)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(award_validator_credit, std::uint64_t, normal, 1 * 30000, 0, 1000000); // validator credit score 0.03;
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(award_auditor_credit, std::uint64_t, normal, 1 * 30000, 0, 1000000);   // auditor credit score 0.03
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(award_validator_credit, std::uint64_t, normal, 1 * 10000, 0, 1000000);                         // validator credit score 0.01;
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(award_auditor_credit, std::uint64_t, normal, 1 * 10000, 0, 1000000);                           // auditor credit score 0.01
#endif
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(backward_node_lock_duration_increment, std::uint64_t, normal, 103680, 0, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_nodedeposit_lock_duration, std::uint64_t, normal, 3110400, 0, std::numeric_limits<uint64_t>::max());
#endif

// tcc:
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_tcc_proposal_deposit, std::uint64_t, normal, ASSET_TOP(0), 0, std::numeric_limits<uint64_t>::max()); // min account activation depost

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(tcc_proposal_expire_time, std::uint32_t, normal, 259200, 0, std::numeric_limits<uint32_t>::max()); // 30 days
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(tcc_member, char const *, critical, "", "", "");

// whitelist
#if defined(XBUILD_CI) || defined(XBUILD_DEV) || defined(XBUILD_GALILEO) || defined(XBUILD_BOUNTY)
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist, bool, normal, false, false, true); // testnets defaults to close
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(toggle_whitelist, bool, normal, true, false, true); // mainnet defaults to open
#endif
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(whitelist, char const *, normal, "", "", "");

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance, std::uint32_t, critical, 300, 1, std::numeric_limits<uint32_t>::max()); // the transaction should arrive any
                                                                                                                                           // nodes in 5 minutes
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee, uint64_t, normal, ASSET_TOP(100), 0, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard, uint64_t, normal, 2160000000000, 1, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset, uint64_t, normal, ASSET_TOP(100), 1, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(free_gas, uint64_t, normal, 25000, 1, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio, uint64_t, normal, 20, 1, std::numeric_limits<uint64_t>::max());
// how many micro second 1 tgas can consume
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(cpu_gas_exchange_ratio, uint32_t, normal, 40, 1, std::numeric_limits<uint32_t>::max());

#ifdef ENABLE_SCALE
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle, xinterval_t, normal, 2 * 6, 1, std::numeric_limits<xinterval_t>::max());
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle, xinterval_t, normal, 24 * 60 * 6, 1, std::numeric_limits<xinterval_t>::max());
#endif
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit, uint32_t, normal, ASSET_uTOP(100000), 1, std::numeric_limits<uint32_t>::max());
#ifdef ENABLE_SCALE
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(unlock_gas_staked_delay_time, xinterval_t, normal, 6, 1, std::numeric_limits<xinterval_t>::max());
#else
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(unlock_gas_staked_delay_time, xinterval_t, normal, 24 * 60 * 6, 1, std::numeric_limits<xinterval_t>::max());
#endif
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account, uint64_t, normal, 1000000, 1, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract, uint64_t, normal, 50000000, 1, std::numeric_limits<uint64_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token, uint64_t, normal, ASSET_TOP(1000000000), 1, std::numeric_limits<uint64_t>::max());

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num, std::uint32_t, critical, 21, 1, std::numeric_limits<uint32_t>::max());

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_name_max_len, std::uint32_t, critical, 16, 1, std::numeric_limits<uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(custom_property_max_number, std::uint32_t, critical, 128, 1, std::numeric_limits<uint32_t>::max());
XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(application_contract_code_max_len, std::uint32_t, critical, 32768, 1, std::numeric_limits<uint32_t>::max());

XDECLARE_ONCHAIN_GOVERNANCE_PARAMETER(contract_call_contracts_num, std::uint32_t, critical, 25, 1, std::numeric_limits<uint32_t>::max());

/* begin of offchain parameters */
XDECLARE_CONFIGURATION(zone_count, std::uint32_t, 1);
XDECLARE_CONFIGURATION(cluster_count, std::uint32_t, 1);
#if defined(XBUILD_CI)
XDECLARE_CONFIGURATION(auditor_group_count, std::uint16_t, 1);
XDECLARE_CONFIGURATION(validator_group_count, std::uint16_t, 1);
#elif defined(XBUILD_DEV)
XDECLARE_CONFIGURATION(auditor_group_count, std::uint16_t, 1);
XDECLARE_CONFIGURATION(validator_group_count, std::uint16_t, 2);
#else
XDECLARE_CONFIGURATION(auditor_group_count, std::uint16_t, 2);
XDECLARE_CONFIGURATION(validator_group_count, std::uint16_t, 4);
#endif
XDECLARE_CONFIGURATION(archive_group_count, std::uint16_t, 2);

XDECLARE_CONFIGURATION(min_edge_archive_deposit, std::uint64_t, ASSET_TOP(0));

XDECLARE_CONFIGURATION(min_account_deposit, std::uint64_t, ASSET_TOP(0)); // min account activation deposit unnecessary
XDECLARE_CONFIGURATION(recv_tx_cache_window, std::uint32_t, 30);
XDECLARE_CONFIGURATION(account_send_queue_tx_max_num, std::uint32_t, 16);
XDECLARE_CONFIGURATION(config_property_alias_name_max_len, std::uint32_t, 32);
XDECLARE_CONFIGURATION(edge_max_msg_packet_size, std::uint32_t, 50000);
XDECLARE_CONFIGURATION(executor_max_total_sessions_service_counts, std::uint32_t, 1000); // service count all sessions per time interval
XDECLARE_CONFIGURATION(executor_max_session_service_counts, std::uint32_t, 600);         // service count per session per time interval
XDECLARE_CONFIGURATION(executor_session_time_interval, std::uint32_t, 60);               // seconds
XDECLARE_CONFIGURATION(executor_max_sessions, std::uint32_t, 10000);                     // max session in cache
XDECLARE_CONFIGURATION(leader_election_round, std::uint32_t, 2);
#ifdef NO_TX_BATCH
XDECLARE_CONFIGURATION(unitblock_confirm_tx_batch_num, std::uint32_t, 1);
XDECLARE_CONFIGURATION(unitblock_recv_transfer_tx_batch_num, std::uint32_t, 1);
XDECLARE_CONFIGURATION(unitblock_send_transfer_tx_batch_num, std::uint32_t, 1);
XDECLARE_CONFIGURATION(tableblock_batch_unitblock_max_num, std::uint32_t, 1);
XDECLARE_CONFIGURATION(tableblock_batch_tx_max_num, std::int32_t, 1);
#else
XDECLARE_CONFIGURATION(unitblock_confirm_tx_batch_num, std::uint32_t, 8);
XDECLARE_CONFIGURATION(unitblock_recv_transfer_tx_batch_num, std::uint32_t, 4);
XDECLARE_CONFIGURATION(unitblock_send_transfer_tx_batch_num, std::uint32_t, 3);
XDECLARE_CONFIGURATION(tableblock_batch_unitblock_max_num, std::uint32_t, 64);
XDECLARE_CONFIGURATION(tableblock_batch_tx_max_num, std::int32_t, 64);
#endif

#if defined(XBUILD_DEV) || defined(XBUILD_CI)
XDECLARE_CONFIGURATION(fulltable_interval_block_num, std::uint32_t, 16);
#else
XDECLARE_CONFIGURATION(fulltable_interval_block_num, std::uint32_t, 128); // TODO(jimmy) 512
#endif
XDECLARE_CONFIGURATION(local_blacklist, const char *, "");
XDECLARE_CONFIGURATION(local_whitelist, const char *, "");
#if defined(XBUILD_DEV) || defined(XBUILD_CI)
XDECLARE_CONFIGURATION(slash_fulltable_interval, xinterval_t, 30); // 5 minutes
XDECLARE_CONFIGURATION(slash_table_split_num, uint16_t, 4);
#else
XDECLARE_CONFIGURATION(slash_table_split_num, uint16_t, 32);              // split num (should divisible by total table num)
// slash fulltable interval
XDECLARE_CONFIGURATION(slash_fulltable_interval, xinterval_t, 120); // 20 minutes
#endif

/* beginning of development parameters */
XDECLARE_CONFIGURATION(http_port, uint16_t, 19081);
XDECLARE_CONFIGURATION(grpc_port, uint16_t, 19082);
XDECLARE_CONFIGURATION(dht_port, uint16_t, 19083);
XDECLARE_CONFIGURATION(msg_port, uint16_t, 19084);
XDECLARE_CONFIGURATION(ws_port, uint16_t, 19085);
XDECLARE_CONFIGURATION(log_level, uint16_t, 0);
XDECLARE_CONFIGURATION(network_id, uint32_t, 0);
XDECLARE_CONFIGURATION(log_path, const char *, "/chain/log/clog"); // config log path
XDECLARE_CONFIGURATION(db_path, const char *, "/chain/db_v2/cdb"); // config log path
XDECLARE_CONFIGURATION(ip, const char *, "0.0.0.0");
XDECLARE_CONFIGURATION(auto_prune_data, const char *, "off");

/* end of development parameters */

/* end of offchain parameters */

#if defined(XBUILD_CI)
XDECLARE_CONFIGURATION(chain_name, char const *, chain_name_testnet);
XDECLARE_CONFIGURATION(platform_public_endpoints,
                       char const *,
                       "192.168.50.155:9921,192.168.50.156:9921,192.168.50.157:9921,192.168.50.158:9921,192.168.50.159:9921,192.168.50.160:9921,192.168.50.121:9921,192.168.50.119:9921");
XDECLARE_CONFIGURATION(platform_url_endpoints, char const *, "http://mainnetwork.org/");
XDECLARE_CONFIGURATION(root_hash, char const *, "");
#elif defined(XBUILD_DEV)
XDECLARE_CONFIGURATION(chain_name, char const *, chain_name_testnet);
XDECLARE_CONFIGURATION(platform_public_endpoints,
                       char const *,
                       "127.0.0.1:9000");
XDECLARE_CONFIGURATION(platform_url_endpoints, char const *, "http://unreachable.org/");
XDECLARE_CONFIGURATION(root_hash, char const *, "");
#elif defined(XBUILD_GALILEO)
XDECLARE_CONFIGURATION(chain_name, char const *, chain_name_testnet);
XDECLARE_CONFIGURATION(platform_public_endpoints,
                       char const *,
                       "206.189.201.14:9000,167.172.128.168:9000,206.189.194.250:9000,134.122.123.81:9000");
XDECLARE_CONFIGURATION(platform_url_endpoints, char const *, "http://galileo.seed.topnetwork.org/");
XDECLARE_CONFIGURATION(root_hash, char const *, "a450ce6a875d55024f60b7ac2a1e9984689ccc6b5de1690890c3b59d588a2ad0");
#elif defined(XBUILD_BOUNTY)
XDECLARE_CONFIGURATION(chain_name, char const *, chain_name_testnet);
XDECLARE_CONFIGURATION(platform_public_endpoints,
                       char const *,
                       "161.35.98.159:9000,137.184.102.85:9000,137.184.68.56:9000,143.198.189.238:9000,147.182.173.6:9000,137.184.106.236:9000,137.184.106.135:9000,137.184.21.161:9000,137.184.66.111:9000,137.184.106.34:9000,138.197.15.200:9000,159.203.163.160:9000,142.93.73.113:9000,142.93.65.231:9000,159.89.184.201:9000,159.89.178.45:9000,159.65.45.247:9000,142.93.73.207:9000,167.71.110.183:9000,142.93.73.67:9000");
XDECLARE_CONFIGURATION(platform_url_endpoints, char const *, "http://bounty.seed.topnetwork.org/");
XDECLARE_CONFIGURATION(root_hash, char const *, "");
#else
XDECLARE_CONFIGURATION(chain_name, char const *, chain_name_mainnet);
XDECLARE_CONFIGURATION(platform_public_endpoints,
                       char const *,
                       "206.189.227.204:9000,206.189.238.224:9000,206.189.205.198:9000,204.48.27.142:9000,206.81.0.133:9000");
XDECLARE_CONFIGURATION(platform_url_endpoints, char const *, "http://mainnet.seed.topnetwork.org/");
XDECLARE_CONFIGURATION(root_hash, char const *, "beaa468a921c7cb0344da5b56fcf79ccdbcddb3226a1c042a1020be6d3fc29f2");
#endif

XDECLARE_CONFIGURATION(platform_business_port, std::uint16_t, 9000);
XDECLARE_CONFIGURATION(platform_show_cmd, bool, false);
XDECLARE_CONFIGURATION(platform_db_path, char const *, "/chain/db_v2/pdb");

#undef XDECLARE_CONFIGURATION

NS_END2
