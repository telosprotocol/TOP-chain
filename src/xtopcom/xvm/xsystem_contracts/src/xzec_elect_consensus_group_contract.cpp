// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xzec/xzec_elect_consensus_group_contract.h"

#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelect_transaction.hpp"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xconfig/xpredefined_configurations.h"
#include "xutility/xhash.h"
#include "xvm/xserialization/xserialization.h"

#include <cassert>
#include <cinttypes>
#include <ratio>  // NOLINT

#ifdef STATIC_CONSENSUS
#    include "xvm/xsystem_contracts/xelection/xstatic_election_center.h"
#endif

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "ZecElectConsensus_"
#define XCONSENSUS_ELECT XSYSCONTRACT_MODULE XCONTRACT_PREFIX

using top::data::election::xelection_association_result_store_t;
using top::data::election::xelection_group_result_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_network_result_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xstandby_network_result_t;
using top::data::election::xstandby_node_info_t;
using top::data::election::xstandby_result_store_t;
using top::data::election::xstandby_result_t;

NS_BEG4(top, xvm, system_contracts, zec)

xtop_zec_elect_consensus_group_contract::xtop_zec_elect_consensus_group_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

#ifdef STATIC_CONSENSUS
// if enabled static_consensus
// make sure add config in config.xxxx.json
// like this :
//
// "consensus_start_nodes":
// "auditor:T00000LNi53Ub726HcPXZfC4z6zLgTo5ks6GzTUp.0.pub_key,T00000LeXNqW7mCCoj23LEsxEmNcWKs8m6kJH446.0.pub_key,T00000LVpL9XRtVdU5RwfnmrCtJhvQFxJ8TB46gB.0.pub_key|
// validator:T00000LXqp1NkfooMAw7Bty2iXTxgTCfsygMnxrT.0.pub_key,T00000LaFmRAybSKTKjE8UXyf7at2Wcw8iodkoZ8.0.pub_key,T00000LhCXUC5iQCREefnRPRFhxwDJTEbufi41EL.0.pub_key|
// auditor:T00000LLJ8AsN4hREDtCpuKAxJFwqka9LwiAon3M.0.pub_key,T00000LefzYnVUayJSgeX3XdKCgB4vk7BVUoqsum.0.pub_key,T00000LTSip8Xbjutrtm8RkQzsHKqt28g97xdUxg.0.pub_key|
// validator:T00000LcNfcqFPH9vy3EYApkrcXLcQN2hb1ygZWE.0.pub_key,T00000LUv7e8RZLNtnE1K9sEfE9SYe74rwYkzEub.0.pub_key,T00000LKfBYfwTcNniDSQqj8fj5atiDqP8ZEJJv6.0.pub_key",

// it will elect the first and only round consensus nodes as you want.
bool executed_consensus{false};

void xtop_zec_elect_consensus_group_contract::swap_election_result(common::xlogic_time_t const current_time) {
    auto auditor_group_count = XGET_CONFIG(auditor_group_count);
    auto validator_group_count = XGET_CONFIG(validator_group_count);
    assert(auditor_group_count == 2 && validator_group_count == 4);
    uint64_t latest_height = get_blockchain_height(sys_contract_zec_elect_consensus_addr);
    if (latest_height <= 0) {
        return;
    }
    uint8_t auditor_group_start_count = common::xauditor_group_id_begin.value();
    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    std::unordered_map<top::common::xgroup_id_t, top::data::election::xelection_result_store_t> all_cluster_election_result_store;
    for (auto const & property : property_names) {
        auto election_result_store = serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, property);
        all_cluster_election_result_store.insert({common::xgroup_id_t{auditor_group_start_count++}, election_result_store});
    }

    assert(all_cluster_election_result_store.size() == auditor_group_count);

    // hard code swap node:
    auto & sharding_1_result_store = all_cluster_election_result_store.at(common::xgroup_id_t{1});
    auto & sharding_2_result_store = all_cluster_election_result_store.at(common::xgroup_id_t{2});

    auto & adv_1 =
        sharding_1_result_store.result_of(m_network_id).result_of(common::xnode_type_t::consensus_auditor).result_of(common::xdefault_cluster_id).result_of(common::xgroup_id_t{1});
    auto & adv_2 =
        sharding_2_result_store.result_of(m_network_id).result_of(common::xnode_type_t::consensus_auditor).result_of(common::xdefault_cluster_id).result_of(common::xgroup_id_t{2});

    auto & con_64 = sharding_1_result_store.result_of(m_network_id)
                        .result_of(common::xnode_type_t::consensus_validator)
                        .result_of(common::xdefault_cluster_id)
                        .result_of(common::xgroup_id_t{64});
    auto & con_65 = sharding_1_result_store.result_of(m_network_id)
                        .result_of(common::xnode_type_t::consensus_validator)
                        .result_of(common::xdefault_cluster_id)
                        .result_of(common::xgroup_id_t{65});
    auto & con_66 = sharding_2_result_store.result_of(m_network_id)
                        .result_of(common::xnode_type_t::consensus_validator)
                        .result_of(common::xdefault_cluster_id)
                        .result_of(common::xgroup_id_t{66});
    auto & con_67 = sharding_2_result_store.result_of(m_network_id)
                        .result_of(common::xnode_type_t::consensus_validator)
                        .result_of(common::xdefault_cluster_id)
                        .result_of(common::xgroup_id_t{67});

