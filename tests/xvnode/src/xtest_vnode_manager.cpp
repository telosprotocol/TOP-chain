// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xvnode/xvnode_manager_fixture.h"

#include "xelection/xcache/xcluster_element.h"
#include "xelection/xcache/xzone_element.h"

#include <cassert>
#include <map>
#include <memory>

NS_BEG3(top, tests, vnode)

std::pair<common::xsharding_address_t, election::cache::xgroup_update_result_t> generate_election_data(std::shared_ptr<election::cache::xnetwork_element_t> & network_element,
                                                                                                       common::xsharding_address_t const & sharding_address,
                                                                                                       common::xelection_round_t const & group_version,
                                                                                                       common::xlogic_time_t const election_timestamp,
                                                                                                       common::xlogic_time_t const start_time,
                                                                                                       std::uint16_t const group_size,
                                                                                                       std::uint64_t const associated_election_blk_height) {
    if (network_element == nullptr) {
        network_element = std::make_shared<election::cache::xnetwork_element_t>(sharding_address.network_id());
    }

    std::error_code ec;
    auto zone_element = network_element->add_zone_element(sharding_address.zone_id(), ec);
    ec.clear();

    auto cluster_element = zone_element->add_cluster_element(sharding_address.cluster_id(), ec);
    ec.clear();

    auto update_result = cluster_element->add_group_element(sharding_address.group_id(), group_version, election_timestamp, start_time, group_size, associated_election_blk_height, ec);
    assert(update_result.added != nullptr);

    std::map<common::xslot_id_t, data::election::xelection_info_bundle_t> nodes;
    common::xnode_id_t node_id;
    for (auto i = 0; i < group_size; ++i) {
        node_id.random();

        data::election::xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.node_id(node_id);
        election_info_bundle.election_info(data::election::xelection_info_t{});

        nodes.insert({common::xslot_id_t{i}, std::move(election_info_bundle)});
    }

    update_result.added->set_node_elements(nodes);

    return {sharding_address, std::move(update_result)};
}

std::pair<common::xsharding_address_t, election::cache::xgroup_update_result_t> generate_election_data(std::shared_ptr<election::cache::xnetwork_element_t> & network_element,
                                                                                                       common::xsharding_address_t const & sharding_address,
                                                                                                       common::xelection_round_t const & group_version,
                                                                                                       common::xlogic_time_t const election_timestamp,
                                                                                                       common::xlogic_time_t const start_time,
                                                                                                       std::uint16_t const group_size,
                                                                                                       std::uint64_t const associated_election_blk_height,
                                                                                                       common::xnode_id_t const & specified_node_id) {
    if (network_element == nullptr) {
        network_element = std::make_shared<election::cache::xnetwork_element_t>(sharding_address.network_id());
    }

    std::error_code ec;
    auto zone_element = network_element->add_zone_element(sharding_address.zone_id(), ec);
    ec.clear();

    auto cluster_element = zone_element->add_cluster_element(sharding_address.cluster_id(), ec);
    ec.clear();

    auto update_result = cluster_element->add_group_element(sharding_address.group_id(), group_version, election_timestamp, start_time, group_size, associated_election_blk_height, ec);
    assert(update_result.added != nullptr);

    std::map<common::xslot_id_t, data::election::xelection_info_bundle_t> nodes;
    common::xnode_id_t node_id;
    for (auto i = 0; i < group_size - 1; ++i) {
        node_id.random();

        data::election::xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.node_id(node_id);
        election_info_bundle.election_info(data::election::xelection_info_t{});

        nodes.insert({common::xslot_id_t{i}, std::move(election_info_bundle)});
    }

    data::election::xelection_info_bundle_t election_info_bundle{};
    election_info_bundle.node_id(specified_node_id);

    nodes.insert({common::xslot_id_t{group_size - 1}, std::move(election_info_bundle)});

    update_result.added->set_node_elements(nodes);

    return {sharding_address, std::move(update_result)};
}

