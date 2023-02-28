
// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xsystem_contract/xdata_structures.h"

NS_BEG3(top, tests, rec_standby)

class xtop_test_registration_data_manager_fixture {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_test_registration_data_manager_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_registration_data_manager_fixture);
    XDECLARE_DEFAULTED_DESTRUCTOR(xtop_test_registration_data_manager_fixture);

    std::map<common::xnode_id_t, data::system_contract::xreg_node_info> m_registration_data;

    bool add_reg_info(data::system_contract::xreg_node_info const & node_info);

    bool update_reg_info(data::system_contract::xreg_node_info const & node_info);

    void change_miner_type(common::xnode_id_t const & node_id, common::xminer_type_t const & new_role_type);

    void change_account_mortgage(common::xnode_id_t const & node_id, uint64_t new_account_mortgage);

    void change_public_key(common::xnode_id_t const & node_id, top::xpublic_key_t const & new_public_key);
};

NS_END3