#    define UPDATE_VERSION(group_result)                                                                                                                                           \
        group_result.group_version(group_result.group_version().empty() ? common::xelection_round_t{0} : (group_result.group_version().increase()));                               \
        group_result.election_committee_version(common::xelection_round_t{0});                                                                                                     \
        group_result.timestamp(current_time);                                                                                                                                      \
        group_result.start_time(current_time);

    UPDATE_VERSION(adv_1);
    UPDATE_VERSION(adv_2);
    UPDATE_VERSION(con_64);
    UPDATE_VERSION(con_65);
    UPDATE_VERSION(con_66);
    UPDATE_VERSION(con_67);

#    undef UPDATE_VERSION
    con_64.associated_group_version(adv_1.group_version());
    con_65.associated_group_version(adv_1.group_version());
    con_66.associated_group_version(adv_2.group_version());
    con_67.associated_group_version(adv_2.group_version());

    std::swap((*adv_1.begin()).second, (*adv_2.begin()).second);
    std::swap((*con_64.begin()).second, (*con_66.begin()).second);
    std::swap((*con_65.begin()).second, (*con_67.begin()).second);

    adv_1.begin()->second.election_info().joined_version = adv_1.group_version();
    adv_2.begin()->second.election_info().joined_version = adv_2.group_version();
    con_64.begin()->second.election_info().joined_version = con_64.group_version();
    con_65.begin()->second.election_info().joined_version = con_65.group_version();
    con_66.begin()->second.election_info().joined_version = con_66.group_version();
    con_67.begin()->second.election_info().joined_version = con_67.group_version();

    auditor_group_start_count = common::xauditor_group_id_begin.value();
    for (auto const & property : property_names) {
        serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(
            *this, property, all_cluster_election_result_store.at(common::xgroup_id_t{auditor_group_start_count++}));
    }
}
void xtop_zec_elect_consensus_group_contract::elect_config_nodes(common::xlogic_time_t const current_time) {
    uint64_t latest_height = get_blockchain_height(sys_contract_zec_elect_consensus_addr);
    xinfo("[consensus_start_nodes] get_latest_height: %" PRIu64, latest_height);
    if (latest_height > 0) {
        executed_consensus = true;
        return;
    }
    using top::data::election::xelection_info_bundle_t;
    using top::data::election::xelection_info_t;
    using top::data::election::xelection_result_store_t;
    using top::data::election::xstandby_node_info_t;

    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());

    auto auditor_group_count = XGET_CONFIG(auditor_group_count);
    auto validator_group_count = XGET_CONFIG(validator_group_count);
    auto cluster_group_num = 1 + validator_group_count / auditor_group_count;
    auto const pre_associate = XGET_CONFIG(validator_group_count) / XGET_CONFIG(auditor_group_count);

    uint8_t auditor_group_start_count = common::xauditor_group_id_begin.value();
    uint8_t validator_group_start_count = common::xvalidator_group_id_begin.value();

    // for association
    uint8_t auditor_group_id_start = common::xauditor_group_id_begin.value();
    uint8_t validator_group_id_start = common::xvalidator_group_id_begin.value();

    for (auto const & property : property_names) {
        auto election_result_store = serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, property);

        // auditor:
        common::xnode_type_t adv_node_type = common::xnode_type_t::consensus_auditor;
        common::xgroup_id_t adv_group_id = common::xgroup_id_t{auditor_group_start_count++};
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

        // elect in:
        auto adv_group_node_infos = xstatic_election_center::instance().get_static_consensus_election_nodes(adv_group_id.value());
        for (auto node : adv_group_node_infos) {
            xelection_info_t new_election_info{};
            new_election_info.joined_version = next_version;
            new_election_info.stake = node.stake;
            new_election_info.comprehensive_stake = node.stake;
            new_election_info.consensus_public_key = node.pub_key;

            xelection_info_bundle_t election_info_bundle{};
            election_info_bundle.node_id(node.node_id);
            election_info_bundle.election_info(new_election_info);
            adv_election_group_result.insert(std::move(election_info_bundle));
        }

        for (auto _index = 0; _index < pre_associate; ++_index) {
            common::xnode_type_t val_node_type = common::xnode_type_t::consensus_validator;
            common::xgroup_id_t val_group_id = common::xgroup_id_t{validator_group_start_count++};
            auto & val_election_group_result =
                election_result_store.result_of(m_network_id).result_of(val_node_type).result_of(common::xdefault_cluster_id).result_of(val_group_id);

            auto next_version = val_election_group_result.group_version();
            if (!next_version.empty()) {
                next_version.increase();
            } else {
                next_version = common::xelection_round_t{0};
            }
            val_election_group_result.group_version(next_version);
            val_election_group_result.election_committee_version(common::xelection_round_t{0});
            val_election_group_result.timestamp(current_time);
            val_election_group_result.start_time(current_time);

            auto val_group_node_infos = xstatic_election_center::instance().get_static_consensus_election_nodes(val_group_id.value());
            for (auto node : val_group_node_infos) {
                xelection_info_t new_election_info{};
                new_election_info.joined_version = next_version;
                new_election_info.stake = node.stake;
                new_election_info.comprehensive_stake = node.stake;
                new_election_info.consensus_public_key = node.pub_key;

                xelection_info_bundle_t election_info_bundle{};
                election_info_bundle.node_id(node.node_id);
                election_info_bundle.election_info(new_election_info);
                val_election_group_result.insert(std::move(election_info_bundle));
            }
        }

        auto const & auditor_group = election_result_store.result_of(m_network_id)
                                         .result_of(common::xnode_type_t::consensus_auditor)
                                         .result_of(common::xdefault_cluster_id)
                                         .result_of(common::xgroup_id_t{auditor_group_id_start});

        for (auto validator_index = 0; validator_index < pre_associate; validator_index++) {
            auto & validator_group = election_result_store.result_of(m_network_id)
                                         .result_of(common::xnode_type_t::consensus_validator)
                                         .result_of(common::xdefault_cluster_id)
                                         .result_of(common::xgroup_id_t{validator_group_id_start++});
            validator_group.associated_group_id(common::xgroup_id_t{auditor_group_id_start});
            validator_group.cluster_version(common::xelection_round_t{0});
            validator_group.associated_group_version(auditor_group.group_version());
        }
        auditor_group_id_start++;

        serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
    }
}
#endif

