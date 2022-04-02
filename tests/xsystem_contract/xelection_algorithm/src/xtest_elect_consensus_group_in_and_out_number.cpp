// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xsystem_contract/xelection_algorithm/xtest_elect_group_contract_fixture.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xelect_transaction.hpp"

NS_BEG3(top, tests, election)

TEST_F(xtest_elect_consensus_group_contract_fixture_t, test_auditor_genesis) {
    top::config::config_register.get_instance().set(config::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(3));
    top::config::config_register.get_instance().set(config::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(4));

    common::xnode_type_t node_type{common::xnode_type_t::consensus_auditor};
    common::xzone_id_t zid{common::xconsensus_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xauditor_group_id_begin};
    auto const min_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
    auto const max_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);
    xrange_t<config::xgroup_size_t> group_size_range{min_auditor_group_size, max_auditor_group_size};
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    auto random_seed = static_cast<uint64_t>(rng());

    add_nodes_to_standby(10, node_type, TEST_NODE_ID_PREFIX);
    auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);

    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 10);
    ASSERT_TRUE(election_result.empty());
    ASSERT_TRUE(m_elect_consensus_group.test_elect(zid, cid, gid, 0, 0, random_seed, group_size_range, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), min_auditor_group_size);
    ASSERT_EQ(election_result.start_time(), common::xlogic_time_t{0});
    ASSERT_EQ(election_result.timestamp(), common::xlogic_time_t{0});
}

TEST_F(xtest_elect_consensus_group_contract_fixture_t, test_validator_genesis) {
    top::config::config_register.get_instance().set(config::xmin_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(3));
    top::config::config_register.get_instance().set(config::xmax_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(4));

    common::xnode_type_t node_type{common::xnode_type_t::consensus_validator};
    common::xzone_id_t zid{common::xconsensus_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xvalidator_group_id_begin};
    auto const min_validator_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_group_size);
    auto const max_validator_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size);
    xrange_t<config::xgroup_size_t> group_size_range{min_validator_group_size, max_validator_group_size};
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    auto random_seed = static_cast<uint64_t>(rng());

    add_nodes_to_standby(10, node_type, TEST_NODE_ID_PREFIX);
    auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);

    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 10);
    ASSERT_TRUE(election_result.empty());
    ASSERT_TRUE(m_elect_consensus_group.test_elect(zid, cid, gid, 0, 0, random_seed, group_size_range, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), min_validator_group_size);
}

// @ test_case_name : the name in TEST_F(class, name)
// @ group_min : min_group_size
// @ group_max : max_group_size
// @ current_group_size : the election_result.size(), default set the node_id : TEST_NODE_ID_PREFIX + [1,size]
// @ effective_group_size : the standby_result.size(), default set the node_id : TEST_NODE_ID_PREFIX + [1,size]
// @ dereg_num : the deregister nodes, default set the node_id: TEST_NODE_ID_PREFIX + [1,dereg_num]
// @ expect_election_result : bool_value , if the election success
// @ expect_elected_in_num: (if success) the nodes number that elected in
// @ expect_elected_out_num: (if success) the nodes number that elected out
#define TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(                                                                                                                                  \
    test_case_name, group_min, group_max, current_group_size, effective_group_size, dereg_num, expect_election_result, expect_elected_in_num, expect_elected_out_num)              \
    TEST_F(xtest_elect_consensus_group_contract_fixture_t, test_case_name) {                                                                                                       \
        top::config::config_register.get_instance().set(config::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(group_min));                                           \
        top::config::config_register.get_instance().set(config::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(group_max));                                           \
        common::xnode_type_t node_type{common::xnode_type_t::consensus_auditor};                                                                                                   \
        common::xzone_id_t zid{common::xconsensus_zone_id};                                                                                                                        \
        common::xcluster_id_t cid{common::xdefault_cluster_id};                                                                                                                    \
        common::xgroup_id_t gid{common::xauditor_group_id_begin};                                                                                                                  \
        auto const min_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);                                                                                                   \
        auto const max_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);                                                                                                   \
        xrange_t<config::xgroup_size_t> group_size_range{min_auditor_group_size, max_auditor_group_size};                                                                            \
        std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());                                                                                 \
        auto random_seed = static_cast<uint64_t>(rng());                                                                                                                           \
        EXPECT_TRUE(add_nodes_to_standby(effective_group_size, node_type, TEST_NODE_ID_PREFIX));                                                                                   \
        EXPECT_TRUE(add_nodes_to_election_result(current_group_size, node_type, cid, gid, TEST_NODE_ID_PREFIX));                                                                   \
        EXPECT_TRUE(dereg_nodes_from_standby(dereg_num, node_type, TEST_NODE_ID_PREFIX));                                                                                          \
        auto const election_result_backup = election_network_result.result_of(node_type).result_of(cid).result_of(gid);                                                            \
        auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);                                                                       \
        ASSERT_EQ(expect_election_result,                                                                                                                                          \
                  m_elect_consensus_group.test_elect(zid, cid, gid, 0, 0, random_seed, group_size_range, standby_network_result, election_network_result));                        \
        if (expect_election_result) {                                                                                                                                              \
            std::size_t actual_in_num{0};                                                                                                                                          \
            std::size_t actual_out_num{0};                                                                                                                                         \
            for (auto node_info : election_result_backup) {                                                                                                                        \
                auto node_id = top::get<xelection_info_bundle_t>(node_info).account_address();                                                                                     \
                if (!election_result.find(node_id).second) {                                                                                                                       \
                    ++actual_out_num;                                                                                                                                              \
                }                                                                                                                                                                  \
            }                                                                                                                                                                      \
            for (auto node_info : election_result) {                                                                                                                               \
                auto node_id = top::get<xelection_info_bundle_t>(node_info).account_address();                                                                                     \
                if (!election_result_backup.find(node_id).second) {                                                                                                                \
                    ++actual_in_num;                                                                                                                                               \
                }                                                                                                                                                                  \
            }                                                                                                                                                                      \
            EXPECT_EQ(expect_elected_in_num, actual_in_num);                                                                                                                       \
            EXPECT_EQ(expect_elected_out_num, actual_out_num);                                                                                                                     \
        }                                                                                                                                                                          \
    }

TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus1, 3, 4, 0, 5, 0, true, 3, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus2, 3, 4, 3, 6, 1, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus3, 3, 4, 4, 6, 1, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus4, 3, 4, 4, 9, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus5, 3, 4, 3, 5, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus6, 3, 14, 13, 15, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus7, 3, 5, 4, 7, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus8, 3, 14, 13, 16, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus9, 3, 6, 4, 6, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus10, 3, 6, 4, 7, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus11, 3, 15, 13, 15, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus12, 3, 15, 13, 16, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus13, 3, 4, 4, 5, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus14, 3, 4, 4, 6, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus15, 3, 13, 13, 15, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus16, 3, 4, 4, 7, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus17, 3, 13, 13, 16, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus18, 3, 4, 3, 4, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus19, 3, 4, 3, 4, 1, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus20, 3, 4, 3, 5, 1, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus21, 3, 15, 13, 15, 1, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus22, 3, 15, 13, 16, 2, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus23, 3, 4, 4, 5, 1, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus24, 3, 4, 4, 6, 1, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus25, 3, 13, 13, 16, 2, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus26, 3, 13, 13, 16, 3, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus27, 23, 64, 0, 60, 0, true, 23, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus28, 23, 64, 23, 60, 0, true, 1, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus29, 23, 64, 26, 60, 0, true, 2, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus30, 23, 64, 30, 60, 0, true, 2, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus31, 23, 64, 34, 60, 0, true, 2, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus32, 23, 64, 39, 60, 0, true, 3, 3);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus33, 23, 64, 39, 60, 3, true, 3, 3);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus34, 23, 64, 39, 60, 7, true, 3, 3);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus35, 23, 64, 50, 600, 0, true, 4, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus36, 23, 64, 58, 600, 0, true, 4, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus37, 23, 64, 58, 600, 3, true, 4, 3);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus38, 23, 64, 58, 600, 12, true, 4, 4);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus39, 23, 64, 64, 70, 0, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus40, 23, 64, 64, 70, 3, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus41, 23, 64, 64, 70, 11, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus42, 23, 64, 64, 600, 0, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus43, 23, 64, 64, 600, 3, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus44, 23, 64, 64, 600, 11, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus45, 23, 126, 126, 210, 15, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus46, 23, 126, 126, 300, 15, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus47, 23, 126, 126, 211, 0, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus48, 23, 126, 126, 211, 15, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus49, 23, 126, 126, 500, 0, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus50, 23, 126, 126, 500, 15, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus51, 23, 126, 126, 500, 17, true, 10, 10);

TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus53, 23, 126, 24, 24, 0, false, 0, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus54, 23, 126, 24, 24, 5, false, 0, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus55, 23, 126, 24, 27, 0, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus56, 23, 126, 24, 27, 2, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus57, 23, 126, 24, 27, 5, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus58, 23, 126, 24, 80, 0, true, 1, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus59, 23, 126, 24, 80, 3, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus60, 23, 126, 24, 80, 4, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus61, 23, 126, 24, 80, 7, true, 1, 1);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus62, 23, 126, 66, 66, 0, false, 0, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus63, 23, 126, 66, 66, 5, false, 0, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus64, 23, 126, 66, 80, 0, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus65, 23, 126, 66, 80, 8, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus66, 23, 126, 66, 80, 15, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus67, 23, 126, 66, 200, 0, true, 5, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus68, 23, 126, 66, 200, 8, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus69, 23, 126, 66, 200, 11, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus70, 23, 126, 66, 200, 15, true, 5, 5);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus71, 23, 126, 126, 126, 0, false, 0, 0);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus72, 23, 126, 126, 130, 0, true, 4, 4);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus73, 23, 126, 126, 130, 2, true, 4, 4);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus74, 23, 126, 126, 138, 3, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus75, 23, 126, 126, 138, 15, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus76, 23, 126, 126, 300, 0, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus77, 23, 126, 126, 300, 15, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus78, 23, 126, 126, 300, 18, true, 10, 10);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus79, 23, 126, 126, 300, 30, true, 10, 10);

TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus81, 23, 64, 100, 101, 0, true, 0, 36);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus82, 23, 64, 100, 110, 10, true, 0, 36);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus83, 23, 64, 100, 110, 50, true, 0, 36);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus84, 23, 64, 100, 100, 0, true, 0, 36);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus85, 23, 64, 100, 200, 40, true, 0, 36);
TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(test_consensus86, 23, 64, 100, 200, 100, true, 0, 36);

TEST_CONSENSUS_ELECTION_IN_AND_OUT_COUNT(testS1, 12, 18, 3, 6, 0, false, 0, 0);

TEST_F(xtest_elect_consensus_group_contract_fixture_t, example) {
    top::config::config_register.get_instance().set(config::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(3));
    top::config::config_register.get_instance().set(config::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(4));
    common::xnode_type_t node_type{common::xnode_type_t::consensus_auditor};
    common::xzone_id_t zid{common::xconsensus_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xauditor_group_id_begin};
    auto const min_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
    auto const max_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);
    xrange_t<config::xgroup_size_t> group_size_range{min_auditor_group_size, max_auditor_group_size};
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    auto random_seed = static_cast<uint64_t>(rng());
    EXPECT_TRUE(add_nodes_to_standby(5, node_type, TEST_NODE_ID_PREFIX));
    EXPECT_TRUE(add_nodes_to_election_result(3, node_type, cid, gid, TEST_NODE_ID_PREFIX));
    EXPECT_TRUE(dereg_nodes_from_standby(1, node_type, TEST_NODE_ID_PREFIX));
    auto const election_result_backup = election_network_result.result_of(node_type).result_of(cid).result_of(gid);
    auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);
    bool expect_election_result = true;
    ASSERT_EQ(expect_election_result, m_elect_consensus_group.test_elect(zid, cid, gid, 0, 0, random_seed, group_size_range, standby_network_result, election_network_result));
    if (expect_election_result) {
        std::size_t expect_elected_in_num{1};
        std::size_t expect_elected_out_num{1};
        std::size_t actual_in_num{0};
        std::size_t actual_out_num{0};
        for (auto node_info : election_result_backup) {
            auto node_id = top::get<xelection_info_bundle_t>(node_info).account_address();
            if (!election_result.find(node_id).second) {
                ++actual_out_num;
            }
        }
        for (auto node_info : election_result) {
            auto node_id = top::get<xelection_info_bundle_t>(node_info).account_address();
            if (!election_result_backup.find(node_id).second) {
                ++actual_in_num;
            }
        }
        EXPECT_EQ(expect_elected_in_num, actual_in_num);
        EXPECT_EQ(expect_elected_out_num, actual_out_num);
    }
}

