// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xtableblock.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xcontract_helper.h"

NS_BEG3(top, xvm, consortium)

using namespace xvm;
using namespace xvm::xcontract;

struct xreward_cons_property_param_t {
    std::map<common::xgroup_address_t, data::system_contract::xgroup_cons_reward_t> map_node_reward_detail;
    data::system_contract::xaccumulated_reward_record accumulated_reward_record;
    std::map<common::xaccount_address_t, data::system_contract::xreg_node_info> map_nodes;
};

class xzec_consortium_reward_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xzec_consortium_reward_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xzec_consortium_reward_contract);

    explicit xzec_consortium_reward_contract(common::xnetwork_id_t const& network_id);

    /**
     * @brief 
     * 
     */
    void setup();

    /**
     * @brief 
     * 
     * @param onchain_timer_round 
     */
    void on_timer(const common::xlogic_time_t onchain_timer_round);
    
    /**
     * @brief 
     * 
     * @param table_info_str 
     */
    void on_receive_reward(std::string const& table_info_str);

    xcontract_base* clone() override { return new xzec_consortium_reward_contract(network_id()); }

    BEGIN_CONTRACT_WITH_PARAM(xzec_consortium_reward_contract)
    CONTRACT_FUNCTION_PARAM(xzec_consortium_reward_contract, on_timer);
    CONTRACT_FUNCTION_PARAM(xzec_consortium_reward_contract, on_receive_reward);
    END_CONTRACT_WITH_PARAM

private:

    /**
     * @brief 
     * 
     * @param table_info_str 
     * @param workload_str 
     * @param height_str 
     * @param workload_str_new 
     */
    void handle_reward_str(const std::string& table_info_str,
        const std::map<std::string, std::string>& workload_str,
        const std::string& height_str,
        std::map<std::string, std::string>& workload_str_new);

    /**
     * @brief 
     * 
     * @param group_workload 
     * @param workload_str 
     * @param workload_new 
     */
    void update_reward(const std::map<common::xgroup_address_t, data::system_contract::xgroup_cons_reward_t>& group_workload,
        const std::map<std::string, std::string>& workload_str,
        std::map<std::string, std::string>& workload_new);

    /**
     * @brief 
     * 
     * @param current_time 
     */
    void reward_dispatch(const common::xlogic_time_t current_time);

    /**
     * @brief Get the reward param object
     * 
     * @param current_time 
     * @param activation_time 
     * @param property_param 
     * @param issue_detail 
     */
    void get_reward_param(const common::xlogic_time_t current_time,
        common::xlogic_time_t& activation_time,
        xreward_cons_property_param_t& property_param,
        data::system_contract::xissue_detail_v2& issue_detail);

    /**
     * @brief 
     * 
     * @param node 
     * @param node_reward_detail 
     * @param reward_to_self 
     */
    void calc_node_rewards(data::system_contract::xreg_node_info const& node,
        std::map<common::xgroup_address_t, data::system_contract::xgroup_cons_reward_t> node_reward_detail,
        ::uint128_t& reward_to_self);

    /**
     * @brief 
     * 
     * @param property_param 
     * @param node_reward_detail 
     * @param table_node_reward_detail 
     * @param table_total_rewards 
     */
    void calc_table_rewards(xreward_cons_property_param_t& property_param,
        std::map<common::xaccount_address_t, ::uint128_t> const& node_reward_detail,
        std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>>& table_node_reward_detail,
        std::map<common::xaccount_address_t, ::uint128_t>& table_total_rewards);

    /**
     * @brief 
     * 
     * @param table_address 
     * @param account 
     * @param node_reward 
     * @param table_total_rewards 
     * @param table_node_reward_detail 
     */
    void calc_table_node_reward_detail(common::xaccount_address_t const& table_address,
        common::xaccount_address_t const& account,
        ::uint128_t node_reward,
        std::map<common::xaccount_address_t, ::uint128_t>& table_total_rewards,
        std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>>& table_node_reward_detail);

    /**
     * @brief 
     * 
     * @param current_time 
     * @param table_total_rewards 
     * @param table_node_reward_detail 
     */
    void dispatch_all_reward_v3(const common::xlogic_time_t current_time,
        std::map<common::xaccount_address_t, ::uint128_t>& table_total_rewards,
        std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>>& table_node_reward_detail);

    /**
     * @brief 
     * 
     * @param current_time 
     * @param record 
     * @param issue_detail 
     */
    void update_property(const uint64_t current_time,
        data::system_contract::xaccumulated_reward_record const& record,
        data::system_contract::xissue_detail_v2 const& issue_detail);

    /**
     * @brief 
     * 
     * @param timer_round 
     */
    void update_accumulated_issuance(uint64_t const timer_round);

    /**
     * @brief 
     * 
     * @param record 
     */
    void update_accumulated_record(const data::system_contract::xaccumulated_reward_record& record);

    /**
     * @brief 
     * 
     * @param issue_detail 
     */
    void update_issuance_detail(data::system_contract::xissue_detail_v2 const& issue_detail);

    /**
     * @brief 
     * 
     * @param onchain_timer_round 
     * @return true 
     * @return false 
     */
    bool reward_expire_check(const uint64_t onchain_timer_round);

    /**
     * @brief Get the accumulated record object
     * 
     * @param record 
     * @return int 
     */
    int get_accumulated_record(data::system_contract::xaccumulated_reward_record& record);

    /**
     * @brief 
     * 
     * @param table_id 
     * @param cur_read_height 
     */
    void update_table_height(const uint32_t table_id, const uint64_t cur_read_height);

    /**
     * @brief 
     * 
     */
    void clear_workload();

    /**
     * @brief 
     * 
     */
    void execute_task();

    /**
     * @brief 
     * 
     * @param cur_time 
     */
    void update_reg_contract_read_status(const uint64_t cur_time);

    /**
     * @brief 
     * 
     * @param current_time 
     * @param issue_time_length 
     * @param property_param 
     * @param issue_detail 
     * @param node_reward_detail 
     */
    void calc_nodes_rewards(common::xlogic_time_t const current_time,
        common::xlogic_time_t const issue_time_length,
        xreward_cons_property_param_t& property_param,
        data::system_contract::xissue_detail_v2& issue_detail,
        std::map<common::xaccount_address_t, ::uint128_t>& node_reward_detail);

    /**
     * @brief 
     * 
     * @param account 
     * @return common::xaccount_address_t 
     */
    common::xaccount_address_t calc_table_contract_address(common::xaccount_address_t const& account);

    /**
     * @brief Get the task id object
     * 
     * @return uint32_t 
     */
    uint32_t get_task_id();

    /**
     * @brief 
     * 
     * @param task_id 
     * @param onchain_timer_round 
     * @param contract 
     * @param action 
     * @param params 
     */
    void add_task(const uint32_t task_id,
        const uint64_t onchain_timer_round,
        const std::string& contract,
        const std::string& action,
        const std::string& params);

    /**
     * @brief Get the activated time object
     * 
     * @return uint64_t 
     */
    uint64_t get_activated_time() const;

    /**
     * @brief 
     * 
     * @param cur_time 
     * @param last_read_time 
     * @param last_read_height 
     * @param latest_height 
     * @param height_step_limitation 
     * @param timeout_limitation 
     * @param next_read_height 
     * @return true 
     * @return false 
     */
    bool check_reg_contract_read_time(const uint64_t cur_time,
        const uint64_t last_read_time,
        const uint64_t last_read_height,
        const uint64_t latest_height,
        const uint64_t height_step_limitation,
        const common::xlogic_time_t timeout_limitation,
        uint64_t& next_read_height);
};

NS_END3
