
// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_network_result.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xelection/xstandby_result_store.h"

NS_BEG3(top, tests, election)

using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_network_result_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xstandby_network_result_t;
using top::data::election::xstandby_node_info_t;

common::xaccount_address_t build_account_address(std::string const & account_prefix, size_t index);

class xtop_test_election_data_manager_fixture {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_test_election_data_manager_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_election_data_manager_fixture);
    XDECLARE_DEFAULTED_DESTRUCTOR(xtop_test_election_data_manager_fixture);

    xelection_network_result_t election_network_result;
    xstandby_network_result_t standby_network_result;

    bool add_standby_node(common::xnode_type_t node_type, common::xnode_id_t node_id, xstandby_node_info_t standby_node_info);

    bool delete_standby_node(common::xnode_type_t node_type, common::xnode_id_t node_id);

    bool add_nodes_to_standby(std::size_t node_count, common::xnode_type_t node_type, std::string node_id_prefix);

    bool dereg_nodes_from_standby(std::size_t node_count, common::xnode_type_t node_type, std::string node_id_prefix);

    bool add_election_result(common::xnode_type_t node_type, common::xcluster_id_t cid, common::xgroup_id_t gid, xelection_info_bundle_t election_info_bundle);

    bool delete_election_result(common::xnode_type_t node_type, common::xcluster_id_t cid, common::xgroup_id_t gid, common::xnode_id_t node_id);

    bool add_nodes_to_election_result(std::size_t node_count, common::xnode_type_t node_type, common::xcluster_id_t cid, common::xgroup_id_t gid, std::string node_id_prefix);
};
using xtest_election_data_manager_fixture_t = xtop_test_election_data_manager_fixture;
NS_END3
