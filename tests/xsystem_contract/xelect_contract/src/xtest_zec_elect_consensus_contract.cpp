// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xsystem_contract/xelect_contract/xtest_zec_elect_consensus_contract.h"

#include <fstream>
#include <sstream>

NS_BEG3(top, tests, election)

using top::xvm::system_contracts::zec::xzec_elect_consensus_group_contract_t;
using top::xvm::xcontract_helper;

void xtest_zec_elect_consensus_contract_t::set_association_result_store() {
    auto const validator_group_count = XGET_CONFIG(validator_group_count);
    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

    auto const validator_group_count_per_auditor_group = validator_group_count / auditor_group_count;

    auto & election_association_result = m_election_association_result_store.result_of(common::xdefault_cluster_id);
    election_association_result.cluster_version(common::xelection_round_t{0});
    for (std::uint16_t i = 0u; i < validator_group_count; ++i) {
        common::xgroup_id_t consensus_gid{static_cast<common::xgroup_id_t::value_type>(common::xvalidator_group_id_value_begin + i)};
        auto const advance_group_id_offset = static_cast<std::uint16_t>(i / validator_group_count_per_auditor_group);
        common::xgroup_id_t associated_advance_gid{static_cast<common::xgroup_id_t::value_type>(common::xauditor_group_id_value_begin + advance_group_id_offset)};
        election_association_result.insert({consensus_gid, associated_advance_gid});
    }

    all_election_result_store.clear();
    xelection_result_store_t empty_result_store;
    for (auto gidv = common::xauditor_group_id_value_begin; gidv <= common::xauditor_group_id_value_begin + XGET_CONFIG(auditor_group_count); gidv++) {
        auto gid = common::xgroup_id_t{gidv};
        all_election_result_store.insert({gid, empty_result_store});
    }
}

void xtest_zec_elect_consensus_contract_t::show_result(common::xgroup_id_t const & auditor_gid,
                                                       common::xnode_type_t const & node_type,
                                                       common::xcluster_id_t const & cid,
                                                       common::xgroup_id_t const & gid) {
    auto & election_result = all_election_result_store.at(auditor_gid).result_of(common::xbeacon_network_id).result_of(node_type).result_of(cid).result_of(gid);
    printf("\n");
    for(auto & node_info : election_result){
        auto node_id = top::get<xelection_info_bundle_t>(node_info).account_address();
        auto index = atoi(node_id.to_string().substr(strlen(TEST_NODE_ID_AUDITOR_PREFIX), 3).c_str());
        printf("%3d ", index);
    }
    printf("\n");
}

