// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xserialization/xserialization.h"

NS_BEG3(top, xvm, serialization)

std::map<common::xaccount_address_t, std::string> const sys_addr_to_metrics_enum_get_property_time{
    {common::xaccount_address_t{sys_contract_rec_elect_rec_addr}, "election_rec_elect_rec_get_property_time"},
    {common::xaccount_address_t{sys_contract_rec_elect_archive_addr}, "election_rec_elect_archive_get_property_time"},
    {common::xaccount_address_t{sys_contract_rec_elect_edge_addr}, "election_rec_elect_edge_get_property_time"},
    {common::xaccount_address_t{sys_contract_rec_elect_zec_addr}, "election_rec_elect_zec_get_property_time"},
    {common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, "rec_standby_pool_get_property_time"},
    {common::xaccount_address_t{sys_contract_zec_elect_consensus_addr}, "election_zec_elect_consensus_get_property_time"},
    {common::xaccount_address_t{sys_contract_zec_group_assoc_addr}, "election_zec_group_association_get_property_time"},
    {common::xaccount_address_t{sys_contract_rec_elect_fullnode_addr}, "election_rec_elect_fullnode_get_property_time"}
};

std::map<common::xaccount_address_t, std::string> const sys_addr_to_metrics_enum_set_property_time{
    {common::xaccount_address_t{sys_contract_rec_elect_rec_addr}, "election_rec_elect_rec_set_property_time"},
    {common::xaccount_address_t{sys_contract_rec_elect_archive_addr}, "election_rec_elect_archive_set_property_time"},
    {common::xaccount_address_t{sys_contract_rec_elect_edge_addr}, "election_rec_elect_edge_set_property_time"},
    {common::xaccount_address_t{sys_contract_rec_elect_zec_addr}, "election_rec_elect_zec_set_property_time"},
    {common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, "rec_standby_pool_set_property_time"},
    {common::xaccount_address_t{sys_contract_zec_elect_consensus_addr}, "election_zec_elect_consensus_set_property_time"},
    {common::xaccount_address_t{sys_contract_zec_group_assoc_addr}, "election_zec_group_association_set_property_time"},
    {common::xaccount_address_t{sys_contract_rec_elect_fullnode_addr}, "election_rec_elect_fullnode_set_property_time"}
};

std::map<common::xaccount_address_t, std::string> const sys_addr_to_metrics_enum_get_property_size{
    {common::xaccount_address_t{sys_contract_rec_elect_rec_addr}, "election_rec_elect_rec_get_property_size"},
    {common::xaccount_address_t{sys_contract_rec_elect_archive_addr}, "election_rec_elect_archive_get_property_size"},
    {common::xaccount_address_t{sys_contract_rec_elect_edge_addr}, "election_rec_elect_edge_get_property_size"},
    {common::xaccount_address_t{sys_contract_rec_elect_zec_addr}, "election_rec_elect_zec_get_property_size"},
    {common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, "rec_standby_pool_get_property_size"},
    {common::xaccount_address_t{sys_contract_zec_elect_consensus_addr}, "election_zec_elect_consensus_get_property_size"},
    {common::xaccount_address_t{sys_contract_zec_group_assoc_addr}, "election_zec_group_association_get_property_size"},
    {common::xaccount_address_t{sys_contract_rec_elect_fullnode_addr}, "election_rec_elect_fullnode_get_property_size"},
};

std::map<common::xaccount_address_t, std::string> const sys_addr_to_metrics_enum_set_property_size{
    {common::xaccount_address_t{sys_contract_rec_elect_rec_addr}, "election_rec_elect_rec_set_property_size"},
    {common::xaccount_address_t{sys_contract_rec_elect_archive_addr}, "election_rec_elect_archive_set_property_size"},
    {common::xaccount_address_t{sys_contract_rec_elect_edge_addr}, "election_rec_elect_edge_set_property_size"},
    {common::xaccount_address_t{sys_contract_rec_elect_zec_addr}, "election_rec_elect_zec_set_property_size"},
    {common::xaccount_address_t{sys_contract_rec_standby_pool_addr}, "rec_standby_pool_set_property_size"},
    {common::xaccount_address_t{sys_contract_zec_elect_consensus_addr}, "election_zec_elect_consensus_set_property_size"},
    {common::xaccount_address_t{sys_contract_zec_group_assoc_addr}, "election_zec_group_association_set_property_size"},
    {common::xaccount_address_t{sys_contract_rec_elect_fullnode_addr}, "election_rec_elect_fullnode_set_property_size"},
};

NS_END3
