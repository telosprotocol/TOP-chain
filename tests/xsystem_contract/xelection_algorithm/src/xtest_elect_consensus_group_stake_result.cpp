// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xsystem_contract/xelection_algorithm/xtest_elect_group_contract_fixture.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xelect_transaction.hpp"

NS_BEG3(top,tests,election)

std::size_t group_size{32};
std::size_t auditor_nodes_per_segment{27};
#if 0
TEST_F(xtest_elect_consensus_group_contract_fixture_t, test_stake) {
    top::config::config_register.get_instance().set(config::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(6));
    top::config::config_register.get_instance().set(config::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(16));
    top::config::config_register.get_instance().set(config::xauditor_nodes_per_segment_onchain_goverance_parameter_t::name, std::to_string(auditor_nodes_per_segment));

    common::xnode_type_t node_type{common::xnode_type_t::consensus_auditor};
    common::xzone_id_t zid{common::xconsensus_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xauditor_group_id_begin};
    auto const min_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
    auto const max_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);
    xrange_t<config::xgroup_size_t> group_size_range{min_auditor_group_size, max_auditor_group_size};


    std::size_t node_count{900};
    for (std::size_t index = 1; index <= node_count; ++index) {
        common::xnode_id_t node_id{std::string(TEST_NODE_ID_PREFIX + std::to_string(index))};
        xstandby_node_info_t standby_node_info;
        // standby_node_info.consensus_public_key = top::xpublic_key_t{std::string{"test_publick_key_"} + std::to_string(index)};
        standby_node_info.stake_container.insert({common::xnode_type_t::consensus_auditor, (100 + index) * 10000});
        standby_node_info.stake_container.insert({common::xnode_type_t::consensus_validator, (100 + index) * 10000});
#if defined XENABLE_MOCK_ZEC_STAKE
        standby_node_info.user_request_role = (node_type == common::xnode_type_t::validator) ? common::xminer_type_t::validator : common::xminer_type_t::advance;
#endif
        add_standby_node(node_type, node_id, standby_node_info);
    }

    for (std::size_t index = 0; index <= node_count; ++index) {
        elect_in_times.push_back(0);
    }
    std::size_t elect_count{100};
    for (std::size_t index = 1; index <= elect_count; ++index) {
        common::xlogic_time_t time{index};
        std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        auto random_seed = static_cast<uint64_t>(rng());
        ASSERT_TRUE(m_elect_consensus_group.test_elect(zid, cid, gid, time, time, random_seed, group_size_range, standby_network_result, election_network_result));
        calc_election_result(node_type, cid, gid);
    }

    std::size_t avg = 0;
    for (std::size_t index = 1; index <= node_count; ++index) {
        std::printf("%5d ", elect_in_times[node_count - index]);
        avg += elect_in_times[node_count - index];
        if (index % auditor_nodes_per_segment == 0) {
            std::printf("avg: %5zu \n", avg / auditor_nodes_per_segment);
            avg = 0;
        }
    }
}
#endif
NS_END3
