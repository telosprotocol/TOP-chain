// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <gtest/gtest.h>

#define private public

#include "tests/xsystem_contract/xelection_algorithm/xtest_election_data_manager_fixture.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xcontract_helper.h"
#include "xvm/xsystem_contracts/xelection/xzec/xzec_elect_consensus_group_contract.h"


XINLINE_CONSTEXPR char const * TEST_NODE_ID_AUDITOR_PREFIX = "test__auditor__node_id_";

XINLINE_CONSTEXPR char const * TEST_NODE_ID_VALIDATOR_PREFIX = "test_validator_node_id_";

NS_BEG3(top, tests, election)

using top::data::election::xelection_association_result_store_t;
using top::data::election::xelection_group_result_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_network_result_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xstandby_network_result_t;
using top::data::election::xstandby_node_info_t;
using top::xvm::xcontract_helper;
using top::xvm::system_contracts::zec::xzec_elect_consensus_group_contract_t;

class xtop_test_zec_elect_consensus_contract
  : public xtest_election_data_manager_fixture_t
  , public testing::Test {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_test_zec_elect_consensus_contract);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_zec_elect_consensus_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_zec_elect_consensus_contract);

    xzec_elect_consensus_group_contract_t m_zec_elect_consensus{common::xbeacon_network_id};

    xelection_association_result_store_t m_election_association_result_store;

    std::unordered_map<common::xgroup_id_t, xelection_result_store_t> all_election_result_store;

    std::unordered_map<common::xgroup_id_t, uint64_t> group_stake_sum;

    std::vector<std::uint16_t> elect_in_auditor_times, elect_in_validator_times;

    std::vector<std::vector<std::uint16_t>> adv_node_elect_in_auditor_cnt, adv_node_elect_in_validator_cnt, val_node_elect_in_validator_cnt;

    void set_association_result_store();

    void show_result(common::xgroup_id_t const & auditor_gid, common::xnode_type_t const & node_type, common::xcluster_id_t const & cid, common::xgroup_id_t const & gid);

    bool check_election_result_XOR();

    void cal_group_stake(std::size_t consensus_cluster_size, std::size_t per_validator_group_cnt);

    void calc_advance_election_count(std::size_t consensus_cluster_size, std::size_t per_validator_group_cnt);

    void cal_advance_node_in_cnt(std::size_t consensus_cluster_size, std::size_t per_validator_group_cnt);

    void cal_validator_node_in_cnt(std::size_t consensus_cluster_size, std::size_t per_validator_group_cnt);

    bool test_elect_auditor_validator(common::xzone_id_t const & zid,
                                      common::xcluster_id_t const & cid,
                                      common::xgroup_id_t const & auditor_group_id,
                                      std::uint64_t const random_seed,
                                      common::xlogic_time_t const election_timestamp,
                                      common::xlogic_time_t const start_time);

    bool test_elect_non_genesis(common::xzone_id_t const & zid,
                                common::xcluster_id_t const & cid,
                                std::uint64_t const random_seed,
                                common::xlogic_time_t const election_timestamp);

protected:
    void SetUp() override;
    void TearDown() override;
};
using xtest_zec_elect_consensus_contract_t = xtop_test_zec_elect_consensus_contract;

NS_END3