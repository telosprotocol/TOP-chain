// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xelection/xtest_fixtures.h"

#include "tests/xelection/xdummy_chain_timer.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xelection/xcache/xdata_accessor.h"

using top::common::xbroadcast_id_t;
using top::common::xnode_id_t;
using top::common::xnode_type_t;
using top::common::xslot_id_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xstandby_node_info_t;

NS_BEG3(top, tests, election)

common::xaccount_address_t build_account_address(size_t const i) {
    std::string account_address_prefix = "T00000LMZLAYynftsjQiKZ5W7TQncDL";

    std::string account_string = account_address_prefix + std::to_string(i);
    if (account_string.size() < top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH) {
        account_string.append(top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH - account_string.size(), 'x');
    }
    assert(account_string.size() == top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH);

    return common::xaccount_address_t{account_string};
}

void xtop_committee_fixure::SetUp() {
    auto const & committee_sharding_address = top::common::build_committee_sharding_address(top::common::xtestnet_id);
    top::common::xnetwork_id_t const & network_id = committee_sharding_address.network_id();
    top::common::xzone_id_t const & zone_id = committee_sharding_address.zone_id();
    top::common::xcluster_id_t const & cluster_id = committee_sharding_address.cluster_id();
    top::common::xgroup_id_t const & group_id = committee_sharding_address.group_id();

    m_election_cache_data_accessor = top::make_unique<top::election::cache::xdata_accessor_t>(network_id, top::make_observer(top::tests::election::xdummy_chain_timer));
    auto const zone_type = top::common::node_type_from(zone_id);
    ASSERT_TRUE(xnode_type_t::committee == zone_type);

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id).result_of(zone_type).result_of(cluster_id).result_of(group_id);
    group_result.group_version(common::xelection_round_t{0});
    group_result.election_committee_version(common::xelection_round_t{0});
    group_result.start_time(0);
    // group_result.associated_election_blk_height(0);

    std::size_t const node_count{1023};
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(zone_type, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;

        standby_node_info.consensus_public_key = top::xpublic_key_t{"fake public key"};

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;

        election_info_bundle.account_address(build_account_address(i));
        election_info_bundle.election_info(std::move(new_election_info));

        group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = m_election_cache_data_accessor->update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result.size());
}

void xtop_committee_fixure::TearDown() {
    m_election_cache_data_accessor.reset();
}

void xtop_zec_fixture::SetUp() {
    top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
    top::common::xzone_id_t zone_id{top::common::xzec_zone_id};
    top::common::xcluster_id_t cluster_id{top::common::xcommittee_cluster_id};
    top::common::xgroup_id_t group_id{top::common::xcommittee_group_id};

    m_election_cache_data_accessor = top::make_unique<top::election::cache::xdata_accessor_t>(network_id, top::make_observer(top::tests::election::xdummy_chain_timer));

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id).result_of(xnode_type_t::zec).result_of(cluster_id).result_of(group_id);
    group_result.group_version(common::xelection_round_t{0});
    group_result.election_committee_version(common::xelection_round_t{0});
    group_result.start_time(0);
    // group_result.associated_election_blk_height(0);

    std::size_t const node_count{1023};
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::zec, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{"fake public key"};

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(xnode_id_t{std::to_string(i)});
        election_info_bundle.election_info(std::move(new_election_info));

        group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = m_election_cache_data_accessor->update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result.size());
}

void xtop_zec_fixture::TearDown() {
    m_election_cache_data_accessor.reset();
}