bool xtest_zec_elect_consensus_contract_t::check_election_result_XOR() {
    auto consensus_cluster_count = all_election_result_store.size();
    for (std::size_t index = 1; index <= consensus_cluster_count; ++index) {
        auto consensus_group_id = common::xgroup_id_t(common::xauditor_group_id_value_begin + index - 1);
        auto const & election_network_result = all_election_result_store.at(consensus_group_id);
        auto const & auditor_group_result = election_network_result.result_of(common::xbeacon_network_id)
                                                .result_of(common::xnode_type_t::consensus_auditor)
                                                .result_of(common::xdefault_cluster_id)
                                                .result_of(consensus_group_id);
        auto const & validator_result =
            election_network_result.result_of(common::xbeacon_network_id).result_of(common::xnode_type_t::consensus_validator).result_of(common::xdefault_cluster_id);

        for (auto const & node_info : auditor_group_result) {
            auto node_id = top::get<xelection_info_bundle_t>(node_info).account_address();
            for (auto const & v : validator_result) {
                auto const & validator_group_result = top::get<xelection_group_result_t>(v);
                if (validator_group_result.find(node_id).second) {
                    return false;
                }
            }
            for (std::size_t index2 = 1; index2 <= consensus_cluster_count; ++index2) {
                if (index == index2)
                    continue;
                auto other_consensus_group_id = common::xgroup_id_t(common::xauditor_group_id_value_begin + index2 - 1);
                auto const & other_auditor_group_result = all_election_result_store.at(other_consensus_group_id)
                                                              .result_of(common::xbeacon_network_id)
                                                              .result_of(common::xnode_type_t::consensus_auditor)
                                                              .result_of(common::xdefault_cluster_id)
                                                              .result_of(other_consensus_group_id);
                if (other_auditor_group_result.find(node_id).second) {
                    return false;
                }
            }
        }

        for (auto const & v : validator_result) {
            auto const & validator_group_result = top::get<xelection_group_result_t>(v);
            auto const & this_group_id = top::get<const common::xgroup_id_t>(v);
            for (auto const & node_info : validator_group_result) {
                auto node_id = top::get<xelection_info_bundle_t>(node_info).account_address();
                for (auto const & v2 : validator_result) {
                    auto const & other_group_id = top::get<const common::xgroup_id_t>(v2);
                    if (this_group_id == other_group_id)
                        continue;
                    auto const & that_validator_group_result = top::get<xelection_group_result_t>(v2);
                    if (that_validator_group_result.find(node_id).second) {
                        return false;
                    }
                }

                for (std::size_t index2 = 1; index2 <= consensus_cluster_count; ++index2) {
                    if (index == index2)
                        continue;
                    auto other_consensus_group_id = common::xgroup_id_t(common::xauditor_group_id_value_begin + index2 - 1);
                    auto const & other_consensus_validator_result = all_election_result_store.at(other_consensus_group_id)
                                                                        .result_of(common::xbeacon_network_id)
                                                                        .result_of(common::xnode_type_t::consensus_validator)
                                                                        .result_of(common::xdefault_cluster_id);
                    for (auto ov : other_consensus_validator_result) {
                        auto validator_group_result = top::get<xelection_group_result_t>(ov);
                        if (validator_group_result.find(node_id).second) {
                            return false;
                        }
                    }
                }
            }
        }
    }
    return true;
}

void xtest_zec_elect_consensus_contract_t::cal_group_stake(std::size_t consensus_cluster_size, std::size_t per_validator_group_cnt) {
    std::size_t validator_group_index = common::xvalidator_group_id_value_begin;
    std::size_t auditor_group_index = common::xauditor_group_id_value_begin;

    for (std::size_t index = 0; index < consensus_cluster_size; index++) {
        auto auditor_group_id = common::xgroup_id_t{auditor_group_index};
        auto const & auditor_election_group_result = all_election_result_store.at(auditor_group_id)
                                                         .result_of(common::xbeacon_network_id)
                                                         .result_of(common::xnode_type_t::consensus_auditor)
                                                         .result_of(common::xdefault_cluster_id)
                                                         .result_of(auditor_group_id);
        uint64_t auditor_stake{0};
        for (auto & node_info : auditor_election_group_result) {
            auditor_stake += top::get<xelection_info_bundle_t>(node_info).election_info().stake();
        }
        auditor_stake /= auditor_election_group_result.size();
        group_stake_sum[auditor_group_id] += auditor_stake;
        printf("group:%d %10" PRIu64 "      ", auditor_group_id.value(), auditor_stake);
        for (std::size_t index2 = 0; index2 < per_validator_group_cnt; index2++) {
            auto validator_group_id = common::xgroup_id_t{validator_group_index};
            auto const & validaotr_election_group_result = all_election_result_store.at(auditor_group_id)
                                                               .result_of(common::xbeacon_network_id)
                                                               .result_of(common::xnode_type_t::consensus_validator)
                                                               .result_of(common::xdefault_cluster_id)
                                                               .result_of(validator_group_id);
            uint64_t validator_stake{0};
            for (auto & node_info : validaotr_election_group_result) {
                validator_stake += top::get<xelection_info_bundle_t>(node_info).election_info().stake();
            }
            validator_stake /= validaotr_election_group_result.size();
            group_stake_sum[validator_group_id] += validator_stake;
            printf("group:%d %10" PRIu64 "      ", validator_group_id.value(), validator_stake);
            validator_group_index++;
        }
        auditor_group_index++;
    }
    printf("\n");
    return;
}

