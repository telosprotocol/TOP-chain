// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xsystem_contract/xelection_algorithm/xtest_election_data_manager_fixture.h"

#include "xdata/xelection/xelection_info.h"

NS_BEG3(top, tests, election)

common::xaccount_address_t build_account_address(std::string const & account_prefix, size_t const index) {
    auto account_string = account_prefix + std::to_string(index);
    if (account_string.length() < top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH) {
        account_string.append(top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH - account_string.length(), 'x');
    }
    assert(account_string.length() == top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH);
    return common::xaccount_address_t{account_string};
}

bool xtop_test_election_data_manager_fixture::add_standby_node(common::xnode_type_t node_type, common::xnode_id_t node_id, xstandby_node_info_t standby_node_info) {
    return standby_network_result.result_of(node_type).insert({node_id, standby_node_info}).second;
}

bool xtop_test_election_data_manager_fixture::delete_standby_node(common::xnode_type_t node_type, common::xnode_id_t node_id) {
    auto & standby_result = standby_network_result.result_of(node_type);
    standby_result.erase(node_id);
    return standby_result.find(node_id) == standby_result.end();
}

bool xtop_test_election_data_manager_fixture::add_nodes_to_standby(std::size_t node_count, common::xnode_type_t node_type, std::string node_id_prefix) {
    std::size_t begin_index{1};
    begin_index += standby_network_result.result_of(node_type).size();
    for (std::size_t index = begin_index; index < node_count + begin_index; ++index) {
        common::xnode_id_t node_id = build_account_address(node_id_prefix, index);
        xstandby_node_info_t standby_node_info;
        standby_node_info.consensus_public_key = top::xpublic_key_t{std::string{"test_publick_key_"} + std::to_string(index)};
#if defined XENABLE_TESTS
        standby_node_info.stake(node_type, index);
#endif
        standby_node_info.miner_type = common::has<common::xnode_type_t::consensus_validator>(node_type) ? common::xminer_type_t::validator : common::xminer_type_t::advance;
        if (!add_standby_node(node_type, node_id, standby_node_info))
            return false;
    }
    return true;
}

bool xtop_test_election_data_manager_fixture::dereg_nodes_from_standby(std::size_t node_count, common::xnode_type_t node_type, std::string node_id_prefix) {
    for (std::size_t index = 1; index <= node_count; ++index) {
        common::xnode_id_t node_id = build_account_address(node_id_prefix, index);
        if (!delete_standby_node(node_type, node_id)) {
            return false;
        }
    }
    return true;
}

bool xtop_test_election_data_manager_fixture::add_election_result(common::xnode_type_t node_type,
                                                                  common::xcluster_id_t cid,
                                                                  common::xgroup_id_t gid,
                                                                  xelection_info_bundle_t election_info_bundle) {
    return election_network_result.result_of(node_type).result_of(cid).result_of(gid).insert(election_info_bundle).second;
}

bool xtop_test_election_data_manager_fixture::delete_election_result(common::xnode_type_t node_type,
                                                                     common::xcluster_id_t cid,
                                                                     common::xgroup_id_t gid,
                                                                     common::xnode_id_t node_id) {
    auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);
    election_result.reset(node_id);
    return !election_result.find(node_id).second;
}

bool xtop_test_election_data_manager_fixture::add_nodes_to_election_result(std::size_t node_count,
                                                                           common::xnode_type_t node_type,
                                                                           common::xcluster_id_t cid,
                                                                           common::xgroup_id_t gid,
                                                                           std::string node_id_prefix) {
    auto & election_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);

    for (std::size_t index = 1; index <= node_count; ++index) {
        common::xnode_id_t node_id = build_account_address(node_id_prefix, index);

        data::election::xelection_info_t new_election_info;
        new_election_info.joined_epoch(common::xelection_round_t{0});
        new_election_info.public_key(xpublic_key_t{std::string{"test_publick_key"} + std::to_string(index)});
        new_election_info.stake(index);
        new_election_info.comprehensive_stake(index);

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(node_id);
        election_info_bundle.election_info(new_election_info);

        if (!add_election_result(node_type, cid, gid, election_info_bundle)) {
            return false;
        }
    }
    return true;
}

NS_END3
