// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xzec/xzec_elect_eth_group_contract.h"

#include "xchain_fork/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xvm/xserialization/xserialization.h"

#include <cinttypes>

#ifdef STATIC_CONSENSUS
#    include "xdata/xelection/xelection_info.h"
#    include "xdata/xelection/xelection_info_bundle.h"
#    include "xvm/xsystem_contracts/xelection/xstatic_election_center.h"
#endif

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "ZecElectEth_"
#define XZEC_ELECT XSYSCONTRACT_MODULE XCONTRACT_PREFIX

NS_BEG4(top, xvm, system_contracts, zec)

using data::election::xelection_info_bundle_t;
using data::election::xelection_network_result_t;
using data::election::xelection_result_store_t;
using data::election::xstandby_node_info_t;
using data::election::xstandby_result_store_t;

xtop_zec_elect_eth_contract::xtop_zec_elect_eth_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {
}

#ifdef STATIC_CONSENSUS
bool executed_eth{false};
// if enabled static_consensus
// make sure add config in config.xxxx.json
// like this :
//
// "eth_start_nodes":"evm_auditor:T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp.0.pub_key,T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446.0.pub_key,T00000LVpL9XRtVdU5RwfnmrCtJhvQFxJ8TB46gB.0.pub_key|evm_validator:T00000LhCXUC5iQCREefnRPRFhxwDJTEbufi41EL.0.pub_key,T00000LTSip8Xbjutrtm8RkQzsHKqt28g97xdUxg.0.pub_key,T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu.0.pub_key",
//
// it will elect the first and only round eth auditor/validator nodes as you want.

void xtop_zec_elect_eth_contract::elect_config_nodes(common::xlogic_time_t const current_time) {
    uint64_t latest_height = get_blockchain_height(sys_contract_zec_elect_eth_addr);
    xinfo("[eth_start_nodes] get_latest_height: %" PRIu64, latest_height);
    if (latest_height > 0) {
        executed_eth = true;
        return;
    }

    using top::data::election::xelection_info_bundle_t;
    using top::data::election::xelection_info_t;
    using top::data::election::xelection_result_store_t;
    using top::data::election::xstandby_node_info_t;


    // auditor:
    common::xnode_type_t adv_node_type = common::xnode_type_t::evm_auditor;
    common::xnode_type_t val_node_type = common::xnode_type_t::evm_validator;
    common::xgroup_id_t adv_group_id = common::xauditor_group_id_begin;
    common::xgroup_id_t val_group_id = common::xvalidator_group_id_begin;
    auto election_result_store = serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, data::election::get_property_by_group_id(adv_group_id));
    auto & adv_election_group_result = election_result_store.result_of(m_network_id).result_of(adv_node_type).result_of(common::xdefault_cluster_id).result_of(adv_group_id);

    auto next_version = adv_election_group_result.group_version();
    if (!next_version.empty()) {
        next_version.increase();
    } else {
        next_version = common::xelection_round_t{0};
    }
    adv_election_group_result.group_version(next_version);
    adv_election_group_result.election_committee_version(common::xelection_round_t{0});
    adv_election_group_result.timestamp(current_time);
    adv_election_group_result.start_time(current_time);

    auto adv_group_node_infos = xstatic_election_center::instance().get_static_evm_consensus_election_nodes(adv_group_id.value());
    for (auto const & node : adv_group_node_infos) {
        xelection_info_t new_election_info{};
        new_election_info.joined_epoch(next_version);
        new_election_info.stake(node.stake);
        new_election_info.comprehensive_stake(node.stake);
        new_election_info.public_key(node.pub_key);
        new_election_info.genesis(false);
        new_election_info.miner_type(common::xminer_type_t::advance);

        xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.account_address(node.node_id);
        election_info_bundle.election_info(new_election_info);
        adv_election_group_result.insert(std::move(election_info_bundle));
    }

    auto & val_election_group_result =
        election_result_store.result_of(m_network_id).result_of(val_node_type).result_of(common::xdefault_cluster_id).result_of(val_group_id);

    next_version = val_election_group_result.group_version();
    if (!next_version.empty()) {
        next_version.increase();
    } else {
        next_version = common::xelection_round_t{0};
    }
    val_election_group_result.group_version(next_version);
    val_election_group_result.election_committee_version(common::xelection_round_t{0});
    val_election_group_result.timestamp(current_time);
    val_election_group_result.start_time(current_time);

    // set associated relation
    val_election_group_result.associated_group_id(adv_group_id);
    val_election_group_result.cluster_version(common::xelection_round_t{0});
    val_election_group_result.associated_group_version(adv_election_group_result.group_version());

    auto val_group_node_infos = xstatic_election_center::instance().get_static_evm_consensus_election_nodes(val_group_id.value());
    for (auto const & node : val_group_node_infos) {
        xelection_info_t new_election_info{};
        new_election_info.joined_epoch(next_version);
        new_election_info.stake(node.stake);
        new_election_info.comprehensive_stake(node.stake);
        new_election_info.public_key(node.pub_key);
        new_election_info.genesis(false);
        new_election_info.miner_type(common::xminer_type_t::validator);

        xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.account_address(node.node_id);
        election_info_bundle.election_info(new_election_info);
        val_election_group_result.insert(std::move(election_info_bundle));
    }

    serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, data::election::get_property_by_group_id(adv_group_id), election_result_store);
}
#endif

