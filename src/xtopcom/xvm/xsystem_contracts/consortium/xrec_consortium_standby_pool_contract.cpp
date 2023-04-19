// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xrec_consortium_standby_pool_contract.h"
#include "generated/version.h"
#include "xbasic/xcrypto_key.h"
#include "xbasic/xutility.h"
#include "xchain_fork/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xrole_type.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xelection/xstandby_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv0/xstandby_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection/xv1/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xstandby_network_storage_result.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xelection/xv0/xstandby_node_info.h"
#include "xdata/xelection/xv1/xstandby_result_store.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrootblock.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xserialization/xserialization.h"

#ifdef STATIC_CONSENSUS
#    include "xvm/xsystem_contracts/xelection/xstatic_election_center.h"
#endif

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "RecStandby_"
#define XREC_STANDBY XSYSCONTRACT_MODULE XCONTRACT_PREFIX

using namespace top::data;

NS_BEG3(top, xvm, consortium)

using data::election::xstandby_node_info_t;
using data::election::xstandby_result_store_t;
using data::election::xstandby_network_storage_result_t;

xtop_rec_consortium_standby_pool_contract::xtop_rec_consortium_standby_pool_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xtop_rec_consortium_standby_pool_contract::setup() {
    election::v0::xstandby_result_store_t standby_result_store;
    const std::vector<node_info_t> & seed_nodes = data::xrootblock_t::get_seed_nodes();
    for (size_t i = 0u; i < seed_nodes.size(); i++) {
        auto const & node_data = seed_nodes[i];

        common::xnode_id_t node_id{node_data.m_account};

        election::v0::xstandby_node_info_t seed_node_info;
        seed_node_info.consensus_public_key = xpublic_key_t{node_data.m_publickey};
        seed_node_info.stake_container.insert({common::xnode_type_t::rec, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::zec, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::storage_archive, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::consensus_auditor, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::consensus_validator, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::edge, 0});
#if defined(XENABLE_MOCK_ZEC_STAKE)
        seed_node_info.miner_type = common::xminer_type_t::edge | common::xminer_type_t::archive | common::xminer_type_t::validator | common::xminer_type_t::advance;
#endif  // #if defined(XENABLE_MOCK_ZEC_STAKE)
        seed_node_info.program_version = "1.1.0"; // todo init version
        seed_node_info.genesis = true;

        standby_result_store.result_of(network_id()).insert({node_id, seed_node_info});
    }

#ifdef STATIC_CONSENSUS
    auto const static_consensus_nodes_info = xstatic_election_center::instance().get_standby_config();
    for (auto const & node_info : static_consensus_nodes_info) {
        common::xnode_id_t node_id{node_info.node_id};
        election::v0::xstandby_node_info_t seed_node_info;
        seed_node_info.consensus_public_key = node_info.pub_key;
        for (auto const & _pair : node_info.type_stake_pair) {
            common::xnode_type_t const & node_type = top::get<common::xnode_type_t>(_pair);
            uint64_t stake = top::get<uint64_t>(_pair);
            seed_node_info.stake_container.insert({node_type, stake});
        }
        seed_node_info.program_version = "1.1.0"; 
        seed_node_info.genesis = false;
        seed_node_info.miner_type = common::xminer_type_t::edge | common::xminer_type_t::validator | common::xminer_type_t::advance;

        standby_result_store.result_of(network_id()).insert({node_id, seed_node_info});
    }
    for (auto & standby_network_result_info : standby_result_store) {
        auto & standby_network_storage_result = top::get<election::v0::xstandby_network_storage_result_t>(standby_network_result_info);
        standby_network_storage_result.set_activate_state(true);
    }
#endif

    STRING_CREATE(XPROPERTY_CONTRACT_STANDBYS_KEY);
    serialization::xmsgpack_t<election::v0::xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
}