void xtest_zec_elect_consensus_contract_t::calc_advance_election_count(std::size_t consensus_cluster_size, std::size_t per_validator_group_cnt) {
    for (std::size_t index = 0; index < consensus_cluster_size; index++) {
        common::xgroup_id_t consensus_cluster_group_id{common::xauditor_group_id_value_begin + index};
        auto & election_group_result = all_election_result_store.at(consensus_cluster_group_id)
                                           .result_of(common::xbeacon_network_id)
                                           .result_of(common::xnode_type_t::consensus_auditor)
                                           .result_of(common::xdefault_cluster_id)
                                           .result_of(consensus_cluster_group_id);
        std::vector<std::size_t> node_ids;

        for (auto & election_result : election_group_result) {
            auto & election_info = top::get<xelection_info_bundle_t>(election_result);
            auto node_id_string = election_info.account_address().to_string();
            if (node_id_string.substr(0, strlen(TEST_NODE_ID_AUDITOR_PREFIX)) == TEST_NODE_ID_AUDITOR_PREFIX) {
                node_ids.push_back(atoi(node_id_string.substr(strlen(TEST_NODE_ID_AUDITOR_PREFIX), 3).c_str()));
            }
        }

        for (auto node_id : node_ids) {
            elect_in_auditor_times[node_id]++;
        }

        for (std::size_t index2 = 0; index2 < per_validator_group_cnt; index2++) {
            common::xgroup_id_t gid{common::xvalidator_group_id_value_begin + index2 + index * per_validator_group_cnt};

            auto & election_group_result = all_election_result_store.at(consensus_cluster_group_id)
                                               .result_of(common::xbeacon_network_id)
                                               .result_of(common::xnode_type_t::consensus_validator)
                                               .result_of(common::xdefault_cluster_id)
                                               .result_of(gid);
            std::vector<std::size_t> node_ids;

            for (auto & election_result : election_group_result) {
                auto & election_info = top::get<xelection_info_bundle_t>(election_result);
                auto node_id_string = election_info.account_address().to_string();
                if (node_id_string.substr(0, strlen(TEST_NODE_ID_AUDITOR_PREFIX)) == TEST_NODE_ID_AUDITOR_PREFIX) {
                    node_ids.push_back(atoi(node_id_string.substr(strlen(TEST_NODE_ID_AUDITOR_PREFIX), 3).c_str()));
                }
            }

            for (auto node_id : node_ids) {
                elect_in_validator_times[node_id]++;
            }
        }
    }
    return;
}

void xtest_zec_elect_consensus_contract_t::cal_advance_node_in_cnt(std::size_t consensus_cluster_size, std::size_t per_validator_group_cnt) {
    for (std::size_t index = 0; index < consensus_cluster_size; index++) {
        common::xgroup_id_t consensus_cluster_group_id{common::xauditor_group_id_value_begin + index};
        auto & election_group_result = all_election_result_store.at(consensus_cluster_group_id)
                                           .result_of(common::xbeacon_network_id)
                                           .result_of(common::xnode_type_t::consensus_auditor)
                                           .result_of(common::xdefault_cluster_id)
                                           .result_of(consensus_cluster_group_id);
        std::vector<std::size_t> auditor_node_ids;

        for (auto & election_result : election_group_result) {
            auto & election_info = top::get<xelection_info_bundle_t>(election_result);
            auto node_id_string = election_info.account_address().to_string();
            if (node_id_string.substr(0, strlen(TEST_NODE_ID_AUDITOR_PREFIX)) == TEST_NODE_ID_AUDITOR_PREFIX) {
                auditor_node_ids.push_back(atoi(node_id_string.substr(strlen(TEST_NODE_ID_AUDITOR_PREFIX), 4).c_str()));
            }
        }

        for (auto node_id : auditor_node_ids) {
            adv_node_elect_in_auditor_cnt[index][node_id]++;
        }

        for (std::size_t index2 = 0; index2 < per_validator_group_cnt; index2++) {
            common::xgroup_id_t gid{common::xvalidator_group_id_value_begin + index2 + index * per_validator_group_cnt};

            auto & election_group_result = all_election_result_store.at(consensus_cluster_group_id)
                                               .result_of(common::xbeacon_network_id)
                                               .result_of(common::xnode_type_t::consensus_validator)
                                               .result_of(common::xdefault_cluster_id)
                                               .result_of(gid);
            std::vector<std::size_t> validator_node_ids;

            for (auto & election_result : election_group_result) {
                auto & election_info = top::get<xelection_info_bundle_t>(election_result);
                auto node_id_string = election_info.account_address().to_string();
                if (node_id_string.substr(0, strlen(TEST_NODE_ID_AUDITOR_PREFIX)) == TEST_NODE_ID_AUDITOR_PREFIX) {
                    validator_node_ids.push_back(atoi(node_id_string.substr(strlen(TEST_NODE_ID_AUDITOR_PREFIX), 4).c_str()));
                }
            }

            for (auto node_id : validator_node_ids) {
                adv_node_elect_in_validator_cnt[index * per_validator_group_cnt + index2][node_id]++;
            }
        }
    }
}