void xtop_zec_elect_eth_contract::setup() {
    data::election::v2::xelection_result_store_t election_result_store;
    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    for (auto const & property : property_names) {
        STRING_CREATE(property);
        serialization::xmsgpack_t<data::election::xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
    }
}

void xtop_zec_elect_eth_contract::on_timer(common::xlogic_time_t const current_time) {
#ifdef STATIC_CONSENSUS
    if (xstatic_election_center::instance().if_allow_elect()) {
        if (!executed_eth) {
            elect_config_nodes(current_time);
            return;
        }
#    ifndef ELECT_WHEREAFTER
        return;
#    endif
    } else {
        return;
    }
#endif
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().to_string(), "xzec_elect_eth_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(SELF_ADDRESS() == zec_elect_eth_contract_address, "xzec_elect_eth_contract_t instance is not triggled by zec_elect_eth_contract_address");
    // XCONTRACT_ENSURE(current_time <= TIME(), "xzec_elect_eth_contract_t::on_timer current_time > consensus leader's time");
    XCONTRACT_ENSURE(current_time + XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_election_interval) / 2 > TIME(),
                     "xzec_elect_eth_contract_t::on_timer retried too many times. TX generated time " + std::to_string(current_time) + " TIME() " + std::to_string(TIME()));

    std::uint64_t random_seed;
    try {
        auto seed = m_contract_helper->get_random_seed();
        random_seed = utl::xxh64_t::digest(seed);
    } catch (std::exception const & eh) {
        xwarn("[zec contract][on_timer] get random seed failed: %s", eh.what());
        return;
    }

    elect(common::xevm_zone_id, common::xdefault_cluster_id, random_seed, current_time);
}

void xtop_zec_elect_eth_contract::elect(common::xzone_id_t const zone_id,
                                        common::xcluster_id_t const cluster_id,
                                        std::uint64_t const random_seed,
                                        common::xlogic_time_t const election_timestamp) {
    uint64_t read_height =
        static_cast<std::uint64_t>(std::stoull(STRING_GET2(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT, sys_contract_zec_standby_pool_addr)));

    std::string result;

    GET_STRING_PROPERTY(data::XPROPERTY_CONTRACT_STANDBYS_KEY, result, read_height, sys_contract_rec_standby_pool_addr);
    if (result.empty()) {
        xwarn("[zec contract][elect_non_genesis] get rec_standby_pool_addr property fail, block height %" PRIu64, read_height);
        return;
    }
    xwarn("[zec contract][elect_non_genesis] elect zone %" PRIu16 " cluster %" PRIu16 " random nonce %" PRIu64 " logic time %" PRIu64 " read_height: %" PRIu64,
          static_cast<std::uint16_t>(zone_id.value()),
          static_cast<std::uint16_t>(cluster_id.value()),
          random_seed,
          election_timestamp,
          read_height);

    auto const & standby_result_store = codec::msgpack_decode<xstandby_result_store_t>({std::begin(result), std::end(result)});

    auto const standby_network_result = standby_result_store.result_of(network_id()).network_result();

    if (standby_network_result.empty()) {
        xwarn("[zec contract][elect_non_genesis] no standby nodes");
        return;
    }

    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    for (auto const & property : property_names) {
        auto election_result_store = serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, property);
        auto & election_network_result = election_result_store.result_of(network_id());

        auto const & current_group_nodes =
            election_network_result.result_of(common::xnode_type_t::evm_auditor).result_of(common::xdefault_cluster_id).result_of(common::xdefault_group_id);
        auto start_time = election_timestamp;
        if (!current_group_nodes.empty()) {
            start_time += XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_election_interval) / 2;
        }

        auto const successful = elect_eth_consensus(common::xevm_zone_id,
                                                    common::xdefault_cluster_id,
                                                    common::xauditor_group_id_begin,
                                                    common::xvalidator_group_id_begin,
                                                    election_timestamp,
                                                    start_time,
                                                    random_seed,
                                                    standby_network_result,
                                                    election_network_result);
        if (successful) {
            serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
            xwarn("[zec elect eth] successful. timestamp %" PRIu64 " start time %" PRIu64 " random seed %" PRIu64, election_timestamp, start_time, random_seed);
        }
    }
}

