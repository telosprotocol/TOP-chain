// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract.h"

#include "generated/version.h"
#include "xbasic/xcrypto_key.h"
#include "xbasic/xutility.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xrole_type.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrootblock.h"
#include "xstake/xstake_algorithm.h"
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

NS_BEG4(top, xvm, system_contracts, rec)

using data::election::xstandby_node_info_t;
using data::election::xstandby_result_store_t;
using data::election::xstandby_network_storage_result_t;

xtop_rec_standby_pool_contract::xtop_rec_standby_pool_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xtop_rec_standby_pool_contract::setup() {
    xstandby_result_store_t standby_result_store;
    const std::vector<node_info_t> & seed_nodes = data::xrootblock_t::get_seed_nodes();
    for (size_t i = 0u; i < seed_nodes.size(); i++) {
        auto const & node_data = seed_nodes[i];

        common::xnode_id_t node_id{node_data.m_account};

        xstandby_node_info_t seed_node_info;
        seed_node_info.consensus_public_key = xpublic_key_t{node_data.m_publickey};
        seed_node_info.stake_container.insert({common::xnode_type_t::rec, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::zec, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::storage_archive, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::consensus_auditor, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::consensus_validator, 0});
        seed_node_info.stake_container.insert({common::xnode_type_t::edge, 0});
#if defined(XENABLE_MOCK_ZEC_STAKE)
        seed_node_info.user_request_role = common::xminer_type_t::edge | common::xminer_type_t::archive | common::xminer_type_t::validator | common::xminer_type_t::advance;
#endif  // #if defined(XENABLE_MOCK_ZEC_STAKE)
        seed_node_info.program_version = "1.1.0"; // todo init version
        seed_node_info.is_genesis_node = true;

        standby_result_store.result_of(network_id()).insert({node_id, seed_node_info});
    }

#ifdef STATIC_CONSENSUS
    auto const static_consensus_nodes_info = xstatic_election_center::instance().get_standby_config();
    for (auto const & node_info : static_consensus_nodes_info) {
        common::xnode_id_t node_id{node_info.node_id};
        xstandby_node_info_t seed_node_info;
        seed_node_info.consensus_public_key = node_info.pub_key;
        for (auto const & _pair : node_info.type_stake_pair) {
            common::xnode_type_t const & node_type = top::get<common::xnode_type_t>(_pair);
            uint64_t stake = top::get<uint64_t>(_pair);
            seed_node_info.stake_container.insert({node_type, stake});
        }
        seed_node_info.program_version = "1.1.0"; 
        seed_node_info.is_genesis_node = false;

        standby_result_store.result_of(network_id()).insert({node_id, seed_node_info});
    }
    for (auto & standby_network_result_info : standby_result_store) {
        auto & standby_network_storage_result = top::get<election::xstandby_network_storage_result_t>(standby_network_result_info);
        standby_network_storage_result.set_activate_state(true);
    }
#endif

    STRING_CREATE(XPROPERTY_CONTRACT_STANDBYS_KEY);
    serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
}