void xtop_zec_elect_consensus_group_contract::setup() {
    xelection_result_store_t election_result_store;
    auto property_names = data::election::get_property_name_by_addr(SELF_ADDRESS());
    for (auto const & property : property_names) {
        STRING_CREATE(property);
        serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, property, election_result_store);
    }

    STRING_CREATE(data::XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY);
    std::string election_executed;
    STRING_SET(data::XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY, election_executed);
}

void xtop_zec_elect_consensus_group_contract::on_timer(common::xlogic_time_t const current_time) {
    XMETRICS_TIME_RECORD(XCONSENSUS_ELECT "on_timer_all_time");
    XMETRICS_CPU_TIME_RECORD(XCONSENSUS_ELECT "on_timer_cpu_time");
#ifdef STATIC_CONSENSUS
    if (xstatic_election_center::instance().if_allow_elect()) {
        if (!executed_consensus) {
            elect_config_nodes(current_time);
            return;
        }
#    ifndef ELECT_WHEREAFTER
        // only once static_consensus elect. return then.
        return;
#    endif
#    ifdef CONSENSUS_SWAP
        swap_election_result(current_time);
        return;
#    endif
    } else {
        return;
    }
#endif
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().value(), "xzec_elect_consensus_group_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(SELF_ADDRESS().value() == sys_contract_zec_elect_consensus_addr,
                     "xzec_elect_consensus_group_contract_t instance is not triggled by sys_contract_zec_elect_consensus_addr");
    // XCONTRACT_ENSURE(current_time <= TIME(), "xzec_elect_consensus_group_contract_t::on_timer current_time > consensus leader's time");
    XCONTRACT_ENSURE(current_time + XGET_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval) / 2 > TIME(),
                     "xzec_elect_consensus_group_contract_t::on_timer retried too many times. TX generated time " + std::to_string(current_time) + " TIME() " + std::to_string(TIME()) + " election interval " + std::to_string(XGET_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval)));

    std::uint64_t random_seed;
    try {
        auto seed = m_contract_helper->get_random_seed();
        random_seed = utl::xxh64_t::digest(seed);
    } catch (std::exception const & eh) {
        xwarn("[zec contract][on_timer] get random seed failed: %s", eh.what());
        return;
    }

    elect(common::xconsensus_zone_id, common::xdefault_cluster_id, random_seed, current_time);
}

