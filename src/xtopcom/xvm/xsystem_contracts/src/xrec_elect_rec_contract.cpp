// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xrec/xrec_elect_rec_contract.h"

#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xgenesis_data.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xrootblock.h"
#include "xvm/xserialization/xserialization.h"

#include <cinttypes>

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "RecElectRec_"
#define XREC_ELECT XSYSCONTRACT_MODULE XCONTRACT_PREFIX

NS_BEG4(top, xvm, system_contracts, rec)

using data::election::xelection_info_bundle_t;
using data::election::xelection_info_t;
using data::election::xelection_result_store_t;
using data::election::xstandby_node_info_t;
using data::election::xstandby_result_store_t;

xtop_rec_elect_rec_contract::xtop_rec_elect_rec_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xtop_rec_elect_rec_contract::setup() {
    xelection_result_store_t election_result_store;
    common::xelection_round_t group_version{0};

    auto & current_group_nodes =
        election_result_store.result_of(network_id()).result_of(common::xnode_type_t::committee).result_of(common::xcommittee_cluster_id).result_of(common::xcommittee_group_id);
    current_group_nodes.election_committee_version(common::xelection_round_t{0});
    current_group_nodes.timestamp(0);
    current_group_nodes.start_time(0);
    current_group_nodes.group_version(group_version);

    auto const max_election_committee_size = config::xmax_election_committee_size_onchain_goverance_parameter_t::value;

    const std::vector<node_info_t> & seeds = data::xrootblock_t::get_seed_nodes();
    for (auto i = 0u; i < seeds.size() && i < max_election_committee_size; ++i) {
        auto const & item = seeds[i];

        common::xnode_id_t node_id{item.m_account};

        xelection_info_t election_info{};
        election_info.joined_version = group_version;
        election_info.stake = 0;
        election_info.consensus_public_key = xpublic_key_t{item.m_publickey};

        xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.node_id(node_id);
        election_info_bundle.election_info(std::move(election_info));

        current_group_nodes.insert(std::move(election_info_bundle));
    }

    STRING_CREATE(data::election::get_property_by_group_id(common::xcommittee_group_id));
    serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(
        *this, data::election::get_property_by_group_id(common::xcommittee_group_id), election_result_store);
}

void xtop_rec_elect_rec_contract::on_timer(common::xlogic_time_t const current_time) {
    XMETRICS_TIME_RECORD(XREC_ELECT "on_timer_all_time");
    XMETRICS_CPU_TIME_RECORD(XREC_ELECT "on_timer_cpu_time");
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().value(), "xtop_rec_elect_rec_contract instance is triggled by " + SOURCE_ADDRESS());
    XCONTRACT_ENSURE(SELF_ADDRESS().value() == sys_contract_rec_elect_rec_addr, u8"xtop_rec_elect_rec_contract instance is not triggled by sys_contract_rec_elect_rec_addr");
    // XCONTRACT_ENSURE(current_time <= TIME(), u8"xtop_rec_elect_rec_contract::on_timer current_time > consensus leader's time");
    XCONTRACT_ENSURE(current_time + XGET_ONCHAIN_GOVERNANCE_PARAMETER(rec_election_interval) / 2 > TIME(),
                     "xrec_elect_rec_contract_t::on_timer retried too many times. TX generated time " + std::to_string(current_time) + " TIME() " + std::to_string(TIME()));

    std::uint64_t random_seed;
    try {
        auto seed = m_contract_helper->get_random_seed();
        random_seed = utl::xxh64_t::digest(seed);
    } catch (std::exception const & eh) {
        xwarn("[rec committee election][on_timer] get random seed failed: %s", eh.what());
        return;
    }
    xinfo("[rec committee election] on_timer random seed %" PRIu64, random_seed);

    auto const rec_election_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(rec_election_interval);
    auto const min_election_committee_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_election_committee_size);
    auto const max_election_committee_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_election_committee_size);

    auto standby_result_store =
        serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, sys_contract_rec_standby_pool_addr, data::XPROPERTY_CONTRACT_STANDBYS_KEY);
    auto standby_network_result = standby_result_store.result_of(network_id()).network_result();

    auto election_result_store =
        serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, data::election::get_property_by_group_id(common::xcommittee_group_id));
    auto & election_network_result = election_result_store.result_of(network_id());

    auto const successful = elect_group(common::xcommittee_zone_id,
                                        common::xcommittee_cluster_id,
                                        common::xcommittee_group_id,
                                        current_time,
                                        current_time + rec_election_interval / 2,
                                        random_seed,
                                        xrange_t<config::xgroup_size_t>{min_election_committee_size, max_election_committee_size},
                                        standby_network_result,
                                        election_network_result);
    if (successful) {
        serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(
            *this, data::election::get_property_by_group_id(common::xcommittee_group_id), election_result_store);
    }
}

NS_END4
