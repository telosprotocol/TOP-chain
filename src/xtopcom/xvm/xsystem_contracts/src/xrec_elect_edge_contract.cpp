// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_edge_contract.h"

#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xnode_id.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xgenesis_data.h"
#include "xconfig/xpredefined_configurations.h"
#include "xstake/xstake_algorithm.h"
#include "xvm/xserialization/xserialization.h"

#include <cinttypes>

#ifdef STATIC_CONSENSUS
#    include "xvm/xsystem_contracts/xelection/xstatic_election_center.h"
#endif

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "RecElectEdge_"
#define XEDGE_ELECT XSYSCONTRACT_MODULE XCONTRACT_PREFIX

NS_BEG4(top, xvm, system_contracts, rec)

using common::xnode_id_t;
using data::election::xelection_result_store_t;
using data::election::xstandby_node_info_t;
using data::election::xstandby_result_store_t;

xtop_rec_elect_edge_contract::xtop_rec_elect_edge_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

#ifdef STATIC_CONSENSUS
bool executed_rec_edge_first{false};
bool executed_config_edge{false};
uint64_t before_elect_config_edge_height{UINT64_MAX};
// if enabled static_consensus
// make sure add config in config.xxxx.json
// like this :
//
// "edge_start_nodes":"T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp.0.pub_key,T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446.0.pub_key,T00000LVpL9XRtVdU5RwfnmrCtJhvQFxJ8TB46gB.0.pub_key",
//
// it will elect the first and only round archive nodes as you want.

void xtop_rec_elect_edge_contract::elect_config_nodes(common::xlogic_time_t const current_time) {
    uint64_t latest_height = get_blockchain_height(sys_contract_rec_elect_edge_addr);
    xinfo("[edge_start_nodes] get_latest_height: %" PRIu64, latest_height);
    if (latest_height > before_elect_config_edge_height) {
        // already elect config edge before
        xinfo("[edge_start_nodes] already elect config edge before. get_latest_height: %" PRIu64 " | %" PRIu64, latest_height, before_elect_config_edge_height);
        executed_config_edge = true;
        return;
    }
    before_elect_config_edge_height = latest_height;

    using top::data::election::xelection_info_bundle_t;
    using top::data::election::xelection_info_t;
    using top::data::election::xelection_result_store_t;
    using top::data::election::xstandby_node_info_t;

    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    auto election_result_store =
        xvm::serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, data::election::get_property_by_group_id(common::xdefault_group_id));
    auto node_type = common::xnode_type_t::edge;
    auto & election_group_result = election_result_store.result_of(network_id()).result_of(node_type).result_of(common::xdefault_cluster_id).result_of(common::xdefault_group_id);

    auto nodes_info = xstatic_election_center::instance().get_static_election_nodes("edge_start_nodes");
    for (auto nodes : nodes_info) {
        xelection_info_t new_election_info{};
        new_election_info.consensus_public_key = nodes.pub_key;
        new_election_info.stake = nodes.stake;
        new_election_info.joined_version = common::xelection_round_t{0};

        xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.node_id(nodes.node_id);
        election_info_bundle.election_info(std::move(new_election_info));

        election_group_result.insert(std::move(election_info_bundle));
    }

    election_group_result.election_committee_version(common::xelection_round_t{0});
    election_group_result.timestamp(current_time);
    election_group_result.start_time(current_time);
    if (election_group_result.group_version().empty()) {
        election_group_result.group_version(common::xelection_round_t::max());
    }
    xvm::serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(
        *this, data::election::get_property_by_group_id(common::xdefault_group_id), election_result_store);
}
#endif

void xtop_rec_elect_edge_contract::setup() {
    xelection_result_store_t election_result_store;
    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    for (auto const & property : property_names) {
        STRING_CREATE(property);
        serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
    }
}

void xtop_rec_elect_edge_contract::on_timer(const uint64_t current_time) {
#ifdef STATIC_CONSENSUS
    if (!executed_rec_edge_first) {
        xinfo("[STATIC_CONSENSUS] edge election should elect seed nodes first");
        executed_rec_edge_first = true;
    } else {
        if (xstatic_election_center::instance().if_allow_elect()) {
            if (!executed_config_edge) {
                elect_config_nodes(current_time);
                return;
            }
#    ifndef ELECT_WHEREAFTER
            return;
#    endif
        } else {
            return;
        }
    }
#endif

    XMETRICS_TIME_RECORD(XEDGE_ELECT "on_timer_all_time");
    XMETRICS_CPU_TIME_RECORD(XEDGE_ELECT "on_timer_cpu_time");
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().value(), "xrec_elect_edge_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(SELF_ADDRESS().value() == sys_contract_rec_elect_edge_addr,
                     "xrec_elect_edge_contract_t instance is not triggled by sys_contract_rec_elect_edge_addr");
    // XCONTRACT_ENSURE(current_time <= TIME(), u8"xrec_elect_edge_contract_t::on_timer current_time > consensus leader's time");
    XCONTRACT_ENSURE(current_time + XGET_ONCHAIN_GOVERNANCE_PARAMETER(edge_election_interval) / 2 > TIME(),
                     "xrec_elect_edge_contract_t::on_timer retried too many times. current_time=" + std::to_string(current_time) +
                         ";edge_election_interval=" + std::to_string(XGET_ONCHAIN_GOVERNANCE_PARAMETER(edge_election_interval)) + ";TIME=" + std::to_string(TIME()));
    xinfo("xrec_elect_edge_contract_t::edge_elect %" PRIu64, current_time);

    xrange_t<config::xgroup_size_t> range{0, XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_edge_group_size)};

    auto standby_result_store =
        xvm::serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, sys_contract_rec_standby_pool_addr, data::XPROPERTY_CONTRACT_STANDBYS_KEY);
    auto standby_network_result = standby_result_store.result_of(network_id()).network_result();

    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    for (auto const & property : property_names) {
        auto election_result_store = xvm::serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, property);
        auto & election_network_result = election_result_store.result_of(network_id());
        if (elect_group(common::xedge_zone_id,
                        common::xdefault_cluster_id,
                        common::xdefault_group_id,
                        current_time,
                        current_time,
                        range,
                        standby_network_result,
                        election_network_result)) {
            xvm::serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
        }
    }
}

common::xnode_type_t xtop_rec_elect_edge_contract::standby_type(common::xzone_id_t const & zid,
                                                                common::xcluster_id_t const & cid,
                                                                common::xgroup_id_t const & gid) const {
    assert(zid == common::xedge_zone_id);
    assert(cid == common::xdefault_cluster_id);
    assert(gid == common::xdefault_group_id);

    return common::xnode_type_t::edge;
}

NS_END4
