// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xelection/xdummy_chain_timer.h"
#include "tests/xelection/xtest_fixtures.h"
#include "xbasic/xutility.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xelection/xcache/xdata_accessor.h"

#include <gtest/gtest.h>

using top::common::xbroadcast_id_t;
using top::common::xnode_id_t;
using top::common::xnode_type_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xstandby_node_info_t;

NS_BEG3(top, tests, election)

TEST(xtest_update_committee_zone, _) {
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

    for (auto i = 0u; i < 1023u; ++i) {
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

        group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = data_accessor.update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result.size());

    auto const & sharding_address = top::get<top::common::xsharding_address_t const>(*update_result.begin());
    // auto const & group_update_result = top::get<top::election::cache::xgroup_update_result_t>(*update_result.begin());

    ASSERT_TRUE(network_id == sharding_address.network_id());
    ASSERT_TRUE(zone_id == sharding_address.zone_id());
    ASSERT_TRUE(cluster_id == sharding_address.cluster_id());
    ASSERT_TRUE(group_id == sharding_address.group_id());

    auto const & xip = sharding_address.xip();
    ASSERT_TRUE(network_id == xip.network_id());
    ASSERT_TRUE(zone_id == xip.zone_id());
    ASSERT_TRUE(cluster_id == xip.cluster_id());
    ASSERT_TRUE(group_id == xip.group_id());
    ASSERT_TRUE(xbroadcast_id_t::slot == xip.slot_id());
    ASSERT_TRUE(top::common::xdefault_network_version == xip.network_version());
}