void xtop_rec_consortium_standby_pool_contract::nodeJoinNetwork2(common::xaccount_address_t const & node_id,
                                                      common::xnetwork_id_t const & joined_network_id,
#if defined(XENABLE_MOCK_ZEC_STAKE)
                                                      common::xminer_type_t miner_type,
                                                      std::string const & consensus_public_key,
                                                      uint64_t const stake,
#endif  // #if defined(XENABLE_MOCK_ZEC_STAKE)
                                                     std::string const & program_version) {
    XMETRICS_TIME_RECORD(XREC_STANDBY "add_node_all_time");
    XMETRICS_CPU_TIME_RECORD(XREC_STANDBY "add_node_cpu_time");

    bool check_ret = check_node_valid(node_id.to_string());
    XCONTRACT_ENSURE(check_ret, "[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork2] failed!");   

#if !defined(XENABLE_MOCK_ZEC_STAKE)

    // get reg_node_info && standby_info
    std::map<std::string, std::string> map_nodes;

    MAP_COPY_GET(top::data::system_contract::XPORPERTY_CONTRACT_REG_KEY, map_nodes, sys_contract_rec_registration_addr);
    XCONTRACT_ENSURE(!map_nodes.empty(), "[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] fail: did not get the MAP");

    auto const iter = map_nodes.find(node_id.to_string());
    XCONTRACT_ENSURE(iter != map_nodes.end(), "[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] fail: did not find the node in contract map");

    auto const & value_str = iter->second;
    base::xstream_t stream(base::xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(value_str.data())), static_cast<uint32_t>(value_str.size()));
    data::system_contract::xreg_node_info node;
    node.serialize_from(stream);

    XCONTRACT_ENSURE(node.m_account == node_id, "[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] storage data messed up?");
    XCONTRACT_ENSURE(node.m_network_ids.find(joined_network_id) != std::end(node.m_network_ids), "[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] network id is not matched. Joined network id: " + joined_network_id.to_string());

    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);
    if (nodeJoinNetworkImpl(program_version, node, standby_result_store)) {
        XMETRICS_PACKET_INFO(XREC_STANDBY "nodeJoinNetwork", "node_id", node_id.to_string(), "miner_type", common::to_string(node.miner_type()));
        serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
    }