TEST_F(xtest_elect_consensus_group_contract_fixture_t, testSP1) {
    top::config::config_register.get_instance().set(config::xmin_election_committee_size_onchain_goverance_parameter_t::name, std::to_string(3));
    top::config::config_register.get_instance().set(config::xmax_election_committee_size_onchain_goverance_parameter_t::name, std::to_string(4));
    common::xnode_type_t node_type{common::xnode_type_t::committee};
    common::xzone_id_t zid{common::xcommittee_zone_id};
    common::xcluster_id_t cid{common::xcommittee_cluster_id};
    common::xgroup_id_t gid{common::xcommittee_group_id};
    auto const min_election_committee_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size);
    auto const max_election_committee_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size);

    xrange_t<config::xgroup_size_t> group_size_range{min_election_committee_size, max_election_committee_size};
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    auto random_seed = static_cast<uint64_t>(rng());
    EXPECT_TRUE(add_nodes_to_standby(1, node_type, TEST_NODE_ID_PREFIX));
    EXPECT_FALSE(m_elect_consensus_group.test_elect(zid, cid, gid, 0, 0, random_seed, group_size_range, standby_network_result, election_network_result));

    common::xnode_type_t node_type2{common::xnode_type_t::zec};
    common::xzone_id_t zid2{common::xzec_zone_id};
    EXPECT_TRUE(add_nodes_to_standby(1, node_type2, TEST_NODE_ID_PREFIX));
    EXPECT_FALSE(m_elect_consensus_group.test_elect(zid2, cid, gid, 0, 0, random_seed, group_size_range, standby_network_result, election_network_result));
}

#define TEST_EC_ELECTION_IN_AND_OUT_COUNT(                                                                                                                                         \
    test_case_name, group_min, group_max, current_group_size, effective_group_size, dereg_num, expect_election_result, expect_elected_in_num, expect_elected_out_num)              \
    TEST_F(xtest_elect_consensus_group_contract_fixture_t, TEST_EC_##test_case_name) {                                                                                             \
        top::config::config_register.get_instance().set(config::xmin_election_committee_size_onchain_goverance_parameter_t::name, std::to_string(group_min));                                      \
        top::config::config_register.get_instance().set(config::xmax_election_committee_size_onchain_goverance_parameter_t::name, std::to_string(group_max));                                      \
        common::xnode_type_t node_type{common::xnode_type_t::committee};                                                                                                           \
        common::xzone_id_t zid{common::xcommittee_zone_id};                                                                                                                        \
        common::xcluster_id_t cid{common::xcommittee_cluster_id};                                                                                                                  \
        common::xgroup_id_t gid{common::xcommittee_group_id};                                                                                                                      \
        auto const min_election_committee_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size);                                                                                         \
        auto const max_election_committee_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size);                                                                                         \
        xrange_t<config::xgroup_size_t> group_size_range{min_election_committee_size, max_election_committee_size};                                                                  \
        std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());                                                                                 \
        auto random_seed = static_cast<uint64_t>(rng());                                                                                                                           \
        EXPECT_TRUE(add_nodes_to_standby(effective_group_size, node_type, TEST_NODE_ID_PREFIX));                                                                                   \
        EXPECT_TRUE(add_nodes_to_election_result(current_group_size, node_type, cid, gid, TEST_NODE_ID_PREFIX));                                                                   \
        EXPECT_TRUE(dereg_nodes_from_standby(dereg_num, node_type, TEST_NODE_ID_PREFIX));                                                                                          \
        auto const election_result_backup = election_network_result.result_of(node_type).result_of(cid).result_of(gid);                                                            \
        auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);                                                                       \
        ASSERT_EQ(expect_election_result,                                                                                                                                          \
                  m_elect_consensus_group.test_elect(zid, cid, gid, 0, 0, random_seed, group_size_range, standby_network_result, election_network_result));                        \
        if (expect_election_result) {                                                                                                                                              \
            std::size_t actual_in_num{0};                                                                                                                                          \
            std::size_t actual_out_num{0};                                                                                                                                         \
            for (auto node_info : election_result_backup) {                                                                                                                        \
                auto node_id = top::get<xelection_info_bundle_t>(node_info).account_address();                                                                                     \
                if (!election_result.find(node_id).second) {                                                                                                                       \
                    ++actual_out_num;                                                                                                                                              \
                }                                                                                                                                                                  \
            }                                                                                                                                                                      \
            for (auto node_info : election_result) {                                                                                                                               \
                auto node_id = top::get<xelection_info_bundle_t>(node_info).account_address();                                                                                     \
                if (!election_result_backup.find(node_id).second) {                                                                                                                \
                    ++actual_in_num;                                                                                                                                               \
                }                                                                                                                                                                  \
            }                                                                                                                                                                      \
            EXPECT_EQ(expect_elected_in_num, actual_in_num);                                                                                                                       \
            EXPECT_EQ(expect_elected_out_num, actual_out_num);                                                                                                                     \
        }                                                                                                                                                                          \
    }

TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec1, 3, 4, 0, 5, 0, true, 3, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec2, 3, 4, 3, 6, 1, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec3, 3, 4, 4, 6, 1, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec4, 3, 4, 4, 9, 0, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec5, 3, 4, 3, 5, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec6, 3, 14, 13, 15, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec7, 3, 5, 4, 7, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec8, 3, 14, 13, 16, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec9, 3, 6, 4, 6, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec10, 3, 6, 4, 7, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec11, 3, 15, 13, 15, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec12, 3, 15, 13, 16, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec13, 3, 4, 4, 5, 0, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec14, 3, 4, 4, 6, 0, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec15, 3, 13, 13, 15, 0, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec16, 3, 4, 4, 7, 0, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec17, 3, 13, 13, 16, 0, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec18, 3, 4, 3, 4, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec19, 3, 4, 3, 4, 1, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec20, 3, 4, 3, 5, 1, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec21, 3, 15, 13, 15, 1, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec22, 3, 15, 13, 16, 2, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec23, 3, 4, 4, 5, 1, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec24, 3, 4, 4, 6, 1, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec25, 3, 13, 13, 16, 2, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec26, 3, 13, 13, 16, 3, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec27, 23, 64, 0, 60, 0, true, 23, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec28, 23, 64, 23, 60, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec29, 23, 64, 26, 60, 0, true, 2, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec30, 23, 64, 30, 60, 0, true, 2, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec31, 23, 64, 34, 60, 0, true, 2, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec32, 23, 64, 39, 60, 0, true, 3, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec33, 23, 64, 39, 60, 3, true, 3, 3);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec34, 23, 64, 39, 60, 7, true, 3, 3);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec35, 23, 64, 50, 600, 0, true, 4, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec36, 23, 64, 58, 600, 0, true, 4, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec37, 23, 64, 58, 600, 3, true, 4, 3);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec38, 23, 64, 58, 600, 12, true, 4, 4);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec39, 23, 64, 64, 70, 0, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec40, 23, 64, 64, 70, 3, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec41, 23, 64, 64, 70, 11, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec42, 23, 64, 64, 600, 0, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec43, 23, 64, 64, 600, 3, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec44, 23, 64, 64, 600, 11, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec45, 23, 126, 126, 210, 15, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec46, 23, 126, 126, 300, 15, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec47, 23, 126, 126, 211, 0, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec48, 23, 126, 126, 211, 15, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec49, 23, 126, 126, 500, 0, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec50, 23, 126, 126, 500, 15, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec51, 23, 126, 126, 500, 17, true, 10, 10);

TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec53, 23, 126, 24, 24, 0, false, 3, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec54, 23, 126, 24, 24, 5, false, 3, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec55, 23, 126, 24, 27, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec56, 23, 126, 24, 27, 2, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec57, 23, 126, 24, 27, 5, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec58, 23, 126, 24, 80, 0, true, 1, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec59, 23, 126, 24, 80, 3, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec60, 23, 126, 24, 80, 4, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec61, 23, 126, 24, 80, 7, true, 1, 1);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec62, 23, 126, 66, 66, 0, false, 10, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec63, 23, 126, 66, 66, 5, false, 10, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec64, 23, 126, 66, 80, 0, true, 5, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec65, 23, 126, 66, 80, 8, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec66, 23, 126, 66, 80, 15, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec67, 23, 126, 66, 200, 0, true, 5, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec68, 23, 126, 66, 200, 8, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec69, 23, 126, 66, 200, 11, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec70, 23, 126, 66, 200, 15, true, 5, 5);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec71, 23, 126, 126, 126, 0, false, 0, 0);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec72, 23, 126, 126, 130, 0, true, 4, 4);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec73, 23, 126, 126, 130, 2, true, 4, 4);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec74, 23, 126, 126, 138, 3, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec75, 23, 126, 126, 138, 15, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec76, 23, 126, 126, 300, 0, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec77, 23, 126, 126, 300, 15, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec78, 23, 126, 126, 300, 18, true, 10, 10);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec79, 23, 126, 126, 300, 30, true, 10, 10);

TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec81, 23, 64, 100, 101, 0, true, 0, 36);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec82, 23, 64, 100, 110, 10, true, 0, 36);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec83, 23, 64, 100, 110, 50, true, 0, 36);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec84, 23, 64, 100, 100, 0, true, 0, 36);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec85, 23, 64, 100, 200, 40, true, 0, 36);
TEST_EC_ELECTION_IN_AND_OUT_COUNT(test_ec86, 23, 64, 100, 200, 100, true, 0, 36);

NS_END3