void xtop_rec_standby_pool_contract::nodeJoinNetwork2(common::xaccount_address_t const & node_id,
                                                      common::xnetwork_id_t const & joined_network_id,
#if defined(XENABLE_MOCK_ZEC_STAKE)
                                                      common::xminer_type_t miner_type,
                                                      std::string const & consensus_public_key,
                                                      uint64_t const stake,
#endif  // #if defined(XENABLE_MOCK_ZEC_STAKE)
                                                     std::string const & program_version) {
    XMETRICS_TIME_RECORD(XREC_STANDBY "add_node_all_time");
    XMETRICS_CPU_TIME_RECORD(XREC_STANDBY "add_node_cpu_time");
#if !defined(XENABLE_MOCK_ZEC_STAKE)

    // get reg_node_info && standby_info
    std::map<std::string, std::string> map_nodes;

    MAP_COPY_GET(top::xstake::XPORPERTY_CONTRACT_REG_KEY, map_nodes, sys_contract_rec_registration_addr);
    XCONTRACT_ENSURE(!map_nodes.empty(), "[xrec_standby_pool_contract_t][nodeJoinNetwork] fail: did not get the MAP");

    auto const iter = map_nodes.find(node_id.value());
    XCONTRACT_ENSURE(iter != map_nodes.end(), "[xrec_standby_pool_contract_t][nodeJoinNetwork] fail: did not find the node in contract map");

    auto const & value_str = iter->second;
    base::xstream_t stream(base::xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(value_str.data())), static_cast<uint32_t>(value_str.size()));
    xstake::xreg_node_info node;
    node.serialize_from(stream);

    XCONTRACT_ENSURE(node.m_account == node_id, "[xrec_standby_pool_contract_t][nodeJoinNetwork] storage data messed up?");
    XCONTRACT_ENSURE(node.m_network_ids.find(joined_network_id) != std::end(node.m_network_ids), "[xrec_standby_pool_contract_t][nodeJoinNetwork] network id is not matched. Joined network id: " + joined_network_id.to_string());

    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);
    if (nodeJoinNetworkImpl(program_version, node, standby_result_store)) {
        XMETRICS_PACKET_INFO(XREC_STANDBY "nodeJoinNetwork", "node_id", node_id.value(), "miner_type", common::to_string(node.miner_type()));
        serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
    }

#else   // #if !defined(XENABLE_MOCK_ZEC_STAKE)
    // mock stake test
    std::set<common::xnetwork_id_t> network_ids{};
    common::xnetwork_id_t nid{top::config::to_chainid(XGET_CONFIG(chain_name))};
    assert(nid == joined_network_id);
    XCONTRACT_ENSURE(nid == joined_network_id, "[xrec_standby_pool_contract_t][nodeJoinNetwork] network id is not matched");
    network_ids.insert(nid);

    bool const rec = xstake::could_be<common::xnode_type_t::rec>(miner_type);
    bool const zec = xstake::could_be<common::xnode_type_t::zec>(miner_type);
    bool const auditor = xstake::could_be<common::xnode_type_t::consensus_auditor>(miner_type);
    bool const validator = xstake::could_be<common::xnode_type_t::consensus_validator>(miner_type);
    bool const edge = xstake::could_be<common::xnode_type_t::edge>(miner_type);
    bool const archive = xstake::could_be<common::xnode_type_t::storage_archive>(miner_type);
    bool const exchange = xstake::could_be<common::xnode_type_t::storage_exchange>(miner_type);
    bool const fullnode = xstake::could_be<common::xnode_type_t::fullnode>(miner_type);

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
    xdbg("[xrec_standby_pool_contract_t][nodeJoinNetwork][mock_zec_stake to registration] node_id:%s,miner_type:%s",
         node_id.c_str(),
         role_type_string.c_str(),
         consensus_public_key.c_str());
    CALL(common::xaccount_address_t{sys_contract_rec_registration_addr},
         "registerNode",
         std::string{reinterpret_cast<char *>(param_stream.data()), static_cast<std::size_t>(param_stream.size())});
    xdbg("[xrec_standby_pool_contract_t][nodeJoinNetwork][mock_zec_stake to registration] finish CALL registration contract");
    XCONTRACT_ENSURE(miner_type != common::xminer_type_t::invalid, "[xrec_standby_pool_contract_t][nodeJoinNetwork] fail: find invalid role in MAP");

    xdbg("[xrec_standby_pool_contract_t][nodeJoinNetwork] %s", node_id.c_str());

    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);

    xstandby_node_info_t new_node_info;

    new_node_info.user_request_role = miner_type;  // new_node.m_role_type;

    new_node_info.consensus_public_key = xpublic_key_t{consensus_public_key};
    new_node_info.program_version = program_version;

    new_node_info.is_genesis_node = false;

    bool new_node{false};
    for (const auto network_id : network_ids) {
        assert(network_id == common::xnetwork_id_t{ base::enum_test_chain_id } || network_id == common::xnetwork_id_t{ base::enum_main_chain_id });

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
    }

    if (new_node) {
        XMETRICS_PACKET_INFO(XREC_STANDBY "nodeJoinNetwork", "node_id", node_id.value(), "miner_type", common::to_string(miner_type));
        serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
    }
#endif
}

