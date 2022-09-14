// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xsystem_contract/xelection_algorithm/xtest_elect_group_contract_fixture.h"

NS_BEG3(top, tests, election)

TEST_F(xtest_elect_nonconsensus_group_contract_fixture_t, test_edge) {
    common::xnode_type_t node_type{common::xnode_type_t::edge};
    common::xzone_id_t zid{common::xedge_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xdefault_group_id};
    xrange_t<config::xgroup_size_t> group_size_range{0, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_edge_group_size)};

    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 0);
    ASSERT_FALSE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 0, 0, group_size_range, false, standby_network_result, election_network_result));

    add_nodes_to_standby(10, node_type, TEST_NODE_ID_PREFIX);
    auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);

    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 10);
    ASSERT_TRUE(election_result.empty());
    ASSERT_TRUE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 0, 0, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), 10);
    ASSERT_EQ(election_result.start_time(), common::xlogic_time_t{0});
    ASSERT_EQ(election_result.timestamp(), common::xlogic_time_t{0});
    ASSERT_FALSE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 1, 1, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), 10);
    ASSERT_EQ(election_result.start_time(), common::xlogic_time_t{0});
    ASSERT_EQ(election_result.timestamp(), common::xlogic_time_t{0});

    dereg_nodes_from_standby(10, node_type, TEST_NODE_ID_PREFIX);
    add_nodes_to_standby(600, node_type, TEST_NODE_ID_PREFIX);
    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 600);
    ASSERT_TRUE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 2, 2, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size));
}

TEST_F(xtest_elect_nonconsensus_group_contract_fixture_t, test_archive) {
    common::xnode_type_t node_type{common::xnode_type_t::storage_archive};
    common::xzone_id_t zid{common::xstorage_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xdefault_group_id};
    xrange_t<config::xgroup_size_t> group_size_range{1, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size)};

    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 0);
    ASSERT_FALSE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 0, 0, group_size_range, false, standby_network_result, election_network_result));

    add_nodes_to_standby(10, node_type, TEST_NODE_ID_PREFIX);
    auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);

    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 10);
    ASSERT_TRUE(election_result.empty());
    ASSERT_TRUE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 0, 0, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), 10);
    ASSERT_EQ(election_result.start_time(), common::xlogic_time_t{0});
    ASSERT_EQ(election_result.timestamp(), common::xlogic_time_t{0});
    ASSERT_FALSE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 1, 1, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), 10);
    ASSERT_EQ(election_result.start_time(), common::xlogic_time_t{0});
    ASSERT_EQ(election_result.timestamp(), common::xlogic_time_t{0});

    dereg_nodes_from_standby(10, node_type, TEST_NODE_ID_PREFIX);
    add_nodes_to_standby(600, node_type, TEST_NODE_ID_PREFIX);
    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 600);
    ASSERT_TRUE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 2, 2, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size));
}

TEST_F(xtest_elect_nonconsensus_group_contract_fixture_t, test_force_update) {
    common::xnode_type_t node_type{common::xnode_type_t::storage_archive};
    common::xzone_id_t zid{common::xstorage_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xarchive_group_id};
    xrange_t<config::xgroup_size_t> group_size_range{1, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size)};

    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 0);
    ASSERT_FALSE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 0, 0, group_size_range, false, standby_network_result, election_network_result));

    add_nodes_to_standby(10, node_type, TEST_NODE_ID_PREFIX);
    auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);

    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 10);
    ASSERT_TRUE(election_result.empty());
    ASSERT_TRUE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 0, 0, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), 10);
    ASSERT_EQ(election_result.start_time(), common::xlogic_time_t{0});
    ASSERT_EQ(election_result.timestamp(), common::xlogic_time_t{0});
    ASSERT_FALSE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 1, 1, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), 10);
    ASSERT_EQ(election_result.start_time(), common::xlogic_time_t{0});
    ASSERT_EQ(election_result.timestamp(), common::xlogic_time_t{0});
    ASSERT_TRUE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 1, 1, group_size_range, true, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), 10);
    ASSERT_EQ(election_result.start_time(), common::xlogic_time_t{1});
    ASSERT_EQ(election_result.timestamp(), common::xlogic_time_t{1});

    dereg_nodes_from_standby(10, node_type, TEST_NODE_ID_PREFIX);
    add_nodes_to_standby(600, node_type, TEST_NODE_ID_PREFIX);
    ASSERT_EQ(standby_network_result.result_of(node_type).size(), 600);
    ASSERT_TRUE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 2, 2, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size));
    ASSERT_FALSE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 3, 3, group_size_range, false, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size));
    ASSERT_TRUE(m_elect_nonconsensus_group.test_elect(zid, cid, gid, 3, 3, group_size_range, true, standby_network_result, election_network_result));
    ASSERT_EQ(election_result.size(), XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_archive_group_size));
}

NS_END3