#else   // #if !defined(XENABLE_MOCK_ZEC_STAKE)
    // mock stake test
    std::set<common::xnetwork_id_t> network_ids{};
    common::xnetwork_id_t nid{top::config::to_chainid(XGET_CONFIG(chain_name))};
    assert(nid == joined_network_id);
    XCONTRACT_ENSURE(nid == joined_network_id, "[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] network id is not matched");
    network_ids.insert(nid);

    bool const rec = data::system_contract::could_be<common::xnode_type_t::rec>(miner_type);
    bool const zec = data::system_contract::could_be<common::xnode_type_t::zec>(miner_type);
    bool const auditor = data::system_contract::could_be<common::xnode_type_t::consensus_auditor>(miner_type);
    bool const validator = data::system_contract::could_be<common::xnode_type_t::consensus_validator>(miner_type);
    bool const edge = data::system_contract::could_be<common::xnode_type_t::edge>(miner_type);
    bool const archive = data::system_contract::could_be<common::xnode_type_t::storage_archive>(miner_type);
    bool const exchange = data::system_contract::could_be<common::xnode_type_t::storage_exchange>(miner_type);
    bool const fullnode = false;
    bool const evm_auditor = data::system_contract::could_be<common::xnode_type_t::evm_auditor>(miner_type);
    bool const evm_validator = data::system_contract::could_be<common::xnode_type_t::evm_validator>(miner_type);
    bool const relay = data::system_contract::could_be<common::xnode_type_t::relay>(miner_type);

    std::string const role_type_string = common::to_string(miner_type);
    assert(role_type_string == common::XMINER_TYPE_EDGE      ||
           role_type_string == common::XMINER_TYPE_ADVANCE   ||
           role_type_string == common::XMINER_TYPE_VALIDATOR ||
           role_type_string == common::XMINER_TYPE_ARCHIVE   ||
           role_type_string == common::XMINER_TYPE_EXCHANGE);

    top::base::xstream_t param_stream(base::xcontext_t::instance());
    std::string nickname{"nickname"};
    param_stream << role_type_string;
    param_stream << nickname;
    param_stream << consensus_public_key;
    param_stream << static_cast<uint32_t>(0);
    param_stream << node_id;
    xdbg("[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork][mock_zec_stake to registration] node_id:%s,miner_type:%s",
         node_id.to_string().c_str(),
         role_type_string.c_str(),
         consensus_public_key.c_str());
    CALL(common::xaccount_address_t{sys_contract_rec_registration_addr},
         "registerNode",
         std::string{reinterpret_cast<char *>(param_stream.data()), static_cast<std::size_t>(param_stream.size())});
    xdbg("[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork][mock_zec_stake to registration] finish CALL registration contract");
    XCONTRACT_ENSURE(miner_type != common::xminer_type_t::invalid, "[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] fail: find invalid role in MAP");

    xdbg("[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] %s", node_id.to_string().c_str());

    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);

    xstandby_node_info_t new_node_info;

    new_node_info.miner_type = miner_type;

    new_node_info.consensus_public_key = xpublic_key_t{consensus_public_key};
    new_node_info.program_version = program_version;

    new_node_info.genesis = false;

    bool new_node{false};
    for (const auto network_id : network_ids) {
        assert(network_id == common::xnetwork_id_t{ base::enum_consortium_id } );

        if (rec) {
            new_node_info.stake_container[common::xnode_type_t::rec] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({ node_id, new_node_info }).second;
        }

        if (zec) {
            new_node_info.stake_container[common::xnode_type_t::zec] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({ node_id, new_node_info }).second;
        }

        if (auditor) {
            new_node_info.stake_container[common::xnode_type_t::consensus_auditor] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({ node_id, new_node_info }).second;
        }

        if (validator) {
            new_node_info.stake_container[common::xnode_type_t::consensus_validator] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({ node_id, new_node_info }).second;
        }

        if (edge) {
            new_node_info.stake_container[common::xnode_type_t::edge] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({ node_id, new_node_info }).second;
        }

        if (archive) {
            new_node_info.stake_container[common::xnode_type_t::storage_archive] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({ node_id, new_node_info }).second;
        }

        if (exchange) {
            new_node_info.stake_container[common::xnode_type_t::storage_exchange] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({ node_id, new_node_info }).second;
        }

        if (fullnode) {
            new_node_info.stake_container[common::xnode_type_t::fullnode] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({node_id, new_node_info}).second;
        }

        if (evm_auditor) {
            new_node_info.stake_container[common::xnode_type_t::evm_auditor] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({node_id, new_node_info}).second;
        }

        if (evm_validator) {
            new_node_info.stake_container[common::xnode_type_t::evm_validator] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({node_id, new_node_info}).second;
        }

        if (relay) {
            new_node_info.stake_container[common::xnode_type_t::relay] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({node_id, new_node_info}).second;
        }
    }

    if (new_node) {
        XMETRICS_PACKET_INFO(XREC_STANDBY "nodeJoinNetwork", "node_id", node_id.to_string(), "miner_type", common::to_string(miner_type));
        serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
    }
#endif
}

