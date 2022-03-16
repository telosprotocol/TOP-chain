// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG4(top, xvm, system_contracts, reward)

class xtop_table_reward_claiming_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_table_reward_claiming_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_table_reward_claiming_contract);

    explicit xtop_table_reward_claiming_contract(common::xnetwork_id_t const & network_id);

    /**
     * @brief setup the contract
     *
     */
    void setup() override;

    xcontract::xcontract_base * clone() override;

    BEGIN_CONTRACT_WITH_PARAM(xtop_table_reward_claiming_contract)
    CONTRACT_FUNCTION_PARAM(xtop_table_reward_claiming_contract, claimNodeReward);
    CONTRACT_FUNCTION_PARAM(xtop_table_reward_claiming_contract, claimVoterDividend);
    CONTRACT_FUNCTION_PARAM(xtop_table_reward_claiming_contract, recv_node_reward);
    CONTRACT_FUNCTION_PARAM(xtop_table_reward_claiming_contract, recv_voter_dividend_reward);
    END_CONTRACT_WITH_PARAM

private:
    /**
     * \brief claim node reward. node reward includes:
     *        1. workload reward + vote reword if the account acts as an auditor
     *        2. workload reward if the account acts as a validator or an edge or an archive
     *        3. tcc reward if the account is a TCC account
     */
    void claimNodeReward();

    /**
     * \brief claim voter dividend reward.
     */
    void claimVoterDividend();

    /**
     * @brief receive node reward
     *
     * @param issuance_clock_height
     * @param rewards
     */
    void recv_node_reward(uint64_t issuance_clock_height, std::map<std::string, ::uint128_t> const & rewards);
    /**
     * @brief receive voter dividend reward
     *
     * @param issuance_clock_height
     * @param rewards
     */
    void recv_voter_dividend_reward(uint64_t issuance_clock_height, std::map<std::string, ::uint128_t> const & rewards);

    /**
     * @brief update node reward record
     *
     * @param account
     * @param record
     * @return void
     */
    void update_working_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_node_record const & record);
    /**
     * @brief update voter reward record
     *
     * @param account
     * @param record
     * @return void
     */
    void update_vote_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_record const & record);

    /**
     * @brief Get the node reward record
     *
     * @param account
     * @param record
     * @return int32_t
     */
    int32_t get_working_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_node_record & record);
    /**
     * @brief Get the voter reward record
     *
     * @param account
     * @param record
     * @return int32_t
     */
    int32_t get_vote_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_record & record);

    /**
     * @brief update node reward
     *
     * @param node_account
     * @param issuance_clock_height
     * @param reward
     */
    void update(common::xaccount_address_t const & node_account, uint64_t issuance_clock_height, ::uint128_t reward);

    /**
     * @brief add node reward
     *
     * @param issuance_clock_height
     * @param votes_table
     * @param rewards
     * @param adv_votes
     * @param record
     */
    void add_voter_reward(uint64_t issuance_clock_height,
                          std::map<std::string, uint64_t> & votes_table,
                          std::map<std::string, ::uint128_t> const & rewards,
                          std::map<std::string, std::string> const & adv_votes,
                          data::system_contract::xreward_record & record);
    
    /**
     * @brief sub voter reward
     *
     * @param cur_time
     * @param reward_record
     * @param reward
     */
    void sub_voter_record(uint64_t cur_time, data::system_contract::xreward_record & reward_record);
};
using xtable_reward_claiming_contract_t = xtop_table_reward_claiming_contract;

NS_END4