void xtop_zec_elect_consensus_group_contract::elect(common::xzone_id_t const zone_id,
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
#if 0
    int32_t ret = 0;
    do {
        base::xauto_ptr<xblock_t> block_ptr = get_block_by_height(sys_contract_rec_standby_pool_addr, read_height);
        if (block_ptr == nullptr) {
            xwarn("[zec contract][elect_non_genesis] get nullptr, block height %" PRIu64, read_height);
            return;
        }
        ret = block_ptr->get_native_property().native_string_get(data::XPROPERTY_CONTRACT_STANDBYS_KEY, result);
        xdbg("[zec contract][elect_non_genesis] see block_height % " PRIu64 " ,result size: %zu", read_height, result.size());
        if ((ret || result.empty()) && read_height > 0) {
            read_height--;
        } else {
            break;
        }
    } while (read_height);
#endif
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

    auto const & election_association_result_store = serialization::xmsgpack_t<xelection_association_result_store_t>::deserialize_from_string_prop(
        *this, sys_contract_zec_group_assoc_addr, data::XPROPERTY_CONTRACT_GROUP_ASSOC_KEY);
    if (election_association_result_store.empty()) {
        xerror("[zec contract][elect_non_genesis] no association info");
        return;
    }

#if defined DEBUG

    //std::string log;
    //for (auto const & standby_result : standby_network_result.results()) {
    //    log += " " + common::to_string(standby_result.first) + ": ";
    //    for (auto const & result : standby_result.second.results()) {
    //        log += result.first.to_string() + "|";
    //    }
    //}

    //xdbg("[zec contract][elect_non_genesis] elect sees standbys %s", log.c_str());

#endif

    auto const validator_group_count = XGET_CONFIG(validator_group_count);
    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);

    auto cluster_election_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_interval);
    XATTRIBUTE_MAYBE_UNUSED auto zone_election_trigger_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(zone_election_trigger_interval);

    // auto const max_auditor_rotation_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_rotation_count);

    auto const actual_auditor_rotation_num = genesis_elected() ? XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_rotation_count) : auditor_group_count;

    assert(zone_election_trigger_interval < cluster_election_interval);

    auto const total_group_count = auditor_group_count + validator_group_count;

    uint16_t auditor_rotation_num{0};
    std::unordered_map<common::xgroup_id_t, data::election::xelection_result_store_t> all_cluster_election_result_store;
    for (auto index = 0; index < auditor_group_count; ++index) {
        top::common::xgroup_id_t auditor_gid{static_cast<top::common::xgroup_id_t::value_type>(common::xauditor_group_id_value_begin + index)};
        xdbg("[zec contract][elect_non_genesis] index: %d ,auditor_gid: %s,insert %s",
             index,
             auditor_gid.to_string().c_str(),
             data::election::get_property_by_group_id(auditor_gid).c_str());
        auto election_result_store =
            serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, data::election::get_property_by_group_id(auditor_gid));

        all_cluster_election_result_store.insert({auditor_gid, election_result_store});
    }

    for (uint16_t index = 0u; (index < auditor_group_count) && (auditor_rotation_num < actual_auditor_rotation_num); ++index) {
        common::xgroup_id_t auditor_group_id{static_cast<common::xgroup_id_t::value_type>(common::xauditor_group_id_value_begin + index % auditor_group_count)};
        assert(auditor_group_id >= common::xauditor_group_id_begin && auditor_group_id < common::xauditor_group_id_end);

        auto & election_result_store = all_cluster_election_result_store.at(auditor_group_id);
        auto & election_network_result = election_result_store.result_of(network_id());

        auto const & auditor_group = election_network_result.result_of(common::xnode_type_t::consensus_auditor).result_of(cluster_id).result_of(auditor_group_id);

        // since empty = true when genesis elect , it will elect all auditor group.
        auto rotation_flag = auditor_group.empty();
        if (rotation_flag == false) {
            int const election_interval_wave_range = total_group_count + index;
            int const election_interval_wave_offset = random_seed % election_interval_wave_range - election_interval_wave_range / 2;
            auto const rotation_time = election_interval_wave_offset + cluster_election_interval;
            xdbg("[zec contract][elect_non_genesis] zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": rotation interval %d",
                 static_cast<std::uint16_t>(zone_id.value()),
                 static_cast<std::uint16_t>(cluster_id.value()),
                 static_cast<std::uint16_t>(auditor_group_id.value()),
                 static_cast<int>(rotation_time));

            rotation_flag = (election_timestamp - auditor_group.timestamp()) >= rotation_time;
        }
        if (!rotation_flag) {
            xdbg("[zec contract][elect_non_genesis] zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": not this rotation, pass ",
                 static_cast<std::uint16_t>(zone_id.value()),
                 static_cast<std::uint16_t>(cluster_id.value()),
                 static_cast<std::uint16_t>(auditor_group_id.value()));
            continue;
        }

        if (elect_auditor_validator(zone_id,
                                    cluster_id,
                                    auditor_group_id,
                                    random_seed,
                                    election_timestamp,
                                    genesis_elected() ? election_timestamp + cluster_election_interval / 2 : election_timestamp,
                                    election_association_result_store,
                                    standby_network_result,
                                    all_cluster_election_result_store)) {
            ++auditor_rotation_num;
            xwarn("[zec contract][elect_non_genesis] elect zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " : success. read_height: %" PRIu64,
                  static_cast<std::uint16_t>(zone_id.value()),
                  static_cast<std::uint16_t>(cluster_id.value()),
                  static_cast<std::uint16_t>(auditor_group_id.value()),
                  read_height);

            serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(*this, data::election::get_property_by_group_id(auditor_group_id), election_result_store);
#if defined DEBUG
            auto serialized_election_result_store_data = codec::msgpack_encode(election_result_store);
            auto base64str =
                base::xstring_utl::base64_encode(serialized_election_result_store_data.data(), static_cast<std::uint32_t>(serialized_election_result_store_data.size()));
            xdbg("[zec contract][elect] election result hash value %" PRIx64, utl::xxh64_t::digest(base64str));
#endif

        } else {
            xwarn("[zec contract][elect_non_genesis] elect zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 " : failed. read_height: %" PRIu64,
                  static_cast<std::uint16_t>(zone_id.value()),
                  static_cast<std::uint16_t>(cluster_id.value()),
                  static_cast<std::uint16_t>(auditor_group_id.value()),
                  read_height);
        }
    }
    if (!genesis_elected()) {
        xinfo("[zec contract] zone %" PRIu16 " cluster %" PRIu16 " check genesis election",
              static_cast<std::uint16_t>(zone_id.value()),
              static_cast<std::uint16_t>(cluster_id.value()));
        auto const & election_association_result = election_association_result_store.result_of(cluster_id);
        for (auto const & p : election_association_result) {
            auto const & validator_group_id = p.first;
            auto const & auditor_group_id = p.second;

            auto const & election_result_store = all_cluster_election_result_store.at(auditor_group_id);
            auto const & election_network_result = election_result_store.result_of(network_id());
            if (election_network_result.result_of(common::xnode_type_t::consensus_auditor).result_of(cluster_id).result_of(auditor_group_id).empty() ||
                election_network_result.result_of(common::xnode_type_t::consensus_validator).result_of(cluster_id).result_of(validator_group_id).empty()) {
                xerror("genesis_election failed auditor %" PRIu16 " validator %" PRIu16,
                       static_cast<std::uint16_t>(auditor_group_id.value()),
                       static_cast<std::uint16_t>(validator_group_id.value()));
                std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
                top::error::throw_error(ec);
            }
        }
        STRING_SET(data::XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY, "1");
    }
}