bool xtop_rec_consortium_standby_pool_contract::nodeJoinNetworkImpl(std::string const & program_version,
                                                         data::system_contract::xreg_node_info const & node,
                                                         data::election::xstandby_result_store_t & standby_result_store) {
    auto const evm_enabled = true;
    auto const relay_enabled = true;

    std::set<common::xnetwork_id_t> network_ids = node.m_network_ids;

    auto consensus_public_key = node.consensus_public_key;
    uint64_t rec_stake{0}, zec_stake{0}, auditor_stake{0}, validator_stake{0}, edge_stake{0}, archive_stake{0}, exchange_stake{0}, fullnode_stake{0}, evm_auditor_stake{0},
        evm_validator_stake{0}, relay_stake{0};
    bool  node_whitelist_enable = XGET_ONCHAIN_GOVERNANCE_PARAMETER(enable_node_whitelist);

    bool const rec{node.can_be_rec()},                                     // NOLINT
        zec{node.can_be_zec()},                                            // NOLINT
        auditor{ node_whitelist_enable ? node.could_be_auditor() : node.can_be_auditor() }, // NOLINT  
        validator{node.can_be_validator()},                                // NOLINT
        edge{node.can_be_edge()},                                          // NOLINT
        archive{node.can_be_archive()},                                    // NOLINT
        exchange{node.can_be_exchange()},                                  // NOLINT
        fullnode{node.can_be_fullnode()},                                  // NOLINT
        evm_auditor{node_whitelist_enable ? node.could_be_evm_auditor() : node.can_be_evm_auditor()}, // NOLINT
        evm_validator{node_whitelist_enable ? node.can_be_evm_validator() : false},  // NOLINT
        relay{node_whitelist_enable ? node.could_be_relay() : node.can_be_relay()};  // NOLINT

    if (rec) {
        rec_stake = node.rec_stake();
    }

    if (zec) {
        zec_stake = node.zec_stake();
    }

    if (auditor) {
        auditor_stake = node.auditor_stake();
    }

    if (validator) {
        validator_stake = node.validator_stake();
    }

    if (edge) {
        edge_stake = node.edge_stake();
    }

    if (archive) {
        archive_stake = node.archive_stake();
    }

    if (exchange) {
        exchange_stake = node.exchange_stake();
    }

    if (fullnode) {
        fullnode_stake = node.fullnode_stake();
    }

    if (evm_enabled && evm_auditor) {
        evm_auditor_stake = node.evm_auditor_stake();
    }

    if (evm_enabled && evm_validator) {
        evm_validator_stake = node.evm_validator_stake();
    }

    if (relay_enabled && relay) {
        relay_stake = node.relay_stake();
    }

    auto const miner_type = node.miner_type();
    XCONTRACT_ENSURE(miner_type != common::xminer_type_t::invalid, "[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] fail: find invalid role in MAP");
    XCONTRACT_ENSURE(node.get_required_min_deposit() <= node.deposit(),
                     "[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] account mortgage < required_min_deposit fail: " + node.m_account.to_string() +
                         ", miner_type : " + common::to_string(miner_type));

    xdbg("[xrec_consortium_standby_pool_contract_t][nodeJoinNetwork] %s", node.m_account.to_string().c_str());

    xstandby_node_info_t new_node_info;

    new_node_info.consensus_public_key = xpublic_key_t{consensus_public_key};
    new_node_info.program_version = program_version;

    new_node_info.genesis = node.genesis();
    new_node_info.miner_type = miner_type;

    bool new_node{false};
    for (const auto network_id : network_ids) {
        assert(network_id == common::xnetwork_id_t{ base::enum_consortium_id } );

        if (rec) {
            new_node_info.stake_container[common::xnode_type_t::rec] = rec_stake;
        }
        if (zec) {
            new_node_info.stake_container[common::xnode_type_t::zec] = zec_stake;
        }

        if (auditor) {
            new_node_info.stake_container[common::xnode_type_t::consensus_auditor] = auditor_stake;
        }

        if (validator) {
            new_node_info.stake_container[common::xnode_type_t::consensus_validator] = validator_stake;
        }

        if (edge) {
            new_node_info.stake_container[common::xnode_type_t::edge] = edge_stake;
        }

        if (archive) {
            new_node_info.stake_container[common::xnode_type_t::storage_archive] = archive_stake;
            xdbg("archive standby: %s", node.m_account.to_string().c_str());
        }

        if (exchange) {
            new_node_info.stake_container[common::xnode_type_t::storage_exchange] = exchange_stake;
        }

        if (fullnode) {
            new_node_info.stake_container[common::xnode_type_t::fullnode] = fullnode_stake;
        }

        if (evm_enabled && evm_auditor) {
            new_node_info.stake_container[common::xnode_type_t::evm_auditor] = evm_auditor_stake;
        }

        if (evm_enabled && evm_validator) {
            new_node_info.stake_container[common::xnode_type_t::evm_validator] = evm_validator_stake;
        }

        if (relay_enabled && relay) {
            new_node_info.stake_container[common::xnode_type_t::relay] = relay_stake;
        }

        if (!new_node) {
            new_node = standby_result_store.result_of(network_id).insert({node.m_account, new_node_info}).second;
        }
    }

    return new_node;
}

