// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xelection/xdummy_chain_timer.h"
#include "tests/xelection/xtest_fixtures.h"
#include "xbasic/xutility.h"
#include "xcommon/xaddress.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xelection/xcache/xdata_accessor.h"

#include <gtest/gtest.h>

using top::common::xbroadcast_id_t;
using top::common::xnode_id_t;
using top::common::xnode_type_t;
using top::common::xslot_id_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xstandby_node_info_t;

NS_BEG3(top, tests, election)

TEST(xtest_committee_sharding_nodes, _) {
    top::common::xnetwork_id_t network_id{ top::common::xtopchain_network_id };
    top::common::xzone_id_t zone_id{ top::common::xcommittee_zone_id };
    top::common::xcluster_id_t cluster_id{ top::common::xcommittee_cluster_id };
    top::common::xgroup_id_t group_id{ top::common::xcommittee_group_id };

    top::election::cache::xdata_accessor_t data_accessor{ network_id, top::make_observer(top::tests::election::xdummy_chain_timer) };

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id)
                                               .result_of(xnode_type_t::committee)
                                               .result_of(cluster_id)
                                               .result_of(group_id);
    group_result.group_version(top::common::xelection_round_t{ 0 });
    group_result.election_committee_version(top::common::xelection_round_t{ 0 });
    group_result.start_time(0);
    // group_result.associated_election_blk_height(0);

    std::size_t const node_count{ 1023 };
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::rec, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{ "fake public key" };

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(build_account_address(i));
        election_info_bundle.election_info(std::move(new_election_info));

        auto r = group_result.insert(std::move(election_info_bundle));
        ASSERT_TRUE(top::get<bool>(r));
    }
    assert(group_result.size() == node_count);

    std::error_code ec;
    auto const & update_result = data_accessor.update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result.size());

    top::common::xsharding_address_t committee_sharding_address{
        network_id,
        zone_id,
        cluster_id,
        group_id
    };
    auto sharding_nodes = data_accessor.sharding_nodes(committee_sharding_address, top::common::xelection_round_t{ 0 }, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(node_count, sharding_nodes.size());
    for (auto i = 0u; i < node_count; ++i) {
        auto const it = std::next(sharding_nodes.begin(), i);

        ASSERT_TRUE(xslot_id_t{ i } == top::get<xslot_id_t const>(*it));
        ASSERT_TRUE(xslot_id_t{ i } == top::get<top::data::xnode_info_t>(*it).address.slot_id());
        ASSERT_TRUE(build_account_address(i) == top::get<top::data::xnode_info_t>(*it).address.node_id());
        ASSERT_TRUE(committee_sharding_address == top::get<top::data::xnode_info_t>(*it).address.sharding_address());
    }
}

TEST(xtest_zec_sharding_nodes, _) {
    top::common::xnetwork_id_t network_id{ top::common::xtopchain_network_id };
    top::common::xzone_id_t zone_id{ top::common::xzec_zone_id };
    top::common::xcluster_id_t cluster_id{ top::common::xcommittee_cluster_id };
    top::common::xgroup_id_t group_id{ top::common::xcommittee_group_id };

    top::election::cache::xdata_accessor_t data_accessor{ network_id, top::make_observer(top::tests::election::xdummy_chain_timer) };

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id)
                                               .result_of(xnode_type_t::zec)
                                               .result_of(cluster_id)
                                               .result_of(group_id);
    group_result.group_version(top::common::xelection_round_t{ 0 });
    group_result.election_committee_version(top::common::xelection_round_t{ 0 });
    group_result.start_time(0);
    // group_result.associated_election_blk_height(0);

    std::size_t const node_count{ 1023 };
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::zec, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{ "fake public key" };

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(build_account_address(i));
        election_info_bundle.election_info(std::move(new_election_info));

        group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = data_accessor.update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result.size());

    top::common::xsharding_address_t zec_sharding_address{
        network_id,
        zone_id,
        cluster_id,
        group_id
    };
    auto sharding_nodes = data_accessor.sharding_nodes(zec_sharding_address, top::common::xelection_round_t{ 0 }, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(node_count, sharding_nodes.size());
    for (auto i = 0u; i < node_count; ++i) {
        auto const it = std::next(sharding_nodes.begin(), i);

        ASSERT_TRUE(xslot_id_t{ i } == top::get<xslot_id_t const>(*it));
        ASSERT_TRUE(xslot_id_t{ i } == top::get<top::data::xnode_info_t>(*it).address.slot_id());
        ASSERT_TRUE(build_account_address(i) == top::get<top::data::xnode_info_t>(*it).address.node_id());
        ASSERT_TRUE(zec_sharding_address == top::get<top::data::xnode_info_t>(*it).address.sharding_address());
    }
}

