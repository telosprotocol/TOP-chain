// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xsystem_contract/xelection_algorithm/xtest_elect_group_contract_fixture.h"

NS_BEG3(top, tests, election)

using top::xvm::system_contracts::xelect_consensus_group_contract_t;

xtop_test_elect_consensus_group_algorithm::xtop_test_elect_consensus_group_algorithm(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

bool xtop_test_elect_consensus_group_algorithm::test_elect(common::xzone_id_t const & zid,
                                                           common::xcluster_id_t const & cid,
                                                           common::xgroup_id_t const & gid,
                                                           common::xlogic_time_t const election_timestamp,
                                                           common::xlogic_time_t const start_time,
                                                           std::uint64_t const random_seed,
                                                           xrange_t<top::config::xgroup_size_t> const & group_size_range,
                                                           data::election::xstandby_network_result_t const & standby_network_result,
                                                           data::election::xelection_network_result_t & election_network_result) {
    return elect_group(zid, cid, gid, election_timestamp, start_time, random_seed, group_size_range, standby_network_result, election_network_result);
}

common::xnode_type_t xtop_test_elect_nonconsensus_group_algorithm::standby_type(common::xzone_id_t const & zid,
                                                                                common::xcluster_id_t const & cid,
                                                                                common::xgroup_id_t const & gid) const {
    if (zid == common::xedge_zone_id) {
        return common::xnode_type_t::edge;
    }

    if (gid == common::xarchive_group_id) {
        return common::xnode_type_t::storage_archive;
    }

    if (gid == common::xexchange_group_id) {
        return common::xnode_type_t::storage_exchange;
    }

    if (zid == common::xfullnode_zone_id) {
        return common::xnode_type_t::fullnode;
    }

    return common::xnode_type_t::invalid;
}

xtop_test_elect_nonconsensus_group_algorithm::xtop_test_elect_nonconsensus_group_algorithm(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

bool xtop_test_elect_nonconsensus_group_algorithm::test_elect(common::xzone_id_t const & zid,
                                                              common::xcluster_id_t const & cid,
                                                              common::xgroup_id_t const & gid,
                                                              common::xlogic_time_t const election_timestamp,
                                                              common::xlogic_time_t const start_time,
                                                              xrange_t<config::xgroup_size_t> const & group_size_range,
                                                              bool const force_update,
                                                              data::election::xstandby_network_result_t & standby_network_result,
                                                              data::election::xelection_network_result_t & election_network_result) {
    return elect_group(zid, cid, gid, election_timestamp, start_time, group_size_range, force_update, standby_network_result, election_network_result);
}

void xtop_test_elect_group_contract_fixture::SetUp() {}

void xtop_test_elect_group_contract_fixture::TearDown() {}

void xtop_test_elect_group_contract_fixture::calc_election_result(common::xnode_type_t node_type, common::xcluster_id_t cid, common::xgroup_id_t gid) {
    auto & election_group_result = election_network_result.result_of(node_type).result_of(cid).result_of(gid);
    std::vector<std::size_t> node_ids;

    for (auto & election_result : election_group_result) {
        auto & election_info = top::get<xelection_info_bundle_t>(election_result);
        auto node_id_string = election_info.account_address().to_string().substr(strlen(TEST_NODE_ID_PREFIX), 3);
        node_ids.push_back(atoi(node_id_string.c_str()));
    }

    for (auto node_id : node_ids) {
        elect_in_times[node_id]++;
    }
    return;
}

NS_END3