bool xtop_rec_consortium_standby_pool_contract::update_standby_node(data::system_contract::xreg_node_info const & reg_node,
                                                         xstandby_node_info_t & standby_node_info,
                                                         common::xlogic_time_t const current_logic_time) const {
    auto const evm_enabled = true;
    auto const relay_enabled = true;

    election::xstandby_node_info_t new_node_info;
    if (reg_node.can_be_rec()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::rec, reg_node.rec_stake() });
    }
    if (reg_node.can_be_zec()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::zec, reg_node.zec_stake() });
    }
    if (reg_node.can_be_fullnode()) {
        new_node_info.stake_container.insert({common::xnode_type_t::fullnode, reg_node.fullnode_stake()});
    }
    if (reg_node.can_be_archive() || reg_node.genesis()) {
        new_node_info.stake_container.insert({common::xnode_type_t::storage_archive, reg_node.archive_stake()});
    }
    if (reg_node.can_be_auditor()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::consensus_auditor, reg_node.auditor_stake() });
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node account %s credit score %" PRIu64,
             reg_node.m_account.to_string().c_str(),
             reg_node.raw_credit_score_data(common::xnode_type_t::consensus_auditor));
        new_node_info.raw_credit_score(common::xnode_type_t::consensus_auditor, reg_node.raw_credit_score_data(top::common::xnode_type_t::consensus_auditor));
    }
    if (reg_node.can_be_validator()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::consensus_validator, reg_node.validator_stake() });
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node account %s credit score %" PRIu64,
             reg_node.m_account.to_string().c_str(),
             reg_node.raw_credit_score_data(common::xnode_type_t::consensus_validator));
        new_node_info.raw_credit_score(common::xnode_type_t::consensus_validator, reg_node.raw_credit_score_data(top::common::xnode_type_t::consensus_validator));
    }
    if (reg_node.can_be_edge()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::edge, reg_node.edge_stake() });
    }
    if (reg_node.can_be_exchange()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::storage_exchange, reg_node.exchange_stake() });
    }
    if (evm_enabled && reg_node.can_be_evm_auditor()) {
        new_node_info.stake_container.insert({common::xnode_type_t::evm_auditor, reg_node.evm_auditor_stake()});
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node account %s credit score %" PRIu64,
             reg_node.m_account.to_string().c_str(),
             reg_node.raw_credit_score_data(common::xnode_type_t::consensus_auditor));
        new_node_info.raw_credit_score(common::xnode_type_t::evm_auditor, reg_node.raw_credit_score_data(top::common::xnode_type_t::consensus_auditor));
    }
    if (evm_enabled && reg_node.can_be_evm_validator()) {
        new_node_info.stake_container.insert({common::xnode_type_t::evm_validator, reg_node.evm_validator_stake()});
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node account %s credit score %" PRIu64,
             reg_node.m_account.to_string().c_str(),
             reg_node.raw_credit_score_data(common::xnode_type_t::consensus_validator));
        new_node_info.raw_credit_score(common::xnode_type_t::evm_validator, reg_node.raw_credit_score_data(top::common::xnode_type_t::consensus_validator));
    }
    if (relay_enabled && reg_node.can_be_relay()) {
        new_node_info.stake_container.insert({common::xnode_type_t::relay, reg_node.relay_stake()});
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node account %s credit score %" PRIu64,
             reg_node.m_account.to_string().c_str(),
             reg_node.raw_credit_score_data(common::xnode_type_t::relay));
        new_node_info.raw_credit_score(common::xnode_type_t::relay, reg_node.raw_credit_score_data(top::common::xnode_type_t::relay));
    }

    new_node_info.consensus_public_key = reg_node.consensus_public_key;
    new_node_info.program_version = standby_node_info.program_version;
    new_node_info.genesis = reg_node.genesis();
    new_node_info.miner_type = reg_node.miner_type();

    if (new_node_info == standby_node_info) {
#if defined(DEBUG)
        for (auto const & score_info : new_node_info.raw_credit_scores) {
            xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node same new account %s credit score %" PRIu64, reg_node.m_account.to_string().c_str(), score_info.second);
        }

        for (auto const & score_info : standby_node_info.raw_credit_scores) {
            xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node same old account %s credit score %" PRIu64, reg_node.m_account.to_string().c_str(), score_info.second);
        }
#endif        
        return false;
    }

