// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xrelay/xrelay_process_election_data_contract.h"

#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xrootblock.h"
#include "xvm/xserialization/xserialization.h"

#include "xbasic/xutility.h"

NS_BEG4(top, xvm, system_contracts, relay)

xtop_relay_process_election_data_contract::xtop_relay_process_election_data_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

// same impl as zec_elect_relay setup
void xtop_relay_process_election_data_contract::setup() {
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
        election_info.joined_version = group_version;
        election_info.stake = std::numeric_limits<uint64_t>::max();
        election_info.consensus_public_key = xpublic_key_t{item.m_publickey};
        election_info.genesis = true;
        election_info.miner_type = static_cast<common::xminer_type_t>(std::numeric_limits<uint32_t>::max());
        election_info.raw_credit_score = std::numeric_limits<uint64_t>::max();

        data::election::v2::xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.account_address(node_id);
        election_info_bundle.election_info(std::move(election_info));

        current_group_nodes.insert(std::move(election_info_bundle));
    }

    STRING_CREATE(data::election::get_property_by_group_id(common::xdefault_group_id));
    serialization::xmsgpack_t<data::election::v2::xelection_result_store_t>::serialize_to_string_prop(
        *this, data::election::get_property_by_group_id(common::xdefault_group_id), election_result_store);
}

void xtop_relay_process_election_data_contract::on_recv_election_data(xbytes_t const & data) {
    std::string result = top::to_string(data);
    assert(!result.empty());
    auto const & coming_election_result_store = codec::msgpack_decode<data::election::v2::xelection_result_store_t>({std::begin(result), std::end(result)});

#ifdef DEBUG
    auto const & coming_group_result = coming_election_result_store.result_of(network_id())
                                           .result_of(common::xnode_type_t::relay)
                                           .result_of(common::xdefault_cluster_id)
                                           .result_of(common::xdefault_group_id);
    
    xdbg("xtop_relay_process_election_data_contract::on_recv_election_data epoch:%llu", coming_group_result.group_epoch().value());
    for (auto const & node_info: coming_group_result){
        auto const & election_info = top::get<top::data::election::v2::xelection_info_bundle_t>(node_info);
        xdbg("xtop_relay_process_election_data_contract: get node: %s", election_info.account_address().c_str());
    }

#endif
    serialization::xmsgpack_t<data::election::v2::xelection_result_store_t>::serialize_to_string_prop(
        *this, data::election::get_property_by_group_id(common::xdefault_group_id), coming_election_result_store);
}

NS_END4