TEST(xtest_edge_sharding_nodes, _) {
    top::common::xnetwork_id_t network_id{ top::common::xtopchain_network_id };
    top::common::xzone_id_t zone_id{ top::common::xedge_zone_id };
    top::common::xcluster_id_t cluster_id{ top::common::xdefault_cluster_id };
    top::common::xgroup_id_t group_id{ top::common::xdefault_group_id };

    top::election::cache::xdata_accessor_t data_accessor{ network_id, top::make_observer(top::tests::election::xdummy_chain_timer) };

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id)
                                               .result_of(xnode_type_t::edge)
                                               .result_of(cluster_id)
                                               .result_of(group_id);
    group_result.group_version(top::common::xelection_round_t{ 0 });
    group_result.election_committee_version(top::common::xelection_round_t{ 0 });
    group_result.start_time(0);
    // group_result.associated_election_blk_height(0);

    std::size_t const node_count{ 1023 };
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::edge, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{ "fake public key" };

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(build_account_address(i));
        election_info_bundle.election_info(std::move(new_election_info));

        group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = data_accessor.update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result.size());

    top::common::xsharding_address_t edge_sharding_address{
        network_id,
        zone_id,
        cluster_id,
        group_id
    };
    auto sharding_nodes = data_accessor.sharding_nodes(edge_sharding_address, top::common::xelection_round_t{ 0 }, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(node_count, sharding_nodes.size());
    for (auto i = 0u; i < node_count; ++i) {
        auto const it = std::next(sharding_nodes.begin(), i);

        ASSERT_TRUE(xslot_id_t{i} == top::get<xslot_id_t const>(*it));
        ASSERT_TRUE(xslot_id_t{i} == top::get<top::data::xnode_info_t>(*it).address.slot_id());
        ASSERT_TRUE(build_account_address(i) == top::get<top::data::xnode_info_t>(*it).address.node_id());
        ASSERT_TRUE(edge_sharding_address == top::get<top::data::xnode_info_t>(*it).address.sharding_address());
    }
}

TEST(xtest_archive_sharding_nodes, _) {
    top::common::xnetwork_id_t network_id{ top::common::xtopchain_network_id };
    top::common::xzone_id_t zone_id{ top::common::xstorage_zone_id };
    top::common::xcluster_id_t cluster_id{ top::common::xdefault_cluster_id };
    top::common::xgroup_id_t group_id{ top::common::xdefault_group_id };

    top::election::cache::xdata_accessor_t data_accessor{ network_id, top::make_observer(top::tests::election::xdummy_chain_timer) };

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id)
                                               .result_of(xnode_type_t::storage_archive)
                                               .result_of(cluster_id)
                                               .result_of(group_id);
    group_result.group_version(top::common::xelection_round_t{ 0 });
    group_result.election_committee_version(top::common::xelection_round_t{ 0 });
    group_result.start_time(0);
    // group_result.associated_election_blk_height(0);

    std::size_t const node_count{ 1023 };
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::storage_archive, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{ "fake public key" };

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(build_account_address(i));
        election_info_bundle.election_info(std::move(new_election_info));

        group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = data_accessor.update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result.size());

    top::common::xsharding_address_t archive_sharding_address{
        network_id,
        zone_id,
        cluster_id,
        group_id
    };
    auto sharding_nodes = data_accessor.sharding_nodes(archive_sharding_address, top::common::xelection_round_t{ 0 }, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(node_count, sharding_nodes.size());
    for (auto i = 0u; i < node_count; ++i) {
        auto const it = std::next(sharding_nodes.begin(), i);

        ASSERT_TRUE(xslot_id_t{i} == top::get<xslot_id_t const>(*it));
        ASSERT_TRUE(xslot_id_t{i} == top::get<top::data::xnode_info_t>(*it).address.slot_id());
        ASSERT_TRUE(build_account_address(i) == top::get<top::data::xnode_info_t>(*it).address.node_id());
        ASSERT_TRUE(archive_sharding_address == top::get<top::data::xnode_info_t>(*it).address.sharding_address());
    }
}