void xtop_edge_fixture::SetUp() {
    top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
    top::common::xzone_id_t zone_id{top::common::xedge_zone_id};
    top::common::xcluster_id_t cluster_id{top::common::xdefault_cluster_id};
    top::common::xgroup_id_t group_id{top::common::xdefault_group_id};

    m_election_cache_data_accessor = top::make_unique<top::election::cache::xdata_accessor_t>(network_id, top::make_observer(top::tests::election::xdummy_chain_timer));

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id).result_of(xnode_type_t::edge).result_of(cluster_id).result_of(group_id);
    group_result.group_version(common::xelection_round_t{0});
    group_result.election_committee_version(common::xelection_round_t{0});
    group_result.start_time(0);
    // group_result.associated_election_blk_height(0);

    std::size_t const node_count{1023};
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::edge, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{"fake public key"};

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(xnode_id_t{std::to_string(i)});
        election_info_bundle.election_info(std::move(new_election_info));

        group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = m_election_cache_data_accessor->update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result.size());
}

void xtop_edge_fixture::TearDown() {
    m_election_cache_data_accessor.reset();
}

void xtop_archive_fixture::SetUp() {
    top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
    top::common::xzone_id_t zone_id{top::common::xstorage_zone_id};
    top::common::xcluster_id_t cluster_id{top::common::xdefault_cluster_id};
    top::common::xgroup_id_t group_id{top::common::xdefault_group_id};

    m_election_cache_data_accessor = top::make_unique<top::election::cache::xdata_accessor_t>(network_id, top::make_observer(top::tests::election::xdummy_chain_timer));

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id).result_of(xnode_type_t::storage_archive).result_of(cluster_id).result_of(group_id);
    group_result.group_version(common::xelection_round_t{0});
    group_result.election_committee_version(common::xelection_round_t{0});
    group_result.start_time(0);
    // group_result.associated_election_blk_height(0);

    std::size_t const node_count{1023};
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::storage_archive, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{"fake public key"};

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(xnode_id_t{std::to_string(i)});
        election_info_bundle.election_info(std::move(new_election_info));

        group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = m_election_cache_data_accessor->update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result.size());
}

void xtop_archive_fixture::TearDown() {
    m_election_cache_data_accessor.reset();
}

void xtop_consensus_fixture::SetUp() {
    top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
    top::common::xzone_id_t zone_id{top::common::xconsensus_zone_id};
    top::common::xcluster_id_t cluster_id{top::common::xdefault_cluster_id};
    top::common::xgroup_id_t auditor_group_id{top::common::xauditor_group_id_begin};
    top::common::xgroup_id_t validator_group_id{top::common::xvalidator_group_id_begin};

    m_election_cache_data_accessor = top::make_unique<top::election::cache::xdata_accessor_t>(network_id, top::make_observer(top::tests::election::xdummy_chain_timer));

    xelection_result_store_t election_result_store;
    auto & auditor_group_result = election_result_store.result_of(network_id).result_of(xnode_type_t::consensus_auditor).result_of(cluster_id).result_of(auditor_group_id);
    auditor_group_result.group_version(common::xelection_round_t{0});
    auditor_group_result.election_committee_version(common::xelection_round_t{0});
    auditor_group_result.start_time(0);
    // auditor_group_result.associated_election_blk_height(0);

    auto & validator_group_result = election_result_store.result_of(network_id).result_of(xnode_type_t::consensus_validator).result_of(cluster_id).result_of(validator_group_id);
    validator_group_result.group_version(common::xelection_round_t{0});
    validator_group_result.election_committee_version(common::xelection_round_t{0});
    validator_group_result.start_time(0);
    // validator_group_result.associated_election_blk_height(0);

    std::size_t const node_count{1023};
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::consensus_auditor, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{"fake public key"};

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(xnode_id_t{std::to_string(i)});
        election_info_bundle.election_info(std::move(new_election_info));

        auditor_group_result.insert(std::move(election_info_bundle));
    }
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::consensus_validator, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::validator;
        standby_node_info.consensus_public_key = top::xpublic_key_t{"fake public key"};

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(xnode_id_t{std::to_string(i + node_count)});
        election_info_bundle.election_info(std::move(new_election_info));

        validator_group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = m_election_cache_data_accessor->update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(2, update_result.size());
}

void xtop_consensus_fixture::TearDown() {
    m_election_cache_data_accessor.reset();
}

NS_END3
