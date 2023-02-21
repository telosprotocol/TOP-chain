// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xfulltableblock_account_data.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvledger/xvcnode.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG3(top, xvm, consortium)

/**
 * @brief the table slash contract
 *
 */
class xtable_statistic_cons_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtable_statistic_cons_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtable_statistic_cons_contract);

    explicit xtable_statistic_cons_contract(common::xnetwork_id_t const& network_id);

    xcontract_base*
    clone() override
    {
        return new xtable_statistic_cons_contract(network_id());
    }

    void setup();

    /**
     * @brief  collect the statistic info on tables
     *
     * @param statistic_info  the info to collect
     * @param block_height the fullblock height
     */
    void on_collect_statistic_info_cons(top::data::xstatistics_cons_data_t const& statistic_data, top::data::xfulltableblock_statistic_accounts const& statistic_accounts, uint64_t block_height);
    /**
     * @brief report the summarized statistic info
     * @param timestamp  the clock timer
     *
     */
    void report_summarized_statistic_info(common::xlogic_time_t timestamp);

    BEGIN_CONTRACT_WITH_PARAM(xtable_statistic_cons_contract)
    CONTRACT_FUNCTION_PARAM(xtable_statistic_cons_contract, on_collect_statistic_info_cons);
    CONTRACT_FUNCTION_PARAM(xtable_statistic_cons_contract, report_summarized_statistic_info);
    END_CONTRACT_WITH_PARAM

private:
    /**
     * @brief process workload statistic data
     *
     * @param  xstatistics_data_t  statistic data
     * @param  tgas tgas
     *
     */
    void process_reward_statistic_data(top::data::xstatistics_cons_data_t const& statistic_data, top::data::xfulltableblock_statistic_accounts const& statistic_accounts);

    /**
     * @brief get_workload
     *
     * @param  xstatistics_data_t  statistic data
     *
     */
    std::map<common::xgroup_address_t, data::system_contract::xgroup_cons_reward_t> calc_reward_from_data(top::data::xstatistics_cons_data_t const& statistic_data,
                                                            top::data::xfulltableblock_statistic_accounts const& statistic_accounts);

    /**
     * @brief 
     * 
     * @param group_workload 
     */
    void update_reward(std::map<common::xgroup_address_t, data::system_contract::xgroup_cons_reward_t> const& group_workload);

    /**
     * @brief Get the reward object
     * 
     * @param group_address 
     * @return data::system_contract::xgroup_cons_reward_t 
     */
    data::system_contract::xgroup_cons_reward_t get_reward(common::xgroup_address_t const& group_address);

    /**
     * @brief 
     * 
     * @param group_address 
     * @param group_workload 
     */
    void save_reward(common::xgroup_address_t const& group_address, data::system_contract::xgroup_cons_reward_t const& group_workload);

    /**
     * @brief upload_reward
     */
    void upload_reward();
};

NS_END3