TEST(xtest_consensus_sharding_nodes, _) {
    top::common::xnetwork_id_t network_id{ top::common::xtopchain_network_id };
    top::common::xzone_id_t zone_id{ top::common::xconsensus_zone_id };
    top::common::xcluster_id_t cluster_id{ top::common::xdefault_cluster_id };
    top::common::xgroup_id_t auditor_group_id{ top::common::xauditor_group_id_begin };
    top::common::xgroup_id_t validator_group_id{ top::common::xvalidator_group_id_begin };

    top::election::cache::xdata_accessor_t data_accessor{ network_id, top::make_observer(top::tests::election::xdummy_chain_timer) };

    xelection_result_store_t election_result_store;
    auto & auditor_group_result = election_result_store.result_of(network_id)
                                                       .result_of(xnode_type_t::consensus_auditor)
                                                       .result_of(cluster_id)
                                                       .result_of(auditor_group_id);
    auditor_group_result.group_version(top::common::xelection_round_t{ 0 });
    auditor_group_result.election_committee_version(top::common::xelection_round_t{ 0 });
    auditor_group_result.start_time(0);
    // auditor_group_result.associated_election_blk_height(0);

    auto & validator_group_result = election_result_store.result_of(network_id)
                                                         .result_of(xnode_type_t::consensus_validator)
                                                         .result_of(cluster_id)
                                                         .result_of(validator_group_id);
    validator_group_result.group_version(top::common::xelection_round_t{ 0 });
    validator_group_result.election_committee_version(top::common::xelection_round_t{ 0 });
    validator_group_result.start_time(0);
    // validator_group_result.associated_election_blk_height(0);

    std::size_t const node_count{ 1023 };
    for (auto i = 0u; i < node_count; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(xnode_type_t::consensus_auditor, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{ "fake public key" };

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(build_account_address(i));
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
        standby_node_info.consensus_public_key = top::xpublic_key_t{ "fake public key" };

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{0});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(build_account_address(i + node_count));
        election_info_bundle.election_info(std::move(new_election_info));

        validator_group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = data_accessor.update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(2, update_result.size());

    top::common::xsharding_address_t auditor_sharding_address{
        network_id,
        zone_id,
        cluster_id,
        auditor_group_id
    };
    auto auditor_sharding_nodes = data_accessor.sharding_nodes(auditor_sharding_address, top::common::xelection_round_t{ 0 }, ec);
    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(node_count, auditor_sharding_nodes.size());

    top::common::xsharding_address_t validator_sharding_address{
        network_id,
        zone_id,
        cluster_id,
        validator_group_id
    };
    auto validator_sharding_nodes = data_accessor.sharding_nodes(validator_sharding_address, top::common::xelection_round_t{ 0 }, ec);
    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(node_count, validator_sharding_nodes.size());

    for (auto const & result : update_result) {
        auto const & sharding_address = top::get<top::common::xsharding_address_t const>(result);
        ASSERT_TRUE(sharding_address == auditor_sharding_address || sharding_address == validator_sharding_address);

        if (sharding_address == auditor_sharding_address) {
            for (auto i = 0u; i < node_count; ++i) {
                auto const it = std::next(auditor_sharding_nodes.begin(), i);

                ASSERT_TRUE(xslot_id_t{i} == top::get<xslot_id_t const>(*it));
                ASSERT_TRUE(xslot_id_t{i} == top::get<top::data::xnode_info_t>(*it).address.slot_id());
                ASSERT_TRUE(build_account_address(i) == top::get<top::data::xnode_info_t>(*it).address.node_id());
                ASSERT_TRUE(auditor_sharding_address == top::get<top::data::xnode_info_t>(*it).address.sharding_address());
            }
        } else {
            for (auto i = 0u; i < node_count; ++i) {
                auto const it = std::next(validator_sharding_nodes.begin(), i);

                ASSERT_TRUE(xslot_id_t{i} == top::get<xslot_id_t const>(*it));
                ASSERT_TRUE(xslot_id_t{i} == top::get<top::data::xnode_info_t>(*it).address.slot_id());
                ASSERT_TRUE(build_account_address(i + node_count) == top::get<top::data::xnode_info_t>(*it).address.node_id());
                ASSERT_TRUE(validator_sharding_address == top::get<top::data::xnode_info_t>(*it).address.sharding_address());
            }
        }
    }
}

NS_END3
