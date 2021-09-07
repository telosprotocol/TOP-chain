#include "xsystem_contracts/xelection/rec/xrec_standby_pool_contract.h"

#include "xcontract_common/xproperties/xproperty_map.h"
#include "xcontract_common/xserialization/xserialization.h"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xrootblock.h"
#include "xsystem_contracts/xsystem_contract_addresses.h"

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "RecStandby_"
#define XREC_STANDBY XSYSCONTRACT_MODULE XCONTRACT_PREFIX

using namespace top::contract_common;
using namespace top::xstake;
using namespace top::data::election;

NS_BEG2(top, system_contracts)

void xtop_rec_standby_pool_contract_new::setup() {
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
#if defined XENABLE_MOCK_ZEC_STAKE
        seed_node_info.user_request_role = common::xrole_type_t::edge | common::xrole_type_t::archive | common::xrole_type_t::validator | common::xrole_type_t::advance;
#endif
        seed_node_info.program_version = "1.1.0"; // todo init version
        seed_node_info.is_genesis_node = true;

        // TODO: contract networkid get
        standby_result_store.result_of(common::xnetwork_id_t{0}).insert({node_id, seed_node_info});
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

        standby_result_store.result_of(common::xnetwork_id_t{0}).insert({node_id, seed_node_info});
    }
    for (auto & standby_network_result_info : standby_result_store) {
        auto & standby_network_storage_result = top::get<election::xstandby_network_storage_result_t>(standby_network_result_info);
        standby_network_storage_result.set_activate_state(true);
    }
#endif
    // serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
    m_standby_prop.update(serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(standby_result_store));
}

void xtop_rec_standby_pool_contract_new::nodeJoinNetwork2(common::xaccount_address_t const & node_id,
                                                          common::xnetwork_id_t const & joined_network_id,
#if defined(XENABLE_MOCK_ZEC_STAKE)
                                                          common::xrole_type_t role_type,
                                                          std::string const & consensus_public_key,
                                                          uint64_t const stake,
#endif
                                                          std::string const & program_version) {
    XMETRICS_TIME_RECORD(XREC_STANDBY "add_node_all_time");
#if !defined(XENABLE_MOCK_ZEC_STAKE)

    // get reg_node_info && standby_info
    // std::map<std::string, std::string> map_nodes;

    // MAP_COPY_GET(top::xstake::XPORPERTY_CONTRACT_REG_KEY, map_nodes, sys_contract_rec_registration_addr);
    contract_common::properties::xmap_property_t<std::string, std::string> reg_node_prop{XPORPERTY_CONTRACT_REG_KEY, this};
    auto map_nodes = reg_node_prop.query(common::xaccount_address_t{sys_contract_rec_registration_addr});
    XCONTRACT_ENSURE(map_nodes.size() != 0, "[xrec_standby_pool_contract_t][nodeJoinNetwork] fail: did not get the MAP");

    auto iter = map_nodes.find(node_id.value());
    XCONTRACT_ENSURE(iter != map_nodes.end(), "[xrec_standby_pool_contract_t][nodeJoinNetwork] fail: did not find the node in contract map");

    auto const & value_str = iter->second;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
    xstake::xreg_node_info node;
    node.serialize_from(stream);

    XCONTRACT_ENSURE(node.m_account == node_id, "[xrec_standby_pool_contract_t][nodeJoinNetwork] storage data messed up?");
    XCONTRACT_ENSURE(node.m_network_ids.find(joined_network_id) != std::end(node.m_network_ids), "[xrec_standby_pool_contract_t][nodeJoinNetwork] network id is not matched. Joined network id: " + joined_network_id.to_string());

    // auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);
    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(m_standby_prop.query());

    bool update_standby{false};
    update_standby = nodeJoinNetworkImpl(program_version, node, standby_result_store);

    if (update_standby) {
        XMETRICS_PACKET_INFO(XREC_STANDBY "nodeJoinNetwork", "node_id", node_id.value(), "role_type", common::to_string(node.get_role_type()));
        // serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
        m_standby_prop.update(serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(standby_result_store));
    }

#else
    // mock stake test
    std::set<common::xnetwork_id_t> network_ids{};
    common::xnetwork_id_t nid{top::config::to_chainid(XGET_CONFIG(chain_name))};
    assert(nid == joined_network_id);
    XCONTRACT_ENSURE(nid == joined_network_id, "[xrec_standby_pool_contract_t][nodeJoinNetwork] network id is not matched");
    network_ids.insert(nid);

    bool rec = common::has<common::xrole_type_t::advance>(role_type);
    bool zec = common::has<common::xrole_type_t::advance>(role_type);
    bool auditor = common::has<common::xrole_type_t::advance>(role_type);
    bool validator = common::has<common::xrole_type_t::advance>(role_type) || common::has<common::xrole_type_t::validator>(role_type);
    bool edge = common::has<common::xrole_type_t::edge>(role_type);
    bool archive = common::has<common::xrole_type_t::advance>(role_type) || common::has<common::xrole_type_t::archive>(role_type);
    bool full_node = common::has<common::xrole_type_t::full_node>(role_type);

    std::string role_type_string = common::to_string(role_type);
    assert(role_type_string == common::XNODE_TYPE_EDGE      ||
           role_type_string == common::XNODE_TYPE_ADVANCE   ||
           role_type_string == common::XNODE_TYPE_VALIDATOR ||
           role_type_string == common::XNODE_TYPE_FULL_NODE);

    top::base::xstream_t param_stream(base::xcontext_t::instance());
    std::string nickname{"nickname"};
    param_stream << role_type_string;
    param_stream << nickname;
    param_stream << consensus_public_key;
    param_stream << static_cast<uint32_t>(0);
    param_stream << node_id;
    xdbg("[xrec_standby_pool_contract_t][nodeJoinNetwork][mock_zec_stake to registration] node_id:%s,role_type:%s",
         node_id.c_str(),
         role_type_string.c_str(),
         consensus_public_key.c_str());

    call(common::xaccount_address_t{sys_contract_rec_registration_addr},
         "registerNode",
         std::string{reinterpret_cast<char *>(param_stream.data()), static_cast<std::size_t>(param_stream.size())},
         xenum_followup_transaction_schedule_type::immediately);
    xdbg("[xrec_standby_pool_contract_t][nodeJoinNetwork][mock_zec_stake to registration] finish CALL registration contract");
    XCONTRACT_ENSURE(role_type != common::xrole_type_t::invalid, "[xrec_standby_pool_contract_t][nodeJoinNetwork] fail: find invalid role in MAP");

    xdbg("[xrec_standby_pool_contract_t][nodeJoinNetwork] %s", node_id.c_str());

    // auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);
    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(m_standby_prop.query());

    xstandby_node_info_t new_node_info;

    new_node_info.user_request_role = role_type;  // new_node.m_role_type;

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

        if (full_node) {
            new_node_info.stake_container[common::xnode_type_t::storage_full_node] = stake;
            new_node |= standby_result_store.result_of(network_id).insert({ node_id, new_node_info }).second;
        }
    }

    if (new_node) {
        XMETRICS_PACKET_INFO(XREC_STANDBY "nodeJoinNetwork", "node_id", node_id.value(), "role_type", common::to_string(role_type));
        // serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
        m_standby_prop.update(serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(standby_result_store));
    }
