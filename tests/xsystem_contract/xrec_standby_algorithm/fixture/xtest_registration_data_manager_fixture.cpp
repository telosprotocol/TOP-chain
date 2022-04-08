// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xsystem_contract/xrec_standby_algorithm/xtest_registration_data_manager_fixture.h"

NS_BEG3(top, tests, rec_standby)

bool xtop_test_registration_data_manager_fixture::add_reg_info(data::system_contract::xreg_node_info const & node_info) {
    return m_registration_data.insert({common::xnode_id_t{node_info.m_account}, node_info}).second;
}

bool xtop_test_registration_data_manager_fixture::update_reg_info(data::system_contract::xreg_node_info const & node_info) {
    common::xnode_id_t xnode_id{node_info.m_account};
    if (m_registration_data.find(xnode_id) == m_registration_data.end())
        return false;
    m_registration_data.at(xnode_id) = node_info;
    return true;
}

void xtop_test_registration_data_manager_fixture::change_miner_type(common::xnode_id_t const & node_id, common::xminer_type_t const & new_role_type) {
    assert(m_registration_data.find(node_id) != m_registration_data.end());
    m_registration_data[node_id].miner_type(new_role_type);
}

void xtop_test_registration_data_manager_fixture::change_account_mortgage(common::xnode_id_t const & node_id, uint64_t new_account_mortgage) {
    assert(m_registration_data.find(node_id) != m_registration_data.end());
    m_registration_data[node_id].m_account_mortgage = new_account_mortgage;
}

void xtop_test_registration_data_manager_fixture::change_public_key(common::xnode_id_t const & node_id, top::xpublic_key_t const & new_public_key) {
    assert(m_registration_data.find(node_id) != m_registration_data.end());
    m_registration_data[node_id].consensus_public_key = top::xpublic_key_t{new_public_key};
}

NS_END3
