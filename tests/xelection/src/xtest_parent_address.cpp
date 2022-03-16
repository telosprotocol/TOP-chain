// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xelection/xtest_fixtures.h"
#include "xelection/xdata_accessor_error.h"

using top::common::xbroadcast_id_t;
using top::common::xnode_id_t;
using top::common::xnode_type_t;
using top::common::xsharding_address_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_result_store_t;
using top::tests::election::xcommittee_fixture_t;

TEST_F(xcommittee_fixture_t, committee_parent_address_empty_child_address) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(xsharding_address_t{},
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::address_empty, ec);
}

TEST_F(xcommittee_fixture_t, committee_parent_address_child_address_is_self) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(top::common::build_committee_sharding_address(m_election_cache_data_accessor->network_id()),
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::associated_group_not_exist, ec);
}

TEST_F(xcommittee_fixture_t, committee_parent_address_child_address_is_zec) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(top::common::build_zec_sharding_address(m_election_cache_data_accessor->network_id()),
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::zone_not_exist, ec);
}

#if 0
TEST_F(xcommittee_fixture_t, committee_parent_address_child_address_in_different_network) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(top::common::build_committee_sharding_address(top::common::xtestnet_id),
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::network_id_mismatch, ec);
}
#endif

TEST_F(xcommittee_fixture_t, committee_parent_address_child_address_in_different_cluster) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(xsharding_address_t{
                                                                m_election_cache_data_accessor->network_id(),
                                                                top::common::xcommittee_zone_id,
                                                                top::common::xdefault_cluster_id,
                                                                top::common::xcommittee_group_id
                                                            },
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::cluster_not_exist, ec);
}

TEST_F(xcommittee_fixture_t, committee_parent_address_child_address_in_different_group) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(xsharding_address_t{
                                                                m_election_cache_data_accessor->network_id(),
                                                                top::common::xcommittee_zone_id,
                                                                top::common::xcommittee_cluster_id,
                                                                top::common::xdefault_group_id
                                                            },
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::group_not_exist, ec);
}

TEST_F(xcommittee_fixture_t, committee_parent_address_child_address_zone_id_empty) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(xsharding_address_t{
                                                                m_election_cache_data_accessor->network_id()
                                                            },
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::zone_id_empty, ec);
}

TEST_F(xcommittee_fixture_t, committee_parent_address_child_address_cluster_id_empty) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(xsharding_address_t{
                                                                m_election_cache_data_accessor->network_id(),
                                                                top::common::xcommittee_zone_id,
                                                            },
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::cluster_id_empty, ec);
}

TEST_F(xcommittee_fixture_t, committee_parent_address_child_address_group_id_empty) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(xsharding_address_t{
                                                                m_election_cache_data_accessor->network_id(),
                                                                top::common::xcommittee_zone_id,
                                                                top::common::xcommittee_cluster_id
                                                            },
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::group_id_empty, ec);
}

TEST_F(xcommittee_fixture_t, committee_parent_address_nonempty_version) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(top::common:: build_committee_sharding_address(m_election_cache_data_accessor->network_id()),
                                                            top::common::xelection_round_t{ 0 },
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::associated_group_not_exist, ec);
}

TEST_F(xcommittee_fixture_t, committee_parent_address_empty_version) {
    std::error_code ec;
    auto r = m_election_cache_data_accessor->parent_address(top::common:: build_committee_sharding_address(m_election_cache_data_accessor->network_id()),
                                                            top::common::xelection_round_t{},
                                                            ec);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(top::election::xdata_accessor_errc_t::associated_group_not_exist, ec);
}