TEST(xtest_update_zec_zone, _) {
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

    for (auto i = 0u; i < 1023u; ++i) {
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

    auto const & sharding_address = top::get<top::common::xsharding_address_t const>(*update_result.begin());
    auto const & group_update_result = top::get<top::election::cache::xgroup_update_result_t>(*update_result.begin());

    ASSERT_TRUE(network_id == sharding_address.network_id());
    ASSERT_TRUE(zone_id == sharding_address.zone_id());
    ASSERT_TRUE(cluster_id == sharding_address.cluster_id());
    ASSERT_TRUE(group_id == sharding_address.group_id());

    auto const & xip = sharding_address.xip();
    ASSERT_TRUE(network_id == xip.network_id());
    ASSERT_TRUE(zone_id == xip.zone_id());
    ASSERT_TRUE(cluster_id == xip.cluster_id());
    ASSERT_TRUE(group_id == xip.group_id());
    ASSERT_TRUE(xbroadcast_id_t::slot == xip.slot_id());
    ASSERT_TRUE(top::common::xdefault_network_version == xip.network_version());
}

TEST(xtest_update_edge_zone, _) {
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

    for (auto i = 0u; i < 1023u; ++i) {
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

    auto const & sharding_address = top::get<top::common::xsharding_address_t const>(*update_result.begin());
    auto const & group_update_result = top::get<top::election::cache::xgroup_update_result_t>(*update_result.begin());

    ASSERT_TRUE(network_id == sharding_address.network_id());
    ASSERT_TRUE(zone_id == sharding_address.zone_id());
    ASSERT_TRUE(cluster_id == sharding_address.cluster_id());
    ASSERT_TRUE(group_id == sharding_address.group_id());

    auto const & xip = sharding_address.xip();
    ASSERT_TRUE(network_id == xip.network_id());
    ASSERT_TRUE(zone_id == xip.zone_id());
    ASSERT_TRUE(cluster_id == xip.cluster_id());
    ASSERT_TRUE(group_id == xip.group_id());
    ASSERT_TRUE(xbroadcast_id_t::slot == xip.slot_id());
    ASSERT_TRUE(top::common::xdefault_network_version == xip.network_version());
}

TEST(xtest_update_archive_zone, _) {
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

    for (auto i = 0u; i < 1023u; ++i) {
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

    auto const & sharding_address = top::get<top::common::xsharding_address_t const>(*update_result.begin());
    auto const & group_update_result = top::get<top::election::cache::xgroup_update_result_t>(*update_result.begin());

    ASSERT_TRUE(network_id == sharding_address.network_id());
    ASSERT_TRUE(zone_id == sharding_address.zone_id());
    ASSERT_TRUE(cluster_id == sharding_address.cluster_id());
    ASSERT_TRUE(group_id == sharding_address.group_id());

    auto const & xip = sharding_address.xip();
    ASSERT_TRUE(network_id == xip.network_id());
    ASSERT_TRUE(zone_id == xip.zone_id());
    ASSERT_TRUE(cluster_id == xip.cluster_id());
    ASSERT_TRUE(group_id == xip.group_id());
    ASSERT_TRUE(xbroadcast_id_t::slot == xip.slot_id());
    ASSERT_TRUE(top::common::xdefault_network_version == xip.network_version());
}

TEST(xtest_update_consensus_zone, _) {
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

    for (auto i = 0u; i < 1023u; ++i) {
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

    for (auto i = 0u; i < 1023u; ++i) {
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
        election_info_bundle.account_address(build_account_address(i + 1023));
        election_info_bundle.election_info(std::move(new_election_info));

        validator_group_result.insert(std::move(election_info_bundle));
    }

    std::error_code ec;
    auto const & update_result = data_accessor.update_zone(zone_id, election_result_store, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(2, update_result.size());

    for (auto const & result : update_result) {
        auto const & sharding_address = top::get<top::common::xsharding_address_t const>(result);
        auto const & group_update_result = top::get<top::election::cache::xgroup_update_result_t>(result);

        ASSERT_TRUE(network_id == sharding_address.network_id());
        ASSERT_TRUE(zone_id == sharding_address.zone_id());
        ASSERT_TRUE(cluster_id == sharding_address.cluster_id());
        ASSERT_TRUE(sharding_address.group_id() == auditor_group_id || sharding_address.group_id() == validator_group_id);

        auto const & xip = sharding_address.xip();
        ASSERT_TRUE(network_id == xip.network_id());
        ASSERT_TRUE(zone_id == xip.zone_id());
        ASSERT_TRUE(cluster_id == xip.cluster_id());
        ASSERT_TRUE(xip.group_id() == auditor_group_id || xip.group_id() == validator_group_id);
        ASSERT_TRUE(xbroadcast_id_t::slot == xip.slot_id());
        ASSERT_TRUE(top::common::xdefault_network_version == xip.network_version());
    }
}

TEST(xtest_update_committee_zone, update_twice) {
    auto const & committee_sharding_address = top::common::build_committee_sharding_address(top::common::xbeacon_network_id);
    auto const network_id = committee_sharding_address.network_id();
    auto const zone_id = committee_sharding_address.zone_id();
    auto const cluster_id = committee_sharding_address.cluster_id();
    auto const group_id = committee_sharding_address.group_id();

    auto const zone_type = top::common::node_type_from(zone_id);

    top::election::cache::xdata_accessor_t data_accessor{ network_id, top::make_observer(top::tests::election::xdummy_chain_timer) };

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id)
                                               .result_of(zone_type)
                                               .result_of(cluster_id)
                                               .result_of(group_id);
    group_result.group_version(top::common::xelection_round_t{ 0 });
    group_result.election_committee_version(top::common::xelection_round_t{ 0 });
    group_result.start_time(0);
    // group_result.associated_election_blk_height(0);

    for (auto i = 0u; i < 1023u; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 0;
#if defined XENABLE_TESTS
        standby_node_info.stake(zone_type, i);
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

    xelection_result_store_t election_result_store2;
    auto & group_result2 = election_result_store2.result_of(network_id)
                                                 .result_of(zone_type)
                                                 .result_of(cluster_id)
                                                 .result_of(group_id);

    group_result2.group_version(top::common::xelection_round_t{ 1 });
    group_result2.election_committee_version(top::common::xelection_round_t{ 1 });
    group_result2.start_time(2);
    // group_result2.associated_election_blk_height(1);

    for (auto i = 0u; i < 1023u; ++i) {
        xstandby_node_info_t standby_node_info{};
        // standby_node_info.joined_time = 1;
#if defined XENABLE_TESTS
        standby_node_info.stake(zone_type, i);
#endif
        standby_node_info.miner_type = top::common::xminer_type_t::advance;
        standby_node_info.consensus_public_key = top::xpublic_key_t{ "fake public key" };

        xelection_info_t new_election_info{};
        // new_election_info.standby_info = std::move(standby_node_info);
        new_election_info.joined_epoch(common::xelection_round_t{1});

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(build_account_address(1023 - i - 1));
        election_info_bundle.election_info(std::move(new_election_info));

        group_result2.insert(std::move(election_info_bundle));
    }

    auto const & update_result2 = data_accessor.update_zone(zone_id, election_result_store2, 0, ec);

    ASSERT_EQ(0, ec.value());
    ASSERT_EQ(1, update_result2.size());

    auto const & update_data = *update_result2.begin();
    auto const & sharding_address = top::get<top::common::xsharding_address_t const>(update_data);
    auto const & group_update_result = top::get<top::election::cache::xgroup_update_result_t>(update_data);

    ASSERT_TRUE(network_id == sharding_address.network_id());
    ASSERT_TRUE(zone_id == sharding_address.zone_id());
    ASSERT_TRUE(cluster_id == sharding_address.cluster_id());
    ASSERT_TRUE(group_id == sharding_address.group_id());

    auto const & xip = sharding_address.xip();
    ASSERT_TRUE(network_id == xip.network_id());
    ASSERT_TRUE(zone_id == xip.zone_id());
    ASSERT_TRUE(cluster_id == xip.cluster_id());
    ASSERT_TRUE(group_id == xip.group_id());
    ASSERT_TRUE(xbroadcast_id_t::slot == xip.slot_id());
    ASSERT_TRUE(top::common::xdefault_network_version == xip.network_version());

    auto const & updated_group_info = top::get<top::election::cache::xgroup_update_result_t>(update_data);
    auto const & outdated = updated_group_info.outdated;
    auto const & faded = updated_group_info.faded;
    auto const & added = updated_group_info.added;

    ASSERT_EQ(nullptr, outdated.get());
    ASSERT_NE(nullptr, faded.get());
    ASSERT_NE(nullptr, added.get());
}

NS_END3
