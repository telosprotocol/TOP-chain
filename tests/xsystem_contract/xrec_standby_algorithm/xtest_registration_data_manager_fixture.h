
// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xstake/xstake_algorithm.h"

NS_BEG3(top, tests, rec_standby)

using top::data::election::xstandby_network_result_t;
using top::data::election::xstandby_node_info_t;

class xtop_test_registration_data_manager_fixture {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_test_registration_data_manager_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_registration_data_manager_fixture);
    XDECLARE_DEFAULTED_DESTRUCTOR(xtop_test_registration_data_manager_fixture);

    std::map<common::xnode_id_t, xstake::xreg_node_info> m_registration_data;

    bool add_reg_info(xstake::xreg_node_info const & node_info);

    bool update_reg_info(xstake::xreg_node_info const & node_info);

    void change_role_type(common::xnode_id_t const & node_id, common::xminer_type_t const & new_role_type);

    void change_account_mortgage(common::xnode_id_t const & node_id, uint64_t new_account_mortgage);

    void change_public_key(common::xnode_id_t const & node_id, top::xpublic_key_t const & new_public_key);

    //... might need more of it.

    // reg_node_info:

    // std::string m_account{""};
    // uint64_t m_account_mortgage{0};
    // common::xminer_type_t m_registered_role{common::xminer_type_t::invalid};
    // uint64_t m_vote_amount{0};
    // uint64_t m_auditor_credit_numerator{0};
    // uint64_t m_auditor_credit_denominator{1000000};
    // uint64_t m_validator_credit_numerator{0};
    // uint64_t m_validator_credit_denominator{1000000};

    // uint m_support_ratio_numerator{0};  // dividends to voters
    // uint m_support_ratio_denominator{100};
    // common::xlogic_time_t m_last_update_time{0};
    // bool m_genesis_node{false};
    // std::set<uint32_t> m_network_ids;
    // std::string nickname;
    // xpublic_key_t consensus_public_key;
};

NS_END3