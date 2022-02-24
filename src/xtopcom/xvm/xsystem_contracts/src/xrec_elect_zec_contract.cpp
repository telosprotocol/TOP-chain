// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_zec_contract.h"

#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xgenesis_data.h"
#include "xvm/xserialization/xserialization.h"

#include <cinttypes>

#ifdef STATIC_CONSENSUS
#    include "xvm/xsystem_contracts/xelection/xstatic_election_center.h"
#endif

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "RecElectZec_"
#define XZEC_ELECT XSYSCONTRACT_MODULE XCONTRACT_PREFIX

NS_BEG4(top, xvm, system_contracts, rec)

using data::election::xelection_result_store_t;
using data::election::xstandby_node_info_t;
using data::election::xstandby_result_store_t;

xtop_rec_elect_zec_contract::xtop_rec_elect_zec_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

#ifdef STATIC_CONSENSUS
bool executed_zec{false};
// if enabled static_consensus
// make sure add config in config.xxxx.json
// like this :
//
// "zec_start_nodes":"T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp.0.pub_key,T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446.0.pub_key,T00000LVpL9XRtVdU5RwfnmrCtJhvQFxJ8TB46gB.0.pub_key",
//
// it will elect the first and only round zec nodes as you want.

void xtop_rec_elect_zec_contract::elect_config_nodes(common::xlogic_time_t const current_time) {
    uint64_t latest_height = get_blockchain_height(sys_contract_rec_elect_zec_addr);
    xinfo("[zec_start_nodes] get_latest_height: %" PRIu64, latest_height);
    if (latest_height > 0) {
        executed_zec = true;
        return;
    }

    using top::data::election::xelection_info_bundle_t;
    using top::data::election::xelection_info_t;
    using top::data::election::xelection_result_store_t;
    using top::data::election::xstandby_node_info_t;

    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    auto election_result_store =
        xvm::serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, data::election::get_property_by_group_id(common::xcommittee_group_id));
    auto node_type = common::xnode_type_t::zec;
    auto & election_group_result =
        election_result_store.result_of(network_id()).result_of(node_type).result_of(common::xcommittee_cluster_id).result_of(common::xcommittee_group_id);

    auto nodes_info = xstatic_election_center::instance().get_static_election_nodes("zec_start_nodes");
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
    auto next_version = election_group_result.group_version();
    if (!next_version.empty()) {
        next_version.increase();
    } else {
        next_version = common::xelection_round_t{0};
    }
    election_group_result.group_version(next_version);
    election_group_result.election_committee_version(common::xelection_round_t{0});
    election_group_result.timestamp(current_time);
    election_group_result.start_time(current_time);

    xvm::serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(
        *this, data::election::get_property_by_group_id(common::xcommittee_group_id), election_result_store);
}
#endif

void xtop_rec_elect_zec_contract::setup() {
    xelection_result_store_t election_result_store;
    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    for (auto const & property : property_names) {
        STRING_CREATE(property);
        serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
    }
}

void xtop_rec_elect_zec_contract::on_timer(common::xlogic_time_t const current_time) {
#ifdef STATIC_CONSENSUS
    if (xstatic_election_center::instance().if_allow_elect()) {
        if (!executed_zec) {
            elect_config_nodes(current_time);
            return;
        }
#ifndef ELECT_WHEREAFTER
    return;
#endif
    } else {
        return;
    }
#endif
    XMETRICS_TIME_RECORD(XZEC_ELECT "on_timer_all_time");
    XMETRICS_CPU_TIME_RECORD(XZEC_ELECT "on_timer_cpu_time");
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().value(), "xrec_elect_zec_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(SELF_ADDRESS().value() == sys_contract_rec_elect_zec_addr, "xrec_elect_zec_contract_t instance is not triggled by sys_contract_rec_elect_zec_addr");
    // XCONTRACT_ENSURE(current_time <= TIME(), "xrec_elect_zec_contract_t::on_timer current_time > consensus leader's time");
    XCONTRACT_ENSURE(current_time + XGET_ONCHAIN_GOVERNANCE_PARAMETER(zec_election_interval) / 2 > TIME(),
                     "xrec_elect_zec_contract_t::on_timer retried too many times. TX generated time " + std::to_string(current_time) + " TIME() " + std::to_string(TIME()));

    std::uint64_t random_seed;
    try {
        auto seed = m_contract_helper->get_random_seed();
        random_seed = utl::xxh64_t::digest(seed);
    } catch (std::exception const & eh) {
        xwarn("[zec committee election] get random seed failed: %s", eh.what());
        return;
    }
    xinfo("[zec committee election] on_timer random seed %" PRIu64, random_seed);

    auto const zec_election_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(zec_election_interval);
    auto const min_election_committee_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size);
    auto const max_election_committee_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size);

    auto standby_result_store =
        serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, sys_contract_rec_standby_pool_addr, data::XPROPERTY_CONTRACT_STANDBYS_KEY);
    auto standby_network_result = standby_result_store.result_of(network_id()).network_result();

    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    for (auto const & property : property_names) {
        auto election_result_store = serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, property);
        auto & election_network_result = election_result_store.result_of(network_id());
        auto const & current_group_nodes =
            election_network_result.result_of(common::xnode_type_t::zec).result_of(common::xcommittee_cluster_id).result_of(common::xcommittee_group_id);

        auto start_time = current_time;
        if (!current_group_nodes.empty()) {
            start_time += zec_election_interval / 2;
        }

        auto const successful = elect_group(common::xzec_zone_id,
                                            common::xcommittee_cluster_id,
                                            common::xcommittee_group_id,
                                            current_time,
                                            start_time,
                                            random_seed,
                                            xrange_t<config::xgroup_size_t>{min_election_committee_size, max_election_committee_size},
                                            standby_network_result,
                                            election_network_result);
        if (successful) {
            serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
            xwarn("[zec committee election] successful. timestamp %" PRIu64 " start time %" PRIu64 " random seed %" PRIu64, current_time, start_time, random_seed);
        }
    }
}

NS_END4
