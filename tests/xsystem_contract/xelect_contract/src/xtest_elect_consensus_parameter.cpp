// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xsystem_contract/xelect_contract/xtest_zec_elect_consensus_contract.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xelect_transaction.hpp"

NS_BEG3(top, tests, election)

uint64_t last_read_height{0};
uint64_t last_read_time{0};
uint64_t latest_height{0};

void adjust_setting() {
    // test result:
    // step : 2 -- 3145 logic_time -- 8.7hours
    // step : 3 -- 2105 logic_time -- 5.8hours
    // step : 4 -- 1765 logic_time -- 4.8hours
    // step : 5 -- 1760 logic_time -- 4.8hours
    // limit by cluster_election_interval
    // step : 4 && cluster_election_interval 41 -- 1590 logic_time -- 4.4hours
    // step : 5 && cluster_election_interval 41 -- 1255 logic_time -- 3.5hours
    // step : 6 && cluster_election_interval 41 -- 1110 logic_time -- 3.1hours
    // step : 8 && cluster_election_interval 41 -- 1040 logic_time -- 2.8hours
    // step : 12 && cluster_election_interval 41 -- 1010 logic_time -- 2.8hours

    // step : 4 && cluster_election_interval 31 -- 1615 logic_time -- 4.5hours
    // step : 5 && cluster_election_interval 31 -- 1305 logic_time -- 3.6hours
    // step : 6 && cluster_election_interval 31 -- 1085 logic_time -- 3.0hours
    // step : 8 && cluster_election_interval 31 -- 830 logic_time -- 2.3hours
    // step : 12 && cluster_election_interval 31 -- 785 logic_time -- 2.2hours

    top::config::config_register.get_instance().set(config::xcross_reading_rec_standby_pool_contract_height_step_limitation_onchain_goverance_parameter_t::name, std::to_string(8));
    // top::config::config_register.get_instance().set(data::xcluster_election_interval_onchain_goverance_parameter_t::name, std::to_string(21));
    top::config::config_register.get_instance().set(config::xcross_reading_rec_standby_pool_contract_logic_timeout_limitation_onchain_goverance_parameter_t::name,
                                                    std::to_string(130));
}

void mock_zec_standby_pool_contract(common::xlogic_time_t current_time) {
    auto const height_step_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_height_step_limitation);
    auto const timeout_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_standby_pool_contract_logic_timeout_limitation);

    bool update_rec_standby_pool_contract_read_status{false};
    // calc current_read_height:
    uint64_t next_read_height = last_read_height;
    if (latest_height - last_read_height >= height_step_limitation) {
        next_read_height = last_read_height + height_step_limitation;
        update_rec_standby_pool_contract_read_status = true;
    }
    if (current_time - last_read_time > timeout_limitation) {
        next_read_height = latest_height;
        update_rec_standby_pool_contract_read_status = true;
    }

    if (update_rec_standby_pool_contract_read_status) {
        last_read_height = next_read_height;
        last_read_time = current_time;
    }
    return;
}

void mock_rec_standby_pool_NodeJoin() {
    if (latest_height <= 600)
        latest_height++;
}

#if 0
// this case is to test logical timer the contracts cost with different parameters
TEST_F(xtest_zec_elect_consensus_contract_t, test_parameter) {
    adjust_setting();
    top::config::config_register.get_instance().set(data::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(6));
    top::config::config_register.get_instance().set(data::xmin_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(6));
    top::config::config_register.get_instance().set(data::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(64));
    top::config::config_register.get_instance().set(data::xmax_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(128));

    set_association_result_store();
    common::xnode_type_t auditor_node_type{common::xnode_type_t::consensus_auditor};
    common::xnode_type_t validator_node_type{common::xnode_type_t::consensus_validator};
    common::xzone_id_t zid{common::xconsensus_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid1{common::xauditor_group_id_value_begin};
    common::xgroup_id_t gid2{common::xauditor_group_id_value_begin + 1};
    common::xgroup_id_t gid64{common::xvalidator_group_id_value_begin};
    common::xgroup_id_t gid65{common::xvalidator_group_id_value_begin + 1};
    common::xgroup_id_t gid66{common::xvalidator_group_id_value_begin + 2};
    common::xgroup_id_t gid67{common::xvalidator_group_id_value_begin + 3};
    auto const min_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
    auto const max_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);
    xrange_t<config::xgroup_size_t> group_size_range{min_auditor_group_size, max_auditor_group_size};

    EXPECT_TRUE(add_nodes_to_standby(36, auditor_node_type, "genesis_node"));
    EXPECT_TRUE(add_nodes_to_standby(36, validator_node_type, "genesis_node"));

    uint64_t mock_logic_time = 0;
    uint64_t old_height{0};
    while (mock_logic_time < 100 ||
           all_election_result_store.at(gid2).result_of(common::xbeacon_network_id).result_of(validator_node_type).result_of(cid).result_of(gid67).size() !=
               XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size)) {
        mock_logic_time++;
        common::xlogic_time_t ctime{mock_logic_time};
        // std::cout << "last_read_height: " << last_read_height << std::endl;
        if (old_height != last_read_height) {
            if (last_read_height <= 150) {
                EXPECT_TRUE(add_nodes_to_standby(last_read_height - old_height, auditor_node_type, "adv_node"));
                EXPECT_TRUE(add_nodes_to_standby(last_read_height - old_height, validator_node_type, "adv_node"));
            } else {
                EXPECT_TRUE(add_nodes_to_standby(last_read_height - old_height, validator_node_type, "con_node"));
            }
            // std::cout << "adv_standby_size: " << standby_network_result.result_of(auditor_node_type).size() << "    "
            //           << "con_standby_size: " << standby_network_result.result_of(validator_node_type).size() << std::endl;
            old_height = last_read_height;
        }
        if (mock_logic_time % 5 == 0) {
            // trigger election
            std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            auto random_seed = static_cast<uint64_t>(rng());
            if (test_elect_non_genesis(zid, cid, random_seed, ctime)) {
                std::stringstream ss;
                ss << "group 1:" << all_election_result_store.at(gid1).result_of(common::xbeacon_network_id).result_of(auditor_node_type).result_of(cid).result_of(gid1).size()
                   << " - " << all_election_result_store.at(gid1).result_of(common::xbeacon_network_id).result_of(validator_node_type).result_of(cid).result_of(gid64).size()
                   << " - " << all_election_result_store.at(gid1).result_of(common::xbeacon_network_id).result_of(validator_node_type).result_of(cid).result_of(gid65).size()
                   << "  group 2:" << all_election_result_store.at(gid2).result_of(common::xbeacon_network_id).result_of(auditor_node_type).result_of(cid).result_of(gid2).size()
                   << " - " << all_election_result_store.at(gid2).result_of(common::xbeacon_network_id).result_of(validator_node_type).result_of(cid).result_of(gid66).size()
                   << " - " << all_election_result_store.at(gid2).result_of(common::xbeacon_network_id).result_of(validator_node_type).result_of(cid).result_of(gid67).size();

                std::cout << ss.str() << std::endl;
            }
        }
        if (mock_logic_time % 3 == 0) {
            // nodejoin 5 nodes per 3 logic_time
            mock_rec_standby_pool_NodeJoin();
            mock_rec_standby_pool_NodeJoin();
            mock_rec_standby_pool_NodeJoin();
            mock_rec_standby_pool_NodeJoin();
            mock_rec_standby_pool_NodeJoin();
        }
        if (mock_logic_time % 11 == 0) {
            mock_zec_standby_pool_contract(ctime);
        }
    }
}

#endif
NS_END3