#endif
}

bool xtop_rec_standby_pool_contract_new::nodeJoinNetworkImpl(std::string const & program_version,
                                                             xstake::xreg_node_info const & node,
                                                             data::election::xstandby_result_store_t & standby_result_store) {
    std::set<common::xnetwork_id_t> network_ids = node.m_network_ids;

    auto consensus_public_key = node.consensus_public_key;
    uint64_t rec_stake{ 0 }, zec_stake{ 0 }, auditor_stake{ 0 }, validator_stake{ 0 }, edge_stake{ 0 }, archive_stake{ 0 }, full_node_stake{ 0 };
    bool rec{ node.rec() }, zec{ node.zec() }, auditor{ node.auditor() }, validator{ node.validator() }, edge{ node.edge() }, archive{ node.archive() }, full_node{ node.full_node() };
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

    if (full_node) {
        full_node_stake = node.full_node_stake();
    }

    auto role_type = node.get_role_type();
    XCONTRACT_ENSURE(role_type != common::xrole_type_t::invalid, "[xrec_standby_pool_contract_t][nodeJoinNetwork] fail: find invalid role in MAP");
    XCONTRACT_ENSURE(node.get_required_min_deposit() <= node.m_account_mortgage,
                     "[xrec_standby_pool_contract_t][nodeJoinNetwork] account mortgage < required_min_deposit fail: " + node.m_account.value() + ", role_type : " + common::to_string(role_type));

    xdbg("[xrec_standby_pool_contract_t][nodeJoinNetwork] %s", node.m_account.c_str());

    xstandby_node_info_t new_node_info;

    new_node_info.consensus_public_key = xpublic_key_t{consensus_public_key};
    new_node_info.program_version = program_version;

    new_node_info.is_genesis_node = node.is_genesis_node();

    // common::xnode_id_t xnode_id{node_id};
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

        if (full_node) {
            new_node_info.stake_container[common::xnode_type_t::storage_full_node] = full_node_stake;
        }

        new_node |= standby_result_store.result_of(network_id).insert2({node.m_account, new_node_info}).second;
    }

    return new_node;
}

