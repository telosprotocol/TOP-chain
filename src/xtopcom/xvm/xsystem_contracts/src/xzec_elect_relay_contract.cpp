// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xzec/xzec_elect_relay_contract.h"

#include "xchain_fork/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrootblock.h"
#include "xvm/xserialization/xserialization.h"

#include <cinttypes>

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "ZecElectRelay_"
#define XRELAY_ELECT XSYSCONTRACT_MODULE XCONTRACT_PREFIX

NS_BEG4(top, xvm, system_contracts, zec)

using data::election::xelection_result_store_t;
using data::election::xstandby_result_store_t;

xtop_zec_elect_relay_contract::xtop_zec_elect_relay_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xtop_zec_elect_relay_contract::setup() {
    data::election::v2::xelection_result_store_t election_result_store;
    common::xelection_round_t group_version{0};

    auto & current_group_nodes =
        election_result_store.result_of(network_id()).result_of(common::xnode_type_t::relay).result_of(common::xdefault_cluster_id).result_of(common::xdefault_group_id);
    current_group_nodes.election_committee_version(common::xelection_round_t{0});
    current_group_nodes.timestamp(0);
    current_group_nodes.start_time(0);
    current_group_nodes.group_version(group_version);

    auto const max_relay_group_size = config::xmax_relay_group_size_onchain_goverance_parameter_t::value;

    const std::vector<data::node_info_t> & seeds = data::xrootblock_t::get_seed_nodes();
    for (auto i = 0u; i < seeds.size() && i < max_relay_group_size; ++i) {
        auto const & item = seeds[i];

        common::xnode_id_t node_id{item.m_account};

        data::election::v2::xelection_info_t election_info{};
        election_info.joined_epoch(group_version);
        election_info.stake(std::numeric_limits<uint64_t>::max());
        election_info.public_key(xpublic_key_t{item.m_publickey});
        election_info.genesis(true);
        election_info.miner_type(static_cast<common::xminer_type_t>(std::numeric_limits<uint32_t>::max()));
        election_info.raw_credit_score(std::numeric_limits<uint64_t>::max());

        data::election::v2::xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.account_address(node_id);
        election_info_bundle.election_info(std::move(election_info));

        current_group_nodes.insert(std::move(election_info_bundle));
    }

    STRING_CREATE(data::election::get_property_by_group_id(common::xdefault_group_id));
    serialization::xmsgpack_t<data::election::v2::xelection_result_store_t>::serialize_to_string_prop(
        *this, data::election::get_property_by_group_id(common::xdefault_group_id), election_result_store);
}

void xtop_zec_elect_relay_contract::on_timer(common::xlogic_time_t const current_time) {
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().to_string(), "xtop_zec_elect_relay_contract instance is triggled by " + SOURCE_ADDRESS());
    XCONTRACT_ENSURE(SELF_ADDRESS() == zec_elect_relay_contract_address, "xtop_zec_elect_relay_contract instance is not triggled by zec_elect_relay_contract_address");
    // XCONTRACT_ENSURE(current_time <= TIME(), u8"xtop_zec_elect_relay_contract::on_timer current_time > consensus leader's time");
    XCONTRACT_ENSURE(current_time + XGET_ONCHAIN_GOVERNANCE_PARAMETER(relay_election_interval) / 2 > TIME(),
                     "xzec_elect_relay_contract_t::on_timer retried too many times. TX generated time " + std::to_string(current_time) + " TIME() " + std::to_string(TIME()));

    std::uint64_t random_seed;
    try {
        auto seed = m_contract_helper->get_random_seed();
        random_seed = utl::xxh64_t::digest(seed);
    } catch (std::exception const & eh) {
        xwarn("[zec contract][relay election][on_timer] get random seed failed: %s", eh.what());
        return;
    }
    xinfo("[zec contract][relay election] on_timer random seed %" PRIu64, random_seed);

    auto const relay_election_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(relay_election_interval);
    auto const min_relay_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_relay_group_size);
    auto const max_relay_group_size = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_relay_group_size);

    uint64_t read_height =
        static_cast<std::uint64_t>(std::stoull(STRING_GET2(data::XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT, sys_contract_zec_standby_pool_addr)));

    std::string result;

    GET_STRING_PROPERTY(data::XPROPERTY_CONTRACT_STANDBYS_KEY, result, read_height, sys_contract_rec_standby_pool_addr);
    if (result.empty()) {
        xwarn("[zec contract][relay election] get rec_standby_pool_addr property fail, block height %" PRIu64, read_height);
        return;
    }

    xwarn("[zec contract][relay election] elect zone %" PRIu16 " cluster %" PRIu16 " random nonce %" PRIu64 " logic time %" PRIu64 " read_height: %" PRIu64,
          static_cast<std::uint16_t>(common::xrelay_zone_id.value()),
          static_cast<std::uint16_t>(common::xdefault_cluster_id.value()),
          random_seed,
          current_time,
          read_height);

    auto const & standby_result_store = codec::msgpack_decode<xstandby_result_store_t>({std::begin(result), std::end(result)});

    auto const standby_network_result = standby_result_store.result_of(network_id()).network_result();

    if (standby_network_result.empty()) {
        xwarn("[zec contract][relay election] no standby nodes");
        return;
    }

    auto election_result_store =
        serialization::xmsgpack_t<xelection_result_store_t>::deserialize_from_string_prop(*this, data::election::get_property_by_group_id(common::xdefault_group_id));
    auto & election_network_result = election_result_store.result_of(network_id());

    auto const successful = elect_group(common::xrelay_zone_id,
                                        common::xdefault_cluster_id,
                                        common::xdefault_group_id,
                                        current_time,
                                        current_time + relay_election_interval / 2,
                                        random_seed,
                                        xrange_t<config::xgroup_size_t>{min_relay_group_size, max_relay_group_size},
                                        standby_network_result,
                                        election_network_result);
    if (successful) {
        serialization::xmsgpack_t<xelection_result_store_t>::serialize_to_string_prop(
            *this, data::election::get_property_by_group_id(common::xdefault_group_id), election_result_store);
    }
}

NS_END4
