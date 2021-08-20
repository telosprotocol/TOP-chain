// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define private public

#include "tests/xsystem_contract/xelect_contract/xtest_zec_elect_consensus_contract.h"
#include "xbase/xutl.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelect_transaction.hpp"

NS_BEG3(top, tests, election)

using top::data::election::xstandby_result_store_t;

TEST_F(xtest_zec_elect_consensus_contract_t, BENCH) {
    top::config::config_register.get_instance().set(config::xmin_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(6));
    top::config::config_register.get_instance().set(config::xmin_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(6));
    top::config::config_register.get_instance().set(config::xmax_auditor_group_size_onchain_goverance_parameter_t::name, std::to_string(64));
    top::config::config_register.get_instance().set(config::xmax_validator_group_size_onchain_goverance_parameter_t::name, std::to_string(128));

    top::config::config_register.get_instance().set(config::xauditor_group_count_configuration_t::name, std::to_string(2));
    top::config::config_register.get_instance().set(config::xvalidator_group_count_configuration_t::name, std::to_string(4));

    set_association_result_store();
    common::xzone_id_t zid{common::xconsensus_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    auto random_seed = static_cast<uint64_t>(rng());
    auto elect_timestamp = common::xlogic_time_t{0};
    auto start_timestamp = common::xlogic_time_t{0};

    xstandby_result_store_t standby_result_store;

    add_standby_node_to_result_store(standby_result_store, common::xbeacon_network_id, 10000, common::xnode_type_t::consensus_auditor, "test_auditor_nodes");
    add_standby_node_to_result_store(standby_result_store, common::xbeacon_network_id, 10000, common::xnode_type_t::consensus_validator, "test_validator_nodes");

    standby_result_store.result_of(common::xbeacon_network_id).set_activate_state(true);
    // standby_result_store.insert({common::xnetwork_id_t{0}, standby_network_result});
    auto standby_bytes = codec::msgpack_encode<xstandby_result_store_t>(standby_result_store);
    std::string standby_property_result = {std::begin(standby_bytes), std::end(standby_bytes)};

    auto association_bytes = codec::msgpack_encode<xelection_association_result_store_t>(m_election_association_result_store);
    std::string election_association_property_result = {std::begin(association_bytes), std::end(association_bytes)};

    std::unordered_map<common::xgroup_id_t, std::string> all_cluster_election_result_store_property_result;
    std::unordered_map<common::xgroup_id_t, xelection_result_store_t> result_election_store;
    xelection_result_store_t election_result_store;
    result_election_store.insert({common::xgroup_id_t{1}, election_result_store});
    result_election_store.insert({common::xgroup_id_t{2}, election_result_store});
    for (auto _p : result_election_store) {
        auto election_result_bytes = codec::msgpack_encode<xelection_result_store_t>(top::get<xelection_result_store_t>(_p));
        std::string election_result_property_result = {std::begin(election_result_bytes), std::end(election_result_bytes)};
        all_cluster_election_result_store_property_result.insert({top::get<const common::xgroup_id_t>(_p), election_result_property_result});
    }
    int sum_duration = 0;
    const int test_cnt = 100;
    for (std::size_t test_index = 0; test_index < test_cnt; test_index++) {
        const int64_t start_time = top::base::xtime_utl::time_now_ms();
        m_zec_elect_consensus.elect_inner(zid,
                                          cid,
                                          random_seed,
                                          elect_timestamp + test_index * 100,
                                          start_timestamp + test_index * 100,
                                          2,
                                          election_association_property_result,
                                          standby_property_result,
                                          all_cluster_election_result_store_property_result,
                                          result_election_store);

        const int64_t end_time = top::base::xtime_utl::time_now_ms();
        const int duration = (int)(end_time - start_time) + 1;
        sum_duration += duration;
        std::printf("adv1: %2zu ",
                    result_election_store.at(common::xgroup_id_t{1})
                        .result_of(common::xbeacon_network_id)
                        .result_of(common::xnode_type_t::consensus_auditor)
                        .result_of(cid)
                        .result_of(common::xgroup_id_t{1})
                        .size());
        std::printf("con64: %2zu ",
                    result_election_store.at(common::xgroup_id_t{1})
                        .result_of(common::xbeacon_network_id)
                        .result_of(common::xnode_type_t::consensus_validator)
                        .result_of(cid)
                        .result_of(common::xgroup_id_t{64})
                        .size());
        std::printf("con65: %2zu ",
                    result_election_store.at(common::xgroup_id_t{1})
                        .result_of(common::xbeacon_network_id)
                        .result_of(common::xnode_type_t::consensus_validator)
                        .result_of(cid)
                        .result_of(common::xgroup_id_t{65})
                        .size());
        std::printf("adv1: %2zu ",
                    result_election_store.at(common::xgroup_id_t{2})
                        .result_of(common::xbeacon_network_id)
                        .result_of(common::xnode_type_t::consensus_auditor)
                        .result_of(cid)
                        .result_of(common::xgroup_id_t{2})
                        .size());
        std::printf("con66: %2zu ",
                    result_election_store.at(common::xgroup_id_t{2})
                        .result_of(common::xbeacon_network_id)
                        .result_of(common::xnode_type_t::consensus_validator)
                        .result_of(cid)
                        .result_of(common::xgroup_id_t{66})
                        .size());
        std::printf("con67: %2zu ",
                    result_election_store.at(common::xgroup_id_t{2})
                        .result_of(common::xbeacon_network_id)
                        .result_of(common::xnode_type_t::consensus_validator)
                        .result_of(cid)
                        .result_of(common::xgroup_id_t{67})
                        .size());
        std::printf("cost: %d ms\n", duration);
        all_cluster_election_result_store_property_result.clear();
        for (auto _p : result_election_store) {
            auto election_result_bytes = codec::msgpack_encode<xelection_result_store_t>(top::get<xelection_result_store_t>(_p));
            std::string election_result_property_result = {std::begin(election_result_bytes), std::end(election_result_bytes)};
            all_cluster_election_result_store_property_result.insert({top::get<const common::xgroup_id_t>(_p), election_result_property_result});
        }
        // clear result
        result_election_store.clear();
    }
    std::printf("avg_cost: %d ms\n", sum_duration / test_cnt);
}

NS_END3