bool xtop_zec_elect_consensus_group_contract::elect_auditor_validator(common::xzone_id_t const & zone_id,
                                                                      common::xcluster_id_t const & cluster_id,
                                                                      common::xgroup_id_t const & auditor_group_id,
                                                                      std::uint64_t const random_seed,
                                                                      common::xlogic_time_t const election_timestamp,
                                                                      common::xlogic_time_t const start_time,
                                                                      data::election::xelection_association_result_store_t const & association_result_store,
                                                                      data::election::xstandby_network_result_t const & standby_network_result,
                                                                      std::unordered_map<common::xgroup_id_t, data::election::xelection_result_store_t> & all_cluster_election_result_store) {
    std::string log_prefix = "[zec contract][elect_auditor_validator] zone " + zone_id.to_string() + u8" cluster " + cluster_id.to_string() + " group " + auditor_group_id.to_string();
    bool election_success{false};
    std::string election_result_log;

    std::vector<common::xgroup_id_t> associated_validator_group_ids;
    associated_validator_group_ids.reserve(8);
    auto const & association_cluster_result = association_result_store.result_of(cluster_id);
    for (auto const & assoc_result : association_cluster_result) {
        auto const & auditor_gid = assoc_result.second;
        if (auditor_gid == auditor_group_id) {
            associated_validator_group_ids.push_back(assoc_result.first);
        }
    }

    XCONTRACT_ENSURE(!associated_validator_group_ids.empty(), u8"associated validator group id empty");
    assert(!associated_validator_group_ids.empty());

    // clean up the auditor standby pool by filtering out the nodes that are currently in the validator group.
    auto effective_standby_network_result = standby_network_result;
    auto & effective_auditor_standbys = effective_standby_network_result.result_of(common::xnode_type_t::consensus_auditor);
    auto & election_network_result = all_cluster_election_result_store.at(auditor_group_id).result_of(network_id());
    xwarn("%s begins to filter auditor standbys (standby size %zu)", log_prefix.c_str(), effective_auditor_standbys.size());
    for (auto const & assoc_validator_group_id : associated_validator_group_ids) {
        auto const & assoc_validator_group_nodes = election_network_result.result_of(common::xnode_type_t::consensus_validator)
                                                                          .result_of(cluster_id)
                                                                          .result_of(assoc_validator_group_id);
        for (auto const & validator_node_info : assoc_validator_group_nodes) {
            auto const & validator_node_id = top::get<data::election::xelection_info_bundle_t>(validator_node_info).node_id();

            auto it = effective_auditor_standbys.find(validator_node_id);
            if (it != std::end(effective_auditor_standbys)) {
                xdbg("group %s kicks out associator validator %s from auditor standby pool", auditor_group_id.to_string().c_str(), validator_node_id.value().c_str());
                effective_auditor_standbys.erase(it);
            }
        }
    }

    std::vector<common::xgroup_id_t> all_auditor_group_id(XGET_CONFIG(auditor_group_count));
    std::iota(all_auditor_group_id.begin(), all_auditor_group_id.end(), common::xauditor_group_id_begin);
    xdbg("all_auditor_group_id.size():%zu begin:%s end:%s",
         all_auditor_group_id.size(),
         all_auditor_group_id.begin()->to_string().c_str(),
         (all_auditor_group_id.end() - 1)->to_string().c_str());
    for (auto const & other_auditor_group_id : all_auditor_group_id) {
        if (other_auditor_group_id == auditor_group_id) {
            continue;
        }
        auto const & other_auditor_group_election_result_store = all_cluster_election_result_store.at(other_auditor_group_id);
        if (other_auditor_group_election_result_store.empty())
            continue;
        auto const & other_auditor_group_nodes = other_auditor_group_election_result_store.result_of(network_id())
                                                                                          .result_of(common::xnode_type_t::consensus_auditor)
                                                                                          .result_of(cluster_id)
                                                                                          .result_of(other_auditor_group_id);
        for (auto const & auditor_node_info : other_auditor_group_nodes) {
            auto const & auditor_node_id = top::get<data::election::xelection_info_bundle_t>(auditor_node_info).node_id();

            auto it = effective_auditor_standbys.find(auditor_node_id);
            if (it != std::end(effective_auditor_standbys)) {
                xdbg("group %s kicks out other group %s auditor %s from auditor standby pool",
                     auditor_group_id.to_string().c_str(),
                     other_auditor_group_id.to_string().c_str(),
                     auditor_node_id.value().c_str());
                effective_auditor_standbys.erase(it);
            }
        }
    }

    xwarn("%s stops to filter auditor standbys (standby size %zu)", log_prefix.c_str(), effective_auditor_standbys.size());

    if (!elect_auditor(zone_id, cluster_id, auditor_group_id, election_timestamp, start_time, random_seed, effective_standby_network_result, election_network_result)) {
        xwarn("%s election failed at logic time %" PRIu64 " and random nonce %" PRIu64,
              log_prefix.c_str(),
              static_cast<std::uint16_t>(zone_id.value()),
              static_cast<std::uint16_t>(cluster_id.value()),
              static_cast<std::uint16_t>(auditor_group_id.value()),
              election_timestamp,
              random_seed);
        election_result_log += "auditor_group " + to_string(auditor_group_id) + " unchanged,";
    } else {
        election_success = true;
        election_result_log += "auditor_group " + to_string(auditor_group_id) + " changed,";
    }

    std::vector<common::xgroup_id_t> all_validator_group_id(XGET_CONFIG(validator_group_count));
    std::iota(all_validator_group_id.begin(), all_validator_group_id.end(), common::xvalidator_group_id_begin);
    xdbg("all_validator_group_id.size():%zu begin:%s end:%s",
         all_validator_group_id.size(),
         all_validator_group_id.begin()->to_string().c_str(),
         (all_validator_group_id.end() - 1)->to_string().c_str());

    for (auto const & validator_group_id : associated_validator_group_ids) {
        auto effective_standby_network_result = standby_network_result;
        auto & effective_validator_standbys = effective_standby_network_result.result_of(common::xnode_type_t::consensus_validator);

        xwarn("%s begins to filter validator standbys (standby size %zu)", log_prefix.c_str(), effective_validator_standbys.size());
        // for electing validator, filter the standby nodes whose account (node id) is already in the associated auditor
        auto const & auditor_group_nodes = election_network_result.result_of(common::xnode_type_t::consensus_auditor).result_of(cluster_id).result_of(auditor_group_id);

        for (auto const & auditor_node_info : auditor_group_nodes) {
            auto const & auditor_node_id = top::get<data::election::xelection_info_bundle_t>(auditor_node_info).node_id();

            auto it = effective_validator_standbys.find(auditor_node_id);
            if (it != std::end(effective_validator_standbys)) {
                xdbg("group %s kicks out associated auditor %s from validator standby pool", validator_group_id.to_string().c_str(), auditor_node_id.value().c_str());
                effective_validator_standbys.erase(it);
            }
        }

        for (auto const & other_validator_group_id : all_validator_group_id) {
            if (other_validator_group_id == validator_group_id) {
                continue;
            }
            auto & related_auditor_group_id = association_result_store.result_of(cluster_id).result_of(other_validator_group_id);
            auto const & other_auditor_group_election_result_store = all_cluster_election_result_store.at(related_auditor_group_id);
            if (other_auditor_group_election_result_store.empty())
                continue;
            auto const & other_auditor_group_nodes = other_auditor_group_election_result_store.result_of(network_id())
                                                         .result_of(common::xnode_type_t::consensus_validator)
                                                         .result_of(cluster_id)
                                                         .result_of(other_validator_group_id);
            for (auto const & validator_node_info : other_auditor_group_nodes) {
                auto const & validator_node_id = top::get<data::election::xelection_info_bundle_t>(validator_node_info).node_id();

                auto it = effective_validator_standbys.find(validator_node_id);
                if (it != std::end(effective_validator_standbys)) {
                    xdbg("group %s kicks out other group %s validator from validator standby pool %s",
                         validator_group_id.to_string().c_str(),
                         other_validator_group_id.to_string().c_str(),
                         validator_node_id.value().c_str());
                    effective_validator_standbys.erase(it);
                }
            }
        }

        xwarn("%s stops to filter validator standbys (standby size %zu)", log_prefix.c_str(), effective_validator_standbys.size());

        if (!elect_validator(zone_id,
                             cluster_id,
                             auditor_group_id,
                             validator_group_id,
                             election_timestamp,
                             start_time,
                             random_seed,
                             effective_standby_network_result,
                             election_network_result)) {
            xwarn("%s election is not executed under auditor group %" PRIu16, log_prefix.c_str(), static_cast<std::uint16_t>(auditor_group_id.value()));

            election_result_log += "validator_group " + to_string(validator_group_id) + " unchanged,";
        } else {
            election_success = true;
            election_result_log += "validator_group " + to_string(validator_group_id) + " changed,";
        }
    }

    auto const & auditor_group = election_network_result.result_of(common::xnode_type_t::consensus_auditor).result_of(cluster_id).result_of(auditor_group_id);
    if (election_success) {
        assert(auditor_group.start_time() != common::xjudgement_day);
        assert(auditor_group.start_time() == start_time);
        assert(auditor_group.timestamp() == election_timestamp);

        xwarn("[zec contract][election_group_result] version %s size %zu timestamp %" PRIu64 " start time %" PRIu64,
              auditor_group.group_version().to_string().c_str(),
              auditor_group.size(),
              auditor_group.timestamp(),
              auditor_group.start_time());

        for (auto const & validator_group_id : associated_validator_group_ids) {
            auto & validator_group = election_network_result.result_of(common::xnode_type_t::consensus_validator).result_of(cluster_id).result_of(validator_group_id);
            validator_group.associated_group_id(auditor_group_id);
            validator_group.cluster_version(association_cluster_result.cluster_version());
            validator_group.associated_group_version(auditor_group.group_version());

            assert(validator_group.start_time() != common::xjudgement_day);
            assert(validator_group.start_time() == start_time);
            assert(validator_group.timestamp() == election_timestamp);

            xwarn("[zec contract][election_group_result] version %s size %zu timestamp %" PRIu64 " start time %" PRIu64,
                  validator_group.group_version().to_string().c_str(),
                  validator_group.size(),
                  validator_group.timestamp(),
                  validator_group.start_time());
        }
    }

    xwarn("[zec contract][election_group_result] in version %s:%s", auditor_group.group_version().to_string().c_str(), election_result_log.c_str());
    return election_success;
}