void xtest_zec_elect_consensus_contract_t::cal_validator_node_in_cnt(std::size_t consensus_cluster_size, std::size_t per_validator_group_cnt) {
    for (std::size_t index = 0; index < consensus_cluster_size; index++) {
        common::xgroup_id_t consensus_cluster_group_id{common::xauditor_group_id_value_begin + index};
        for (std::size_t index2 = 0; index2 < per_validator_group_cnt; index2++) {
            common::xgroup_id_t gid{common::xvalidator_group_id_value_begin + index2 + index * per_validator_group_cnt};

            auto & election_group_result = all_election_result_store.at(consensus_cluster_group_id)
                                               .result_of(common::xbeacon_network_id)
                                               .result_of(common::xnode_type_t::consensus_validator)
                                               .result_of(common::xdefault_cluster_id)
                                               .result_of(gid);
            std::vector<std::size_t> validator_node_ids;

            for (auto & election_result : election_group_result) {
                auto & election_info = top::get<xelection_info_bundle_t>(election_result);
                auto node_id_string = election_info.account_address().to_string();
                if (node_id_string.substr(0, strlen(TEST_NODE_ID_VALIDATOR_PREFIX)) == TEST_NODE_ID_VALIDATOR_PREFIX) {
                    validator_node_ids.push_back(atoi(node_id_string.substr(strlen(TEST_NODE_ID_VALIDATOR_PREFIX), 4).c_str()));
                }
            }

            for (auto node_id : validator_node_ids) {
                val_node_elect_in_validator_cnt[index * per_validator_group_cnt + index2][node_id]++;
            }
        }
    }
}

bool xtest_zec_elect_consensus_contract_t::test_elect_auditor_validator(common::xzone_id_t const & zid,
                                                                        common::xcluster_id_t const & cid,
                                                                        common::xgroup_id_t const & auditor_group_id,
                                                                        std::uint64_t const random_seed,
                                                                        common::xlogic_time_t const election_timestamp,
                                                                        common::xlogic_time_t const start_time) {
    return m_zec_elect_consensus.elect_auditor_validator(
        zid, cid, auditor_group_id, random_seed, election_timestamp, start_time, m_election_association_result_store, standby_network_result, all_election_result_store);
}