#if defined(DEBUG)
    for (auto const & score_info : new_node_info.raw_credit_scores) {
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node diff new account %s credit score %" PRIu64, reg_node.m_account.to_string().c_str(), score_info.second);
    }

    for (auto const & score_info : standby_node_info.raw_credit_scores) {
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node diff old account %s credit score %" PRIu64, reg_node.m_account.to_string().c_str(), score_info.second);
    }
#endif

    standby_node_info = new_node_info;

#if defined(DEBUG)
    if (reg_node.can_be_validator()) {
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node account %s updated credit score %" PRIu64,
             reg_node.m_account.to_string().c_str(),
             reg_node.raw_credit_score_data(common::xnode_type_t::consensus_validator));
    }
    if (reg_node.can_be_auditor()) {
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node account %s updated credit score %" PRIu64,
             reg_node.m_account.to_string().c_str(),
             reg_node.raw_credit_score_data(common::xnode_type_t::consensus_auditor));
    }
    if (reg_node.can_be_evm_validator()) {
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node account %s updated credit score %" PRIu64,
             reg_node.m_account.to_string().c_str(),
             reg_node.raw_credit_score_data(common::xnode_type_t::consensus_validator));
    }
    if (reg_node.can_be_evm_auditor()) {
        xdbg("xrec_consortium_standby_pool_contract_t::update_standby_node account %s updated credit score %" PRIu64,
             reg_node.m_account.to_string().c_str(),
             reg_node.raw_credit_score_data(common::xnode_type_t::consensus_auditor));
    }
#endif
    return true;
}

bool xtop_rec_consortium_standby_pool_contract::update_activated_state(xstandby_network_storage_result_t & standby_network_storage_result,
                                                            data::system_contract::xactivation_record const & activation_record) {
    if (standby_network_storage_result.activated_state()) {
        return false;
    }
    if (activation_record.activated) {
        standby_network_storage_result.set_activate_state(true);
        return true;
    }

    return false;
}

bool xtop_rec_consortium_standby_pool_contract::update_standby_result_store(std::map<common::xnode_id_t, data::system_contract::xreg_node_info> const & registration_data,
                                                                 data::election::xstandby_result_store_t & standby_result_store,
                                                                 data::system_contract::xactivation_record const & activation_record,
                                                                 common::xlogic_time_t const current_logic_time) {
    bool updated{false};
    for (auto & standby_network_result_info : standby_result_store) {
        assert(top::get<common::xnetwork_id_t const>(standby_network_result_info).value() == base::enum_consortium_id);

        auto & standby_network_storage_result = top::get<election::xstandby_network_storage_result_t>(standby_network_result_info);
        for (auto it = standby_network_storage_result.begin(); it != standby_network_storage_result.end();) {
            auto const & node_id = top::get<common::xnode_id_t const>(*it);
            auto & node_info = top::get<election::xstandby_node_info_t>(*it);
            assert(!node_info.program_version.empty());

            auto registration_iter = registration_data.find(node_id);
            if (registration_iter == std::end(registration_data)) {
                XMETRICS_PACKET_INFO(XREC_STANDBY "nodeLeaveNetwork", "node_id", node_id.to_string(), "reason", "dereg");
                it = standby_network_storage_result.erase(it);
                if (!updated) {
                    updated = true;
                }
                continue;
            } else {
                auto const & reg_node = top::get<data::system_contract::xreg_node_info>(*registration_iter);
                if (update_standby_node(reg_node, node_info, current_logic_time) && !updated) {
                    updated = true;
                }
            }
            it++;
        }

        auto activated_state_updated = update_activated_state(standby_network_storage_result, activation_record);
        if (!updated && activated_state_updated) {
            updated = activated_state_updated;
        }
    }
    return updated;
}