bool xtop_rec_standby_pool_contract::nodeJoinNetworkImpl(std::string const & program_version,
                                                         xstake::xreg_node_info const & node,
                                                         data::election::xstandby_result_store_t & standby_result_store) {
    auto const & fork_config = chain_fork::xchain_fork_config_center_t::chain_fork_config();
#if defined(XENABLE_TESTS)
    auto const fullnode_enabled = true;
#else
    auto const fullnode_enabled = chain_fork::xchain_fork_config_center_t::is_forked(fork_config.enable_fullnode_election_fork_point, TIME());
#endif

    std::set<common::xnetwork_id_t> network_ids = node.m_network_ids;

    auto consensus_public_key = node.consensus_public_key;
    uint64_t rec_stake{0}, zec_stake{0}, auditor_stake{0}, validator_stake{0}, edge_stake{0}, archive_stake{0}, exchange_stake{0}, fullnode_stake{0};
    bool const rec{node.can_be_rec()}, zec{node.can_be_zec()}, auditor{node.can_be_auditor()}, validator{node.can_be_validator()}, edge{node.can_be_edge()},
        archive{fullnode_enabled ? node.can_be_archive() : node.legacy_can_be_archive()}, exchange{node.can_be_exchange()}, fullnode{node.can_be_fullnode()};
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

    auto const miner_type = node.miner_type();
    XCONTRACT_ENSURE(miner_type != common::xminer_type_t::invalid, "[xrec_standby_pool_contract_t][nodeJoinNetwork] fail: find invalid role in MAP");
    XCONTRACT_ENSURE(node.get_required_min_deposit() <= node.deposit(),
                     "[xrec_standby_pool_contract_t][nodeJoinNetwork] account mortgage < required_min_deposit fail: " + node.m_account.value() +
                         ", miner_type : " + common::to_string(miner_type));

    xdbg("[xrec_standby_pool_contract_t][nodeJoinNetwork] %s", node.m_account.c_str());

    xstandby_node_info_t new_node_info;

    new_node_info.consensus_public_key = xpublic_key_t{consensus_public_key};
    new_node_info.program_version = program_version;

    new_node_info.is_genesis_node = node.is_genesis_node();

    bool new_node{false};
    for (const auto network_id : network_ids) {
        assert(network_id == common::xnetwork_id_t{ base::enum_test_chain_id } ||
               network_id == common::xnetwork_id_t{ base::enum_main_chain_id });

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
            xdbg("archive standby: %s", node.m_account.c_str());
        }

        if (exchange) {
            new_node_info.stake_container[common::xnode_type_t::storage_exchange] = exchange_stake;
        }

        if (fullnode_enabled && fullnode) {
            new_node_info.stake_container[common::xnode_type_t::fullnode] = fullnode_stake;
        }

        if (!new_node) {
            new_node = standby_result_store.result_of(network_id).insert2({node.m_account, new_node_info}).second;
        }
    }

    return new_node;
}

bool xtop_rec_standby_pool_contract::update_standby_node(top::xstake::xreg_node_info const & reg_node,
                                                         xstandby_node_info_t & standby_node_info,
                                                         common::xlogic_time_t const current_logic_time) const {
    auto const & fork_config = chain_fork::xchain_fork_config_center_t::chain_fork_config();
    // make sure fullnode must be elected out before clearing auditor type.
    auto const fullnode_enabled = chain_fork::xchain_fork_config_center_t::is_forked(
        fork_config.enable_fullnode_election_fork_point, 2 * XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullnode_election_interval), current_logic_time);

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
    if (fullnode_enabled) {
        // after fullnode enabled, archive node generates from genesis node. Normal advance miner won't be arhive node anymore.
        if (reg_node.can_be_archive() || reg_node.is_genesis_node()) {
            new_node_info.stake_container.insert({common::xnode_type_t::storage_archive, reg_node.archive_stake()});
        }
    } else {
        if (reg_node.legacy_can_be_archive()) {
            new_node_info.stake_container.insert({common::xnode_type_t::storage_archive, reg_node.archive_stake()});
        }
    }
    if (reg_node.can_be_auditor()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::consensus_auditor, reg_node.auditor_stake() });
    }
    if (reg_node.can_be_validator()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::consensus_validator, reg_node.validator_stake() });
    }
    if (reg_node.can_be_edge()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::edge, reg_node.edge_stake() });
    }
    if (reg_node.can_be_exchange()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::storage_exchange, reg_node.exchange_stake() });
    }
    new_node_info.consensus_public_key = reg_node.consensus_public_key;
    new_node_info.program_version = standby_node_info.program_version;
    new_node_info.is_genesis_node = reg_node.is_genesis_node();
    if (new_node_info == standby_node_info) {
        return false;
    } else {
        standby_node_info = new_node_info;
    }
    return true;
}