bool xtest_zec_elect_consensus_contract_t::test_elect_non_genesis(common::xzone_id_t const & zid,
                                                                  common::xcluster_id_t const & cid,
                                                                  std::uint64_t const random_seed,
                                                                  common::xlogic_time_t const election_timestamp) {
    bool result{false};
    auto const validator_group_count = XGET_CONFIG(validator_group_count);
    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

    auto cluster_election_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval);
    XATTRIBUTE_MAYBE_UNUSED auto zone_election_trigger_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(zone_election_trigger_interval);

    auto const max_advance_rotation_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_rotation_count);

    assert(zone_election_trigger_interval < cluster_election_interval);

    auto const total_group_count = auditor_group_count + validator_group_count;

    uint16_t auditor_rotation_num{0};

    for (uint16_t index = 0u; (index < auditor_group_count) && (auditor_rotation_num < max_advance_rotation_num); ++index) {
        common::xgroup_id_t auditor_group_id{static_cast<common::xgroup_id_t::value_type>(common::xauditor_group_id_value_begin + index % auditor_group_count)};
        assert(auditor_group_id >= common::xauditor_group_id_begin && auditor_group_id < common::xauditor_group_id_end);

        // auto & election_result_store = all_election_result_store.at(auditor_group_id);
        // auto & election_network_result = election_result_store.result_of(common::xbeacon_network_id);

        auto const & auditor_group = all_election_result_store.at(auditor_group_id)
                                         .result_of(common::xbeacon_network_id)
                                         .result_of(common::xnode_type_t::consensus_auditor)
                                         .result_of(cid)
                                         .result_of(auditor_group_id);

        auto rotation_flag = auditor_group.empty();
        if (rotation_flag == false) {
            int const election_interval_wave_range = total_group_count + index;
            int const election_interval_wave_offset = random_seed % election_interval_wave_range - election_interval_wave_range / 2;
            auto const rotation_time = election_interval_wave_offset + cluster_election_interval;

            rotation_flag = (election_timestamp - auditor_group.timestamp()) >= rotation_time;
        }
        if (!rotation_flag) {
            std::stringstream ss;
            ss << "time: " << election_timestamp << " group " << static_cast<std::uint16_t>(auditor_group_id.value()) << ": not this rotation, pass. ";
            // std::cout << ss.str() << std::endl;
            continue;
        }
        std::stringstream ss;
        ss << "time: " << election_timestamp << " elect group " << static_cast<std::uint16_t>(auditor_group_id.value());
        std::cout << ss.str() << std::endl;
        result|=test_elect_auditor_validator(zid, cid, auditor_group_id, random_seed, election_timestamp, election_timestamp);
    }
    return result;
}

void xtest_zec_elect_consensus_contract_t::SetUp() {}
void xtest_zec_elect_consensus_contract_t::TearDown() {}