void xtop_rec_consortium_standby_pool_contract::on_timer(common::xlogic_time_t const current_time) {
#ifdef STATIC_CONSENSUS
    // static_consensus won't sync registration contract data.
    return;
#endif
    XMETRICS_TIME_RECORD(XREC_STANDBY "on_timer_all_time");
    XMETRICS_CPU_TIME_RECORD(XREC_STANDBY "on_timer_cpu_time");
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().to_string(), "xrec_consortium_standby_pool_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(SELF_ADDRESS().to_string() == sys_contract_rec_standby_pool_addr, "xrec_consortium_standby_pool_contract_t instance is not triggled by xrec_consortium_standby_pool_contract_t");
    // XCONTRACT_ENSURE(current_time <= TIME(), "xrec_consortium_standby_pool_contract_t::on_timer current_time > consensus leader's time");

    std::map<std::string, std::string> reg_node_info;  // key is the account string, value is the serialized data
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_REG_KEY, reg_node_info, sys_contract_rec_registration_addr);
    xdbg("[xrec_consortium_standby_pool_contract_t][on_timer] registration data size %zu", reg_node_info.size());

    std::map<common::xnode_id_t, data::system_contract::xreg_node_info> registration_data;
    for (auto const & item : reg_node_info) {
        data::system_contract::xreg_node_info node_info;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)item.second.c_str(), (uint32_t)item.second.size());

        node_info.serialize_from(stream);

        bool check_ret = check_node_valid(item.first);
        if (!check_ret) {
            xwarn("[xrec_consortium_standby_pool_contract_t][on_timer] account is not valid %s",item.first.c_str());
            continue;
        }
        registration_data[common::xnode_id_t{item.first}] = node_info;
        xdbg("[xrec_consortium_standby_pool_contract_t][on_timer] found from registration contract node %s", item.first.c_str());
    }
    XCONTRACT_ENSURE(!registration_data.empty(), "read registration data failed");

    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);

    data::system_contract::xactivation_record activation_record;
    std::string value_str = STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        activation_record.serialize_from(stream);
    }

    if (update_standby_result_store(registration_data, standby_result_store, activation_record, current_time)) {
        xdbg("[xrec_consortium_standby_pool_contract_t][on_timer] standby pool updated");

        serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
    }
}

bool xtop_rec_consortium_standby_pool_contract::check_node_valid(std::string const &account_str)
{ 
    std::string reg_node_info_str;

    try {
        MAP_GET2(data::system_contract::XPROPERTY_NODE_INFO_MAP_KEY, account_str, reg_node_info_str, sys_contract_rec_node_manage_addr);
        if(reg_node_info_str.empty()) {
            xwarn("[xtop_rec_consortium_standby_pool_contract::check_node_valid] can't find account from node manage contract %s", account_str.c_str());
            return false;
        }
    } catch (top::error::xtop_error_t const&) {
        xdbg("[xtop_rec_consortium_standby_pool_contract::check_node_valid] can't find %s", account_str.c_str());
        return false; // not exist
    }

    data::system_contract::xnode_manage_account_info_t reg_account_info;
    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t*)reg_node_info_str.data(), reg_node_info_str.size());
    if (_stream.size() > 0) {
        reg_account_info.serialize_from(_stream);
    }

    std::string check_all, check_ca, check_expiry_time;
    try {
        MAP_GET2(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, "check_all", check_all, sys_contract_rec_node_manage_addr);
        MAP_GET2(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, "check_ca", check_ca, sys_contract_rec_node_manage_addr);
        MAP_GET2(data::system_contract::XPROPERTY_NODE_CHECK_OPTION_KEY, "check_expiry_time", check_expiry_time, sys_contract_rec_node_manage_addr);
    } catch (top::error::xtop_error_t const&) {
        xwarn("[xtop_rec_consortium_standby_pool_contract::check_node_valid] can't find XPROPERTY_NODE_CHECK_OPTION_KEY");
        return false;
    }

    if (check_all == "1") {
        uint64_t cur_time = TIME();
        if (check_ca == "1") {
            if (reg_account_info.cert_time < cur_time) {
                xwarn("[xtop_rec_consortium_standby_pool_contract::check_node_valid] account %s  time is expiry", account_str.c_str());
                return false;
            }
        }
        if (check_expiry_time == "1") {
            if (reg_account_info.expiry_time < cur_time) {
                xwarn("[xtop_rec_consortium_standby_pool_contract::check_node_valid] account %s cert time is expiry", account_str.c_str());
                return false;
            }
        }
    }
    return true;
}

NS_END3
