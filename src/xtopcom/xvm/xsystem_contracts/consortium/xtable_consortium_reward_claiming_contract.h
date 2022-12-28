// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG3(top, xvm, consortium)


class xtop_table_consortium_reward_claiming_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_table_consortium_reward_claiming_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_table_consortium_reward_claiming_contract);

    explicit xtop_table_consortium_reward_claiming_contract(common::xnetwork_id_t const & network_id);

    /**
     * @brief setup the contract
     *
     */
    void setup() override;

    xcontract::xcontract_base * clone() override;

    BEGIN_CONTRACT_WITH_PARAM(xtop_table_consortium_reward_claiming_contract)
    CONTRACT_FUNCTION_PARAM(xtop_table_consortium_reward_claiming_contract, claimNodeReward);
    CONTRACT_FUNCTION_PARAM(xtop_table_consortium_reward_claiming_contract, recv_node_reward);
    END_CONTRACT_WITH_PARAM

private:

    /**
     * @brief  
     * @note   
     * @retval None
     */
    void claimNodeReward();

    /**
     * @brief receive node reward
     *
     * @param issuance_clock_height
     * @param rewards
     */
    void recv_node_reward(uint64_t issuance_clock_height, std::map<std::string, ::uint128_t> const & rewards);

    /**
     * @brief update node reward record
     *
     * @param account
     * @param record
     * @return void
     */
    void update_working_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_node_record const & record);

    /**
     * @brief Get the node reward record
     *
     * @param account
     * @param record
     * @return int32_t
     */
    int32_t get_working_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_node_record & record);

    /**
     * @brief update node reward
     *
     * @param node_account
     * @param issuance_clock_height
     * @param reward
     */
    void update(common::xaccount_address_t const & node_account, uint64_t issuance_clock_height, ::uint128_t reward);

    
};
using  xtable_consortium_reward_claiming_contract_t = xtop_table_consortium_reward_claiming_contract;

NS_END3

