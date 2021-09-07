#include "xsystem_contracts/xelection/rec/xrec_standby_pool_contract.h"

#include "xcontract_common/xserialization/xserialization.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xrootblock.h"
#include "xstake/xstake_algorithm.h"
#include "xsystem_contracts/xsystem_contract_addresses.h"
#include "xcontract_common/xproperties/xproperty_map.h"

// #include "xcontract_runtime/xcontract_helper.h"

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

        // standby_result_store.result_of(network_id()).insert({node_id, seed_node_info});
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
    auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(m_standby_prop.query());
    // auto standby_result_store = serialization::xmsgpack_t<xstandby_result_store_t>::deserialize_from_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY);

    xactivation_record activation_record;
    // std::string value_str = STRING_GET2(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
    contract_common::properties::xstring_property_t genesis_prop{XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, this};
    std::string value_str = genesis_prop.query(common::xaccount_address_t{sys_contract_rec_registration_addr});
    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        activation_record.serialize_from(stream);
    }

    // if (update_standby_result_store(registration_data, standby_result_store, activation_record)) {
    //     xdbg("[xrec_standby_pool_contract_t][on_timer] standby pool updated");
    //     // serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
    //     m_standby_prop.update(serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(standby_result_store));
    // }
}

NS_END2