#if 0
// this test is to build consensus_stake_model
TEST_F(xtest_zec_elect_consensus_contract_t, count_election_in_times) {
    std::size_t auditor_nodes_per_segment{27};
    top::config::config_register.get_instance().set(data::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(64));
    top::config::config_register.get_instance().set(data::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(64));
    top::config::config_register.get_instance().set(data::xmin_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(128));
    top::config::config_register.get_instance().set(data::xmax_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(128));
    top::config::config_register.get_instance().set(data::xauditor_nodes_per_segment_onchain_goverance_parameter_t::name, std::to_string(auditor_nodes_per_segment));

    top::config::config_register.get_instance().set(data::xauditor_group_count_onchain_goverance_parameter_t::name, std::to_string(2));
    top::config::config_register.get_instance().set(data::xvalidator_group_count_onchain_goverance_parameter_t::name, std::to_string(4));

    set_association_result_store();
    common::xzone_id_t zid{common::xconsensus_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xauditor_group_id_begin};

    auto const min_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
    auto const max_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);
    xrange_t<config::xgroup_size_t> group_size_range{min_auditor_group_size, max_auditor_group_size};
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    auto random_seed = static_cast<uint64_t>(rng());


    std::size_t node_count{900};
    for (std::size_t index = 1; index <= node_count; ++index) {
        common::xnode_id_t node_id{std::string(TEST_NODE_ID_AUDITOR_PREFIX + std::to_string(index))};
        xstandby_node_info_t standby_node_info;
        standby_node_info.consensus_public_key = top::xpublic_key_t{std::string{"test_publick_key_"} + std::to_string(index)};
        standby_node_info.comprehensive_stake = 1;
        standby_node_info.stake_container = (200 + index) * 10000;
#if defined XENABLE_MOCK_ZEC_STAKE
        standby_node_info.user_request_role = common::xminer_type_t::advance;
#endif
        add_standby_node(common::xnode_type_t::consensus_auditor, node_id, standby_node_info);
    }
    for (std::size_t index = 1; index <= node_count; ++index) {
        common::xnode_id_t node_id{std::string(TEST_NODE_ID_AUDITOR_PREFIX + std::to_string(index))};
        xstandby_node_info_t standby_node_info;
        standby_node_info.consensus_public_key = top::xpublic_key_t{std::string{"test_publick_key_"} + std::to_string(index)};
        standby_node_info.comprehensive_stake = 1;
        standby_node_info.stake_container = (200 + index) * 10000;
#if defined XENABLE_MOCK_ZEC_STAKE
        standby_node_info.user_request_role = common::xminer_type_t::validator;
#endif
        add_standby_node(common::xnode_type_t::consensus_validator, node_id, standby_node_info);
    }
    for (std::size_t index = 1; index <= node_count; ++index) {
        common::xnode_id_t node_id{std::string(TEST_NODE_ID_VALIDATOR_PREFIX + std::to_string(index))};
        xstandby_node_info_t standby_node_info;
        standby_node_info.consensus_public_key = top::xpublic_key_t{std::string{"test_publick_key_"} + std::to_string(index)};
        standby_node_info.comprehensive_stake = 1;
        standby_node_info.stake_container = (200 + index) * 10000;
#if defined XENABLE_MOCK_ZEC_STAKE
        standby_node_info.user_request_role = common::xminer_type_t::validator;
#endif
        add_standby_node(common::xnode_type_t::consensus_validator, node_id, standby_node_info);
    }

    for (std::size_t index = 0; index <= node_count; ++index) {
        elect_in_auditor_times.push_back(0);
        elect_in_validator_times.push_back(0);
    }

    std::size_t elect_count{100};
    common::xgroup_id_t gid1{common::xauditor_group_id_value_begin};
    common::xgroup_id_t gid2{common::xauditor_group_id_value_begin + 1};
    for (std::size_t index = 1; index <= elect_count; ++index) {

        std::mt19937_64 rng2(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        auto random_seed2 = static_cast<uint64_t>(rng2());
        common::xlogic_time_t time2{index};
        ASSERT_TRUE(test_elect_auditor_validator(zid, cid, gid1, random_seed2, time2, time2));
        ASSERT_TRUE(test_elect_auditor_validator(zid, cid, gid2, random_seed2, time2, time2));
        // if (index < 10) {
        //     show_result(common::xgroup_id_t{1}, common::xnode_type_t::consensus_auditor, cid, common::xgroup_id_t{1});
        //     show_result(common::xgroup_id_t{1}, common::xnode_type_t::consensus_validator, cid, common::xgroup_id_t{64});
        //     show_result(common::xgroup_id_t{1}, common::xnode_type_t::consensus_validator, cid, common::xgroup_id_t{65});

        //     show_result(common::xgroup_id_t{2}, common::xnode_type_t::consensus_auditor, cid, common::xgroup_id_t{2});
        //     show_result(common::xgroup_id_t{2}, common::xnode_type_t::consensus_validator, cid, common::xgroup_id_t{66});
        //     show_result(common::xgroup_id_t{2}, common::xnode_type_t::consensus_validator, cid, common::xgroup_id_t{67});
        // }
        std::printf("election %4zu:", index);
        cal_group_stake(2, 2);
        calc_advance_election_count(2, 2);
        ASSERT_TRUE(check_election_result_XOR());
    }
    std::printf("sum & avg    :");
    common::xgroup_id_t vgid64{common::xvalidator_group_id_value_begin};
    common::xgroup_id_t vgid65{common::xvalidator_group_id_value_begin + 1};
    common::xgroup_id_t vgid66{common::xvalidator_group_id_value_begin + 2};
    common::xgroup_id_t vgid67{common::xvalidator_group_id_value_begin + 3};
    printf("group:%d %10lu      ", gid1.value(), group_stake_sum[gid1] / elect_count);
    printf("group:%d %10lu      ", vgid64.value(), group_stake_sum[vgid64] / elect_count);
    printf("group:%d %10lu      ", vgid65.value(), group_stake_sum[vgid65] / elect_count);
    printf("group:%d %10lu      ", gid2.value(), group_stake_sum[gid2] / elect_count);
    printf("group:%d %10lu      ", vgid66.value(), group_stake_sum[vgid66] / elect_count);
    printf("group:%d %10lu      \n", vgid67.value(), group_stake_sum[vgid67] / elect_count);

    std::size_t avg_a = 0, avg_v = 0;
    for (std::size_t index = 1; index <= node_count; ++index) {
        std::printf("%3d/%3d ", elect_in_auditor_times[node_count - index], elect_in_validator_times[node_count - index]);
        avg_a += elect_in_auditor_times[node_count - index];
        avg_v += elect_in_validator_times[node_count - index];
        if (index % auditor_nodes_per_segment == 0) {
            std::printf("avg_a/avg_v: %3zu/%3zu \n", avg_a / auditor_nodes_per_segment, avg_v / auditor_nodes_per_segment);
            avg_a = 0;
            avg_v = 0;
        }
    }
}