bool xtop_zec_elect_consensus_group_contract::elect_auditor(common::xzone_id_t const & zid,
                                                            common::xcluster_id_t const & cid,
                                                            common::xgroup_id_t const & gid,
                                                            common::xlogic_time_t const election_timestamp,
                                                            common::xlogic_time_t const start_time,
                                                            std::uint64_t const random_seed,
                                                            xstandby_network_result_t const & standby_network_result,
                                                            xelection_network_result_t & election_network_result) {
    auto const min_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_auditor_group_size);
    auto const max_auditor_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_auditor_group_size);

    xdbg("[zec contract] zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": group size [%" PRIu16 ", %" PRIu16 "]",
         static_cast<std::uint16_t>(zid.value()),
         static_cast<std::uint16_t>(cid.value()),
         static_cast<std::uint16_t>(gid.value()),
         static_cast<std::uint16_t>(min_auditor_group_size),
         static_cast<std::uint16_t>(max_auditor_group_size));

    if (max_auditor_group_size < min_auditor_group_size) {
        assert(false);
        xwarn("[zec contract] zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": group size [%" PRIu16 ", %" PRIu16 "]",
              static_cast<std::uint16_t>(zid.value()),
              static_cast<std::uint16_t>(cid.value()),
              static_cast<std::uint16_t>(gid.value()),
              static_cast<std::uint16_t>(min_auditor_group_size),
              static_cast<std::uint16_t>(max_auditor_group_size));

        return false;
    }

    return elect_group(zid,
                       cid,
                       gid,
                       election_timestamp,
                       start_time,
                       random_seed,
                       xrange_t<config::xgroup_size_t>{min_auditor_group_size, max_auditor_group_size},
                       standby_network_result,
                       election_network_result);
}

