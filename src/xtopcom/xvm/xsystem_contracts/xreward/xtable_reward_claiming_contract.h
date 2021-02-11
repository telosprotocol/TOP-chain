// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xstake/xstake_algorithm.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG4(top, xvm, system_contracts, reward)

using namespace xstake;

class xtop_table_reward_claiming_contract final : public xcontract::xcontract_base {
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
    void recv_node_reward(uint64_t issuance_clock_height, std::map<std::string, top::xstake::uint128_t> const & rewards);
    /**
     * @brief receive voter dividend reward
     *
     * @param issuance_clock_height
     * @param rewards
     */
    void recv_voter_dividend_reward(uint64_t issuance_clock_height, std::map<std::string, top::xstake::uint128_t> const & rewards);

    /**
     * @brief update node reward record
     *
     * @param account
     * @param record
     * @return void
     */
    void update_working_reward_record(std::string const & account, xstake::xreward_node_record & record);
    /**
     * @brief update voter reward record
     *
     * @param account
     * @param record
     * @return void
     */
    void update_vote_reward_record(std::string const & account, xstake::xreward_record & record);

    /**
     * @brief Get the node reward record
     *
     * @param account
     * @param record
     * @return int32_t
     */
    int32_t get_working_reward_record(std::string const & account, xstake::xreward_node_record & record);
    /**
     * @brief Get the voter reward record
     *
     * @param account
     * @param record
     * @return int32_t
     */
    int32_t get_vote_reward_record(std::string const & account, xstake::xreward_record & record);

    /**
     * @brief update node reward
     *
     * @param node_account
     * @param issuance_clock_height
     * @param reward
     */
    void update(std::string const & node_account, uint64_t issuance_clock_height, top::xstake::uint128_t reward);
};
using xtable_reward_claiming_contract_t = xtop_table_reward_claiming_contract;

NS_END4