TEST_F(xvnode_manager_fixture, not_elected_in) {
    auto datum =
        generate_election_data(network_, top::common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{1}, common::xlogic_time_t{1}, common::xlogic_time_t{1}, 10, 1);

    auto outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());
    ASSERT_TRUE(m_all_nodes.empty());
}

TEST_F(xvnode_manager_fixture, elected_in) {
    auto datum = generate_election_data(network_, top::common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{1}, common::xlogic_time_t{1}, common::xlogic_time_t{1}, 10, 1, vhost_->host_node_id());

    auto outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());

    ASSERT_TRUE(m_all_nodes.size() == 1);
}

TEST_F(xvnode_manager_fixture, elected_in_two_times) {
    auto datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{1}, common::xlogic_time_t{1}, common::xlogic_time_t{1}, 10, 1, vhost_->host_node_id());

    auto outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());

    ASSERT_TRUE(m_all_nodes.size() == 1);

    datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{2}, common::xlogic_time_t{2}, common::xlogic_time_t{2}, 10, 2, vhost_->host_node_id());

    outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());
    ASSERT_TRUE(m_all_nodes.size() == 2);
}

TEST_F(xvnode_manager_fixture, elected_in_thress_times) {
    auto datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{1}, common::xlogic_time_t{1}, common::xlogic_time_t{1}, 10, 1, vhost_->host_node_id());

    auto outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());

    ASSERT_TRUE(m_all_nodes.size() == 1);

    datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{2}, common::xlogic_time_t{2}, common::xlogic_time_t{2}, 10, 2, vhost_->host_node_id());

    outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());
    ASSERT_TRUE(m_all_nodes.size() == 2);

    datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{3}, common::xlogic_time_t{3}, common::xlogic_time_t{3}, 10, 3, vhost_->host_node_id());

    outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());
    ASSERT_TRUE(m_all_nodes.size() == 3);
}

TEST_F(xvnode_manager_fixture, elected_in_rotation_status_faded) {
    auto datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{1}, common::xlogic_time_t{1}, common::xlogic_time_t{1}, 10, 1, vhost_->host_node_id());

    auto outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());

    ASSERT_TRUE(m_all_nodes.size() == 1);

    datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{2}, common::xlogic_time_t{2}, common::xlogic_time_t{2}, 10, 2);

    outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());
    ASSERT_TRUE(m_all_nodes.size() == 1);
    ASSERT_TRUE(std::begin(m_all_nodes)->second->rotation_status(common::xlogic_time_t{2}) == common::xrotation_status_t::faded);
}

TEST_F(xvnode_manager_fixture, elected_in_rotation_status_outdated) {
    auto datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{1}, common::xlogic_time_t{1}, common::xlogic_time_t{1}, 10, 1, vhost_->host_node_id());

    auto outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());

    ASSERT_TRUE(m_all_nodes.size() == 1);

    datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{2}, common::xlogic_time_t{2}, common::xlogic_time_t{2}, 10, 2);

    outdated_xip = handle_election_data({datum});
    ASSERT_TRUE(outdated_xip.second.empty());
    ASSERT_TRUE(m_all_nodes.size() == 1);
    ASSERT_TRUE(std::begin(m_all_nodes)->second->rotation_status(common::xlogic_time_t{2}) == common::xrotation_status_t::faded);

    datum = generate_election_data(
        network_, common::build_committee_sharding_address(top::common::xbeacon_network_id), common::xelection_round_t{3}, common::xlogic_time_t{3}, common::xlogic_time_t{3}, 10, 3);

    outdated_xip = handle_election_data({datum});
    ASSERT_FALSE(outdated_xip.second.empty());
    ASSERT_TRUE(m_all_nodes.size() == 1);
    ASSERT_TRUE(std::begin(m_all_nodes)->second->rotation_status(common::xlogic_time_t{3}) == common::xrotation_status_t::outdated);
}

NS_END3
