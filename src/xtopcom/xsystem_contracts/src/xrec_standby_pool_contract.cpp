#include "xsystem_contracts/xelection/rec/xrec_standby_pool_contract.h"

#include "xsystem_contracts/xsystem_contract_addresses.h"

// #include "xcontract_runtime/xcontract_helper.h"

NS_BEG2(top, system_contracts)

void xtop_rec_standby_pool_contract_new::setup() {
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
#if 0
#ifdef STATIC_CONSENSUS
    // static_consensus won't sync registration contract data.
    return;
#endif
    XMETRICS_TIME_RECORD(XREC_STANDBY "on_timer_all_time");
    XCONTRACT_ENSURE(sender() == address(), "xrec_standby_pool_contract_t instance is triggled by others");
    XCONTRACT_ENSURE(address().value() == sys_contract_rec_standby_pool_addr, "xrec_standby_pool_contract_t instance is not triggled by xrec_standby_pool_contract_t");
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

    if (update_standby_result_store(registration_data, standby_result_store, activation_record)) {
        xdbg("[xrec_standby_pool_contract_t][on_timer] standby pool updated");
        serialization::xmsgpack_t<xstandby_result_store_t>::serialize_to_string_prop(*this, XPROPERTY_CONTRACT_STANDBYS_KEY, standby_result_store);
    }
#endif
}

NS_END2