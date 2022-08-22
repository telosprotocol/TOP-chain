// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xelection_result_property.h"

#include "xconfig/xconfig_register.h"
#include "xdata/xnative_contract_address.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xproperty.h"

NS_BEG3(top, data, election)

using common::xaccount_address_t;
std::vector<std::string> get_property_name_by_addr(common::xaccount_address_t const & sys_contract_addr) {
    std::vector<common::xaccount_address_t> sys_addr{rec_elect_rec_contract_address,
                                                     rec_elect_zec_contract_address,
                                                     zec_elect_consensus_contract_address,
                                                     rec_elect_archive_contract_address,
                                                     rec_elect_exchange_contract_address,
                                                     rec_elect_edge_contract_address,
                                                     rec_elect_fullnode_contract_address,
                                                     zec_elect_eth_contract_address,
                                                     zec_elect_relay_contract_address,
                                                     relay_make_block_contract_address};
    assert(std::find(sys_addr.begin(), sys_addr.end(), sys_contract_addr) != sys_addr.end());

    std::vector<std::string> property_name;
    if (sys_contract_addr == sys_addr[0] || sys_contract_addr == sys_addr[1]) {
        property_name.push_back(get_property_by_group_id(common::xcommittee_group_id));
    } else if (sys_contract_addr == sys_addr[2]) {
        auto const auditor_group_count = XGET_CONFIG(auditor_group_count);
        for (auto index = common::xauditor_group_id_value_begin; index <= auditor_group_count; ++index) {
            property_name.push_back(get_property_by_group_id(common::xgroup_id_t{ index }));
        }
    } else if (sys_contract_addr == sys_addr[3]) { // rec_elect_archive_contract_address
        auto const legacy_archive_group_count = XGET_CONFIG(legacy_archive_group_count); // ! can't not delete legacy exchange group value. (used in rec_elect_archive setup)
        for (auto index = common::xarchive_group_id_value_begin; index <= legacy_archive_group_count; ++index) {
            property_name.push_back(get_property_by_group_id(common::xgroup_id_t{ index }));
        }
    } else if (sys_contract_addr == sys_addr[4]) { // rec_elect_exchange_contract_address
        property_name.push_back(get_property_by_group_id(common::xexchange_group_id));
    } else if (sys_contract_addr == sys_addr[5] || sys_contract_addr == sys_addr[6] || sys_contract_addr == sys_addr[7] || sys_contract_addr == sys_addr[8] ||
               sys_contract_addr == sys_addr[9]) {
        property_name.push_back(get_property_by_group_id(common::xdefault_group_id));
    }
    assert(!property_name.empty());
    return property_name;
}

std::string get_property_by_group_id(common::xgroup_id_t const & group_id) {
    return std::string(data::XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_" + std::to_string(group_id.value());
}

NS_END3
