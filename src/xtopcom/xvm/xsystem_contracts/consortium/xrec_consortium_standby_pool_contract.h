// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xrole_type.h"
#include "xdata/xelection/xstandby_network_storage_result.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

#include <string>

NS_BEG3(top, xvm, consortium)

class xtop_rec_consortium_standby_pool_contract final : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_rec_consortium_standby_pool_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_rec_consortium_standby_pool_contract);

    explicit xtop_rec_consortium_standby_pool_contract(common::xnetwork_id_t const & network_id);

    xcontract::xcontract_base * clone() override { return new xtop_rec_consortium_standby_pool_contract(network_id()); }

    void setup();

    BEGIN_CONTRACT_WITH_PARAM(xtop_rec_consortium_standby_pool_contract)
    CONTRACT_FUNCTION_PARAM(xtop_rec_consortium_standby_pool_contract, nodeJoinNetwork2);
    CONTRACT_FUNCTION_PARAM(xtop_rec_consortium_standby_pool_contract, on_timer);
    END_CONTRACT_WITH_PARAM

private:
    void nodeJoinNetwork2(common::xaccount_address_t const & node_id,
                         common::xnetwork_id_t const & joined_network_id,
#if defined XENABLE_MOCK_ZEC_STAKE
                         common::xminer_type_t role_type,
                         std::string const & pubkey,
                         uint64_t const stake,
#endif
                         std::string const & program_version);

    bool nodeJoinNetworkImpl(std::string const & program_version,
                             data::system_contract::xreg_node_info const & node,
                             data::election::xstandby_result_store_t & standby_result_store);

    void on_timer(common::xlogic_time_t const current_time);

    bool update_standby_result_store(std::map<common::xnode_id_t, data::system_contract::xreg_node_info> const & registration_data,
                                     data::election::xstandby_result_store_t & standby_result_store,
                                     data::system_contract::xactivation_record const & activation_record,
                                     common::xlogic_time_t const current_logic_time);

    bool update_standby_node(top::data::system_contract::xreg_node_info const & reg_node,
                             top::data::election::xstandby_node_info_t & standby_node_info,
                             common::xlogic_time_t const current_logic_time) const;

    bool update_activated_state(data::election::xstandby_network_storage_result_t & standby_network_storage_result,
                                data::system_contract::xactivation_record const & activation_record);

    /**
     * @brief  
     * @note   
     * @param  account_str: 
     * @retval 
     */
    bool check_node_valid(std::string const &account_str);
};
using xrec_consortium_standby_pool_contract_t = xtop_rec_consortium_standby_pool_contract;

NS_END3