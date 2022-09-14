// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once



#include "tests/xsystem_contract/xelection_algorithm/xtest_election_data_manager_fixture.h"
#include "xbasic/xrange.hpp"
#include "xcommon/xip.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xconfig/xpredefined_configurations.h"
#include "xvm/xsystem_contracts/xelection/xelect_consensus_group_contract.h"
#include "xvm/xsystem_contracts/xelection/xelect_nonconsensus_group_contract.h"
#include <gtest/gtest.h>
#include <vector>

NS_BEG3(top, tests, election)

XINLINE_CONSTEXPR char const * TEST_NODE_ID_PREFIX = "T00000LLyxLtW";

using top::data::election::xelection_group_result_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_network_result_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xstandby_network_result_t;
using top::data::election::xstandby_node_info_t;
using top::xvm::system_contracts::xelect_consensus_group_contract_t;
using top::xvm::system_contracts::xelect_nonconsensus_group_contract_t;

class xtop_test_elect_consensus_group_algorithm : public xelect_consensus_group_contract_t {
    using xbase_t = xelect_consensus_group_contract_t;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_elect_consensus_group_algorithm);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_elect_consensus_group_algorithm);

    explicit xtop_test_elect_consensus_group_algorithm(common::xnetwork_id_t const & network_id);
    xcontract_base * clone() override { return {}; }

    void exec(top::xvm::xvm_context * vm_ctx) { return; }

    bool test_elect(common::xzone_id_t const & zid,
                    common::xcluster_id_t const & cid,
                    common::xgroup_id_t const & gid,
                    common::xlogic_time_t const election_timestamp,
                    common::xlogic_time_t const start_time,
                    std::uint64_t const random_seed,
                    xrange_t<top::config::xgroup_size_t> const & group_size_range,
                    data::election::xstandby_network_result_t const & standby_network_result,
                    data::election::xelection_network_result_t & election_network_result);
};
using xtest_elect_consensus_group_algorithm_t = xtop_test_elect_consensus_group_algorithm;

class xtop_test_elect_nonconsensus_group_algorithm : public xelect_nonconsensus_group_contract_t {
    using xbase_t = xelect_nonconsensus_group_contract_t;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_elect_nonconsensus_group_algorithm);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_elect_nonconsensus_group_algorithm);

    explicit xtop_test_elect_nonconsensus_group_algorithm(common::xnetwork_id_t const & network_id);
    xcontract_base * clone() override { return {}; }

    void exec(top::xvm::xvm_context * vm_ctx) { return; }

    bool test_elect(common::xzone_id_t const & zid,
                    common::xcluster_id_t const & cid,
                    common::xgroup_id_t const & gid,
                    common::xlogic_time_t const election_timestamp,
                    common::xlogic_time_t const start_time,
                    xrange_t<config::xgroup_size_t> const & group_size_range,
                    bool force_udpate,
                    data::election::xstandby_network_result_t & standby_network_result,
                    data::election::xelection_network_result_t & election_network_result);

    common::xnode_type_t standby_type(common::xzone_id_t const & zid,
                                      common::xcluster_id_t const & cid,
                                      common::xgroup_id_t const & gid) const override;
};
using xtest_elect_nonconsensus_group_algorithm_t = xtop_test_elect_nonconsensus_group_algorithm;

class xtop_test_elect_group_contract_fixture
  : public xtop_test_election_data_manager_fixture
  , public testing::Test {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_test_elect_group_contract_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_elect_group_contract_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_elect_group_contract_fixture);

    xtest_elect_consensus_group_algorithm_t m_elect_consensus_group{common::xbeacon_network_id};
    xtest_elect_nonconsensus_group_algorithm_t m_elect_nonconsensus_group{common::xbeacon_network_id};

    void calc_election_result(common::xnode_type_t node_type, common::xcluster_id_t cid, common::xgroup_id_t gid);
    std::vector<std::uint16_t> elect_in_times;

    std::vector<common::xnode_id_t> electing_in_nodes;

protected:
    void SetUp() override;

    void TearDown() override;
};
using xtest_elect_consensus_group_contract_fixture_t = xtop_test_elect_group_contract_fixture;
using xtest_elect_nonconsensus_group_contract_fixture_t = xtop_test_elect_group_contract_fixture;

NS_END3