bool xtop_zec_elect_eth_contract::elect_eth_consensus(common::xzone_id_t const zone_id,
                                                      common::xcluster_id_t const cluster_id,
                                                      common::xgroup_id_t const auditor_group_id,
                                                      common::xgroup_id_t const validator_group_id,
                                                      common::xlogic_time_t const election_timestamp,
                                                      common::xlogic_time_t const start_time,
                                                      std::uint64_t const random_seed,
                                                      data::election::xstandby_network_result_t const & standby_network_result,
                                                      xelection_network_result_t & election_network_result) {
    auto const min_eth_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_eth_auditor_group_size);
    auto const max_eth_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_eth_auditor_group_size);

    // clean up the auditor standby pool by filtering out the nodes that are currently in the validator group.
    auto effective_standby_network_result = standby_network_result;
    auto & effective_auditor_standbys = effective_standby_network_result.result_of(common::xnode_type_t::evm_auditor);
    auto const & election_validator_group_nodes = election_network_result.result_of(common::xnode_type_t::evm_validator).result_of(cluster_id).result_of(validator_group_id);
    for (auto const & validator_node_info : election_validator_group_nodes) {
        auto const & validator_node_id = top::get<data::election::xelection_info_bundle_t>(validator_node_info).account_address();
        auto it = effective_auditor_standbys.find(validator_node_id);
        if (it != std::end(effective_auditor_standbys)) {
            xdbg("group %s kicks out associator validator %s from auditor standby pool", auditor_group_id.to_string().c_str(), validator_node_id.to_string().c_str());
            effective_auditor_standbys.erase(it);
        }
    }

    auto eth_auditor_result = elect_group(zone_id,
                                          cluster_id,
                                          auditor_group_id,
                                          election_timestamp,
                                          start_time,
                                          random_seed,
                                          xrange_t<config::xgroup_size_t>(min_eth_auditor_group_size, max_eth_auditor_group_size),
                                          effective_standby_network_result,
                                          election_network_result);

    auto const min_eth_validator_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_eth_validator_group_size);
    auto const max_eth_validator_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_eth_validator_group_size);

    // clean up the validator standby pool by filtering out the nodes that are currently in the auditor group.
    effective_standby_network_result = standby_network_result;
    auto & effective_validator_standbys = effective_standby_network_result.result_of(common::xnode_type_t::evm_validator);
    auto const & election_auditor_group_nodes = election_network_result.result_of(common::xnode_type_t::evm_auditor).result_of(cluster_id).result_of(auditor_group_id);
    for (auto const & auditor_node_info : election_auditor_group_nodes) {
        auto const & auditor_node_id = top::get<data::election::xelection_info_bundle_t>(auditor_node_info).account_address();
        auto it = effective_validator_standbys.find(auditor_node_id);
        if (it != std::end(effective_validator_standbys)) {
            xdbg("group %s kicks out associator auditor %s from auditor standby pool", auditor_group_id.to_string().c_str(), auditor_node_id.to_string().c_str());
            effective_validator_standbys.erase(it);
        }
    }

    auto eth_validator_result = elect_group(zone_id,
                                            cluster_id,
                                            validator_group_id,
                                            election_timestamp,
                                            start_time,
                                            random_seed,
                                            xrange_t<config::xgroup_size_t>(min_eth_validator_group_size, max_eth_validator_group_size),
                                            effective_standby_network_result,
                                            election_network_result);

    if (eth_auditor_result || eth_validator_result) {
        auto const & evm_auditor_group = election_network_result.result_of(common::xnode_type_t::evm_auditor).result_of(cluster_id).result_of(auditor_group_id);

        assert(evm_auditor_group.start_time() != common::xjudgement_day);
        assert(evm_auditor_group.start_time() == start_time);
        assert(evm_auditor_group.timestamp() == election_timestamp);

        xwarn("[zec elect eth] version %s size %zu timestamp %" PRIu64 " start time %" PRIu64,
              evm_auditor_group.group_version().to_string().c_str(),
              evm_auditor_group.size(),
              evm_auditor_group.timestamp(),
              evm_auditor_group.start_time());

        auto & evm_validator_group = election_network_result.result_of(common::xnode_type_t::evm_validator).result_of(cluster_id).result_of(validator_group_id);
        evm_validator_group.associated_group_id(auditor_group_id);
        evm_validator_group.associated_group_version(evm_auditor_group.group_version());

        assert(evm_validator_group.start_time() != common::xjudgement_day);
        assert(evm_validator_group.start_time() == start_time);
        assert(evm_validator_group.timestamp() == election_timestamp);

        xwarn("[zec elect eth] version %s size %zu timestamp %" PRIu64 " start time %" PRIu64,
              evm_validator_group.group_version().to_string().c_str(),
              evm_validator_group.size(),
              evm_validator_group.timestamp(),
              evm_validator_group.start_time());

        if (evm_auditor_group.size() && evm_validator_group.size()) {
            return true;
        }
    }
    return false;
}

NS_END4