bool xtop_rec_standby_pool_contract_new::update_standby_node(top::xstake::xreg_node_info const & reg_node, xstandby_node_info_t & standby_node_info) const {
#if defined(XENABLE_MOCK_ZEC_STAKE)
    return false;
#endif

    election::xstandby_node_info_t new_node_info;
    if (reg_node.rec()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::rec, reg_node.rec_stake() });
    }
    if (reg_node.zec()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::zec, reg_node.zec_stake() });
    }
    if (reg_node.archive()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::storage_archive, reg_node.archive_stake() });
    }
    if (reg_node.auditor()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::consensus_auditor, reg_node.auditor_stake() });
    }
    if (reg_node.validator()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::consensus_validator, reg_node.validator_stake() });
    }
    if (reg_node.edge()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::edge, reg_node.edge_stake() });
    }
    if (reg_node.full_node()) {
        new_node_info.stake_container.insert({ common::xnode_type_t::storage_full_node, reg_node.full_node_stake() });
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

bool xtop_rec_standby_pool_contract_new::update_activated_state(xstandby_network_storage_result_t & standby_network_storage_result,
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

bool xtop_rec_standby_pool_contract_new::update_standby_result_store(std::map<common::xnode_id_t, xstake::xreg_node_info> const & registration_data,
                                                                     data::election::xstandby_result_store_t & standby_result_store,
                                                                     xstake::xactivation_record const & activation_record) {
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
                if (update_standby_node(reg_node, node_info) && !updated) {
                    updated = true;
                }
            }
            it++;
        }
        updated |= update_activated_state(standby_network_storage_result, activation_record);
    }
    return updated;
}

void xtop_rec_standby_pool_contract_new::on_timer(common::xlogic_time_t const current_time) {
#ifdef STATIC_CONSENSUS
    // static_consensus won't sync registration contract data.
    return;
#endif
    XMETRICS_TIME_RECORD(XREC_STANDBY "on_timer_all_time");
    XCONTRACT_ENSURE(sender() == address(), "xrec_standby_pool_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(address().value() == sys_contract_rec_standby_pool_addr, "xrec_standby_pool_contract_t instance is not triggled by xrec_standby_pool_contract_t");
    // XCONTRACT_ENSURE(current_time <= TIME(), "xrec_standby_pool_contract_t::on_timer current_time > consensus leader's time");

    std::map<std::string, std::string> reg_node_info;  // key is the account string, value is the serialized data
    // MAP_COPY_GET(XPORPERTY_CONTRACT_REG_KEY, reg_node_info, sys_contract_rec_registration_addr);
    contract_common::properties::xmap_property_t<std::string, std::string> reg_node_prop{XPORPERTY_CONTRACT_REG_KEY, this};
    reg_node_info = reg_node_prop.query(common::xaccount_address_t{sys_contract_rec_registration_addr});
    xdbg("[xrec_standby_pool_contract_t][on_timer] registration data size %zu", reg_node_info.size());

    std::map<common::xnode_id_t, xreg_node_info> registration_data;
    for (auto const & item : reg_node_info) {
        xreg_node_info node_info;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)item.second.c_str(), (uint32_t)item.second.size());

        node_info.serialize_from(stream);
        registration_data[common::xnode_id_t{item.first}] = node_info;
        xdbg("[xrec_standby_pool_contract_t][on_timer] found from registration contract node %s", item.first.c_str());
    }
    XCONTRACT_ENSURE(!registration_data.empty(), "read registration data failed");
    ;
    // auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);
    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(m_standby_prop.query());

    xactivation_record activation_record;
    // std::string value_str = STRING_GET2(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
    contract_common::properties::xstring_property_t genesis_prop{XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, this};
    std::string value_str = genesis_prop.query(common::xaccount_address_t{sys_contract_rec_registration_addr});
    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        activation_record.serialize_from(stream);
    }

    if (update_standby_result_store(registration_data, standby_result_store, activation_record)) {
        xdbg("[xrec_standby_pool_contract_t][on_timer] standby pool updated");
        // serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
        m_standby_prop.update(serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(standby_result_store));
    }
}

NS_END2