bool xtop_zec_elect_consensus_group_contract::elect_validator(common::xzone_id_t const & zid,
                                                              common::xcluster_id_t const & cid,
                                                              common::xgroup_id_t const & auditor_gid,
                                                              common::xgroup_id_t const & validator_gid,
                                                              common::xlogic_time_t const election_timestamp,
                                                              common::xlogic_time_t const start_time,
                                                              std::uint64_t const random_seed,
                                                              xstandby_network_result_t const & standby_network_result,
                                                              xelection_network_result_t & election_network_result) {
    auto const min_validator_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_validator_group_size);
    auto const max_validator_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_validator_group_size);

    xdbg("[zec contract] zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": group size [%" PRIu16 ", %" PRIu16 "]",
         static_cast<std::uint16_t>(zid.value()),
         static_cast<std::uint16_t>(cid.value()),
         static_cast<std::uint16_t>(validator_gid.value()),
         static_cast<std::uint16_t>(min_validator_group_size),
         static_cast<std::uint16_t>(max_validator_group_size));

    if (max_validator_group_size < min_validator_group_size) {
        assert(false);
        xwarn("[zec contract] zone %" PRIu16 " cluster %" PRIu16 " group %" PRIu16 ": group size [%" PRIu16 ", %" PRIu16 "]",
              static_cast<std::uint16_t>(zid.value()),
              static_cast<std::uint16_t>(cid.value()),
              static_cast<std::uint16_t>(validator_gid.value()),
              static_cast<std::uint16_t>(min_validator_group_size),
              static_cast<std::uint16_t>(max_validator_group_size));
        return false;
    }

    return elect_group(zid,
                       cid,
                       validator_gid,
                       election_timestamp,
                       start_time,
                       random_seed,
                       xrange_t<config::xgroup_size_t>{min_validator_group_size, max_validator_group_size},
                       standby_network_result,
                       election_network_result);
}

bool xtop_zec_elect_consensus_group_contract::genesis_elected() const {
    auto const election_executed = STRING_GET(data::XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY);
    return !election_executed.empty();
}

NS_END4