#endif

#if 0
// this test is to build advance&&validator node in&&out count
TEST_F(xtest_zec_elect_consensus_contract_t, count_adv_node_election_times) {
    std::size_t auditor_nodes_per_segment{27};
    std::size_t begin_stake{1000};
    std::size_t stake_offset{8};
    std::size_t stake_mul{1}; // {1008 , 1016 , ... , 5000 }* 1
    top::config::config_register.get_instance().set(config::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(64));
    top::config::config_register.get_instance().set(config::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(64));
    top::config::config_register.get_instance().set(config::xmin_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(128));
    top::config::config_register.get_instance().set(config::xmax_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(128));
    top::config::config_register.get_instance().set(config::xauditor_nodes_per_segment_onchain_goverance_parameter_t::name, std::to_string(auditor_nodes_per_segment));

    top::config::config_register.get_instance().set(config::xauditor_group_count_onchain_goverance_parameter_t::name, std::to_string(2));
    top::config::config_register.get_instance().set(config::xvalidator_group_count_onchain_goverance_parameter_t::name, std::to_string(4));

    set_association_result_store();
    common::xzone_id_t zid{common::xconsensus_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xauditor_group_id_begin};

    auto const min_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
    auto const max_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);
    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);
    auto const validator_group_count = XGET_CONFIG(validator_group_count);
    xrange_t<config::xgroup_size_t> group_size_range{min_auditor_group_size, max_auditor_group_size};
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    auto random_seed = static_cast<uint64_t>(rng());


    std::size_t node_count{500};
    for (std::size_t index = 1; index <= node_count; ++index) {
        common::xnode_id_t node_id{std::string(TEST_NODE_ID_AUDITOR_PREFIX + std::to_string(index))};
        xstandby_node_info_t standby_node_info;
        standby_node_info.consensus_public_key = top::xpublic_key_t{std::string{"test_publick_key_"} + std::to_string(index)};
        standby_node_info.stake_container[common::xnode_type_t::consensus_auditor] = (begin_stake + index * stake_offset) * stake_mul;
#if defined XENABLE_MOCK_ZEC_STAKE
        standby_node_info.user_request_role = common::xminer_type_t::advance;
#endif
        add_standby_node(common::xnode_type_t::consensus_auditor, node_id, standby_node_info);
    }
    for (std::size_t index = 1; index <= node_count; ++index) {
        common::xnode_id_t node_id{std::string(TEST_NODE_ID_AUDITOR_PREFIX + std::to_string(index))};
        xstandby_node_info_t standby_node_info;
        standby_node_info.consensus_public_key = top::xpublic_key_t{std::string{"test_publick_key_"} + std::to_string(index)};
        standby_node_info.stake_container[common::xnode_type_t::consensus_validator] = (begin_stake + index * stake_offset) * stake_mul;
#if defined XENABLE_MOCK_ZEC_STAKE
        standby_node_info.user_request_role = common::xminer_type_t::validator;
#endif
        add_standby_node(common::xnode_type_t::consensus_validator, node_id, standby_node_info);
    }
    for (std::size_t index = 1; index <= node_count; ++index) {
        common::xnode_id_t node_id{std::string(TEST_NODE_ID_VALIDATOR_PREFIX + std::to_string(index))};
        xstandby_node_info_t standby_node_info;
        standby_node_info.consensus_public_key = top::xpublic_key_t{std::string{"test_publick_key_"} + std::to_string(index)};
        standby_node_info.stake_container[common::xnode_type_t::consensus_validator] = (begin_stake + index * stake_offset) * stake_mul;
#if defined XENABLE_MOCK_ZEC_STAKE
        standby_node_info.user_request_role = common::xminer_type_t::validator;
#endif
        add_standby_node(common::xnode_type_t::consensus_validator, node_id, standby_node_info);
    }

    std::vector<uint16_t> tmp(node_count + 1, 0);
    for (std::size_t i = 0; i < auditor_group_count; ++i)
        adv_node_elect_in_auditor_cnt.push_back(tmp);
    for (std::size_t i = 0; i < validator_group_count; ++i)
        adv_node_elect_in_validator_cnt.push_back(tmp);
    for(std::size_t i = 0;i<validator_group_count;++i)
        val_node_elect_in_validator_cnt.push_back(tmp);

    std::size_t elect_count{1000};
    common::xgroup_id_t gid1{common::xauditor_group_id_value_begin};
    common::xgroup_id_t gid2{common::xauditor_group_id_value_begin + 1};
    for (std::size_t index = 1; index <= elect_count; ++index) {
        if (index % 10 == 0) {
            std::printf("elect count : %zu \n", index);
        }
        std::mt19937_64 rng2(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        auto random_seed2 = static_cast<uint64_t>(rng2());
        common::xlogic_time_t time{index};
        for (auto gi = 0; gi < auditor_group_count; gi++) {
            common::xgroup_id_t gid{common::xauditor_group_id_value_begin + gi};
            ASSERT_TRUE(test_elect_auditor_validator(zid, cid, gid, random_seed2, time, time));
        }
        cal_advance_node_in_cnt(auditor_group_count, validator_group_count / auditor_group_count);
        cal_validator_node_in_cnt(auditor_group_count, validator_group_count / auditor_group_count);
        // ASSERT_TRUE(check_election_result_XOR());
    }

    std::ofstream adv_node_data;
    adv_node_data.open("adv_node_data");
    adv_node_data << "adv_node_elect_in_group_cnt:" << std::endl;
    for (std::size_t index = 0; index < node_count; ++index) {
        for (std::size_t auditor_index = 0; auditor_index < auditor_group_count; ++auditor_index) {
            adv_node_data << adv_node_elect_in_auditor_cnt[auditor_index][node_count - index] << " ";
        }
        for (std::size_t validator_index = 0; validator_index < validator_group_count; ++validator_index) {
            adv_node_data << adv_node_elect_in_validator_cnt[validator_index][node_count - index] << " ";
        }
        adv_node_data << std::endl;
    }
    adv_node_data.close();

    std::ofstream val_node_data;
    val_node_data.open("val_node_data");
    val_node_data << "val_node_elect_in_group_cnt:" << std::endl;
    // std::size_t avg_a = 0, avg_v = 0;
    for (std::size_t index = 0; index < node_count; ++index) {
        for (std::size_t validator_index = 0; validator_index < validator_group_count; ++validator_index) {
            val_node_data << val_node_elect_in_validator_cnt[validator_index][node_count - index] << " ";
        }
        val_node_data << std::endl;
    }
    val_node_data.close();
}
#endif

NS_END3
