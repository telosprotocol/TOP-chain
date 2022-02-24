// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xelect_nonconsensus_group_contract.h"

#include "xbasic/xutility.h"
#include "xcommon/xnode_id.h"
#include "xcommon/xsharding_info.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xgenesis_data.h"

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "NonConsensusElection_"
#define XNONCONSENSUS_ELECTION XSYSCONTRACT_MODULE XCONTRACT_PREFIX

NS_BEG3(top, xvm, system_contracts)

using common::xnode_id_t;
using data::election::xelection_info_bundle_t;
using data::election::xelection_info_t;
using data::election::xelection_result_store_t;
using data::election::xstandby_node_info_t;

xtop_elect_nonconsensus_group_contract::xtop_elect_nonconsensus_group_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

bool xtop_elect_nonconsensus_group_contract::elect_group(common::xzone_id_t const & zid,
                                                         common::xcluster_id_t const & cid,
                                                         common::xgroup_id_t const & gid,
                                                         common::xlogic_time_t const election_timestamp,
                                                         common::xlogic_time_t const start_time,
                                                         xrange_t<config::xgroup_size_t> const & group_size_range,
                                                         data::election::xstandby_network_result_t & standby_network_result,
                                                         data::election::xelection_network_result_t & election_network_result) {
    assert(!broadcast(cid) && !broadcast(gid));
    auto const log_prefix = "[elect non-consensus group contract] zone " + zid.to_string() + " cluster " + cid.to_string() + " group " + gid.to_string() + ":";

    auto const min_elect_group_size = group_size_range.begin;
    auto const max_elect_group_size = group_size_range.end;

    common::xnode_type_t const node_type = standby_type(zid, cid, gid);
    assert(node_type != common::xnode_type_t::invalid);
    assert(max_elect_group_size);

    auto & standby_result = standby_network_result.result_of(node_type);
    if (standby_result.size() < min_elect_group_size) {
        xwarn("%s start electing but no enough standby nodes available", log_prefix.c_str());
        return false;
    }

    data::election::xelection_network_result_t new_election_network_result;
    auto & new_group_result = new_election_network_result.result_of(node_type).result_of(cid).result_of(gid);
    auto & current_group = election_network_result.result_of(node_type).result_of(cid).result_of(gid);
    bool node_change{false};
    size_t elect_node_size{0};

    for (auto & new_node_info : standby_result) {
        auto const & node_id = top::get<xnode_id_t const>(new_node_info);
        auto const & node_standby_info = top::get<xstandby_node_info_t>(new_node_info);

        xelection_info_t new_election_info{};
        new_election_info.consensus_public_key = node_standby_info.consensus_public_key;
        new_election_info.stake = node_standby_info.stake(node_type);

        xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.node_id(node_id);
        election_info_bundle.election_info(std::move(new_election_info));

        if (elect_node_size++ >= max_elect_group_size) {
            xinfo("node size %" PRIu32 " reaches upper limit %" PRIu16, elect_node_size, max_elect_group_size);
            break;
        }

        new_group_result.insert(std::move(election_info_bundle));
    }
    node_change = (new_group_result != current_group);
    if (node_change) {
        for (auto & node_info : new_group_result) {
            auto const node_id = top::get<xelection_info_bundle_t>(node_info).node_id();
            XMETRICS_PACKET_INFO(
                XNONCONSENSUS_ELECTION "elect_in", "zone_id", zid.to_string(), "cluster_id", cid.to_string(), "group_id", gid.to_string(), "node_id", node_id.value());
        }
#if defined(DEBUG)
        for (auto const & node_info : new_group_result) {
            xdbg("%s elected in %s", log_prefix.c_str(), top::get<xelection_info_bundle_t>(node_info).node_id().c_str());
        }
#endif
        new_group_result.election_committee_version(common::xelection_round_t{0});
        new_group_result.timestamp(election_timestamp);
        new_group_result.start_time(start_time);
        if (new_group_result.group_version().empty()) {
            new_group_result.group_version(common::xelection_round_t::max());
        }

        xwarn("%s version %s size %zu timestamp %" PRIu64 " start time %" PRIu64,
              log_prefix.c_str(),
              new_group_result.group_version().to_string().c_str(),
              new_group_result.size(),
              new_group_result.timestamp(),
              new_group_result.start_time());

        current_group = new_group_result;
        xinfo("%s new election success", log_prefix.c_str());
    } else {
        xinfo("%s new election failed. no elect in or out.", log_prefix.c_str());
    }

    return node_change;
}

NS_END3