bool xtop_rec_standby_pool_contract::update_activated_state(xstandby_network_storage_result_t & standby_network_storage_result,
                                                            xstake::xactivation_record const & activation_record) {
    if (standby_network_storage_result.activated_state()) {
        return false;
    }
    if (activation_record.activated) {
        standby_network_storage_result.set_activate_state(true);
        return true;
    }

    return false;
}

bool xtop_rec_standby_pool_contract::update_standby_result_store(std::map<common::xnode_id_t, xstake::xreg_node_info> const & registration_data,
                                                                 data::election::xstandby_result_store_t & standby_result_store,
                                                                 xstake::xactivation_record const & activation_record,
                                                                 common::xlogic_time_t const current_logic_time) {
    bool updated{false};
    for (auto & standby_network_result_info : standby_result_store) {
        assert(top::get<common::xnetwork_id_t const>(standby_network_result_info).value() == base::enum_test_chain_id ||
               top::get<common::xnetwork_id_t const>(standby_network_result_info).value() == base::enum_main_chain_id);

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
                auto const & reg_node = top::get<top::xstake::xreg_node_info>(*registration_iter);
                if (update_standby_node(reg_node, node_info, current_logic_time) && !updated) {
                    updated = true;
                }
            }
            it++;
        }

        if (!updated) {
            updated = update_activated_state(standby_network_storage_result, activation_record);
        }
    }
    return updated;
}

void xtop_rec_standby_pool_contract::on_timer(common::xlogic_time_t const current_time) {
#ifdef STATIC_CONSENSUS
    // static_consensus won't sync registration contract data.
    return;
#endif
    XMETRICS_TIME_RECORD(XREC_STANDBY "on_timer_all_time");
    XMETRICS_CPU_TIME_RECORD(XREC_STANDBY "on_timer_cpu_time");
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().value(), "xrec_standby_pool_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(SELF_ADDRESS().value() == sys_contract_rec_standby_pool_addr, "xrec_standby_pool_contract_t instance is not triggled by xrec_standby_pool_contract_t");
    // XCONTRACT_ENSURE(current_time <= TIME(), "xrec_standby_pool_contract_t::on_timer current_time > consensus leader's time");

    std::map<std::string, std::string> reg_node_info;  // key is the account string, value is the serialized data
    MAP_COPY_GET(xstake::XPORPERTY_CONTRACT_REG_KEY, reg_node_info, sys_contract_rec_registration_addr);
    xdbg("[xrec_standby_pool_contract_t][on_timer] registration data size %zu", reg_node_info.size());

    std::map<common::xnode_id_t, xstake::xreg_node_info> registration_data;
    for (auto const & item : reg_node_info) {
        xstake::xreg_node_info node_info;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)item.second.c_str(), (uint32_t)item.second.size());

        node_info.serialize_from(stream);
        registration_data[common::xnode_id_t{item.first}] = node_info;
        xdbg("[xrec_standby_pool_contract_t][on_timer] found from registration contract node %s", item.first.c_str());
    }
    XCONTRACT_ENSURE(!registration_data.empty(), "read registration data failed");

    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);

    xstake::xactivation_record activation_record;
    std::string value_str = STRING_GET2(xstake::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        activation_record.serialize_from(stream);
    }

    if (update_standby_result_store(registration_data, standby_result_store, activation_record, current_time)) {
        xdbg("[xrec_standby_pool_contract_t][on_timer] standby pool updated");
        serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
    }
}

NS_END4
