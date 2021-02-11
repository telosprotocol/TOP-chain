// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvm/xcontract_helper.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xdata/xtableblock.h"
#include "xstake/xstake_algorithm.h"

NS_BEG2(top, xstake)

using namespace xvm;
using namespace xvm::xcontract;

enum class xreward_type : std::uint8_t { edge_reward, archive_reward, validator_reward, auditor_reward, vote_reward, governance_reward };

class xzec_reward_contract : public xcontract_base {
    using xbase_t = xcontract_base;
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xzec_reward_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xzec_reward_contract);

    explicit
    xzec_reward_contract(common::xnetwork_id_t const & network_id);

    xcontract_base*  clone() override {return new xzec_reward_contract(network_id());}

    /**
     * @brief setupo the contract
     *
     */
    void        setup();

    /**
     * @brief timer processing functions
     *
     * @param onchain_timer_round chain timer round
     */
    void on_timer(const uint64_t onchain_timer_round);

    /**
     * @brief calaute reward from workload contract
     *
     * @param timer_round  the timer round
     * @param workload_str the workload report from workload contract
     */
    void calculate_reward(common::xlogic_time_t timer_round, std::string const& workload_str);

    BEGIN_CONTRACT_WITH_PARAM(xzec_reward_contract)
        CONTRACT_FUNCTION_PARAM(xzec_reward_contract, on_timer);
        CONTRACT_FUNCTION_PARAM(xzec_reward_contract, calculate_reward);
    END_CONTRACT_WITH_PARAM

private:
    /**
     * @brief check if we can calculate and dispatch rewards now
     *
     * @param onchain_timer_round chain timer round
     * @return bool
     */
    bool     reward_is_expire(const uint64_t onchain_timer_round);

    /**
     * @brief check if we can calculate and dispatch rewards now
     *
     * @param onchain_timer_round chain timer round
     * @return bool
     */
    bool     reward_is_expire_v2(const uint64_t onchain_timer_round);

    /**
     * @brief call calc_nodes_rewards_v2 and dispatch_all_reward
     *
     * @param onchain_timer_round chain timer round
     * @param workload_str the workload str
     */
    void        reward(const uint64_t onchain_timer_round, std::string const& workload_str);

    /**
     * @brief calculate nodes rewards
     *
     * @param nodes_rewards nodes rewards
     * @param contract_rewards contract rewards
     * @param contract_auditor_vote_rewards contract auditor vote rewards
     * @param onchain_timer_round chian timer round
     * @param workload_str the workload str
     */
    void        calc_nodes_rewards(std::map<std::string, std::map<std::string, top::xstake::uint128_t >> & table_nodes_rewards,
                                              std::map<std::string, top::xstake::uint128_t> & contract_rewards,
                                              std::map<std::string, std::map<std::string, top::xstake::uint128_t >> & contract_auditor_vote_rewards,
                                              const uint64_t onchain_timer_round, std::string const& workload_str);

    /**
     * @brief calculate nodes rewards
     *
     * @param nodes_rewards nodes rewards
     * @param contract_rewards contract rewards
     * @param contract_auditor_vote_rewards contract auditor vote rewards
     * @param onchain_timer_round chian timer round
     * @param workload_str the workload str
     */
    void        calc_nodes_rewards_v2(std::map<std::string, std::map<std::string, top::xstake::uint128_t >> & table_nodes_rewards,
                                              std::map<std::string, top::xstake::uint128_t> & contract_rewards,
                                              std::map<std::string, std::map<std::string, top::xstake::uint128_t >> & contract_auditor_vote_rewards,
                                              const uint64_t onchain_timer_round, std::string const& workload_str);
    /**
     * @brief calculate nodes rewards
     *
     * @param nodes_rewards nodes rewards
     * @param contract_rewards contract rewards
     * @param contract_auditor_vote_rewards contract auditor vote rewards
     * @param onchain_timer_round chian timer round
     */
    void        calc_nodes_rewards_v3(std::map<std::string, std::map<std::string, top::xstake::uint128_t >> & table_nodes_rewards,
                                              std::map<std::string, top::xstake::uint128_t> & contract_rewards,
                                              std::map<std::string, std::map<std::string, top::xstake::uint128_t >> & contract_auditor_vote_rewards,
                                              const uint64_t onchain_timer_round);

    /**
     * @brief calculate nodes rewards
     *
     * @param nodes_rewards nodes rewards
     * @param contract_rewards contract rewards
     * @param contract_auditor_vote_rewards contract auditor vote rewards
     * @param onchain_timer_round chian timer round
     */
    void        calc_nodes_rewards_v4(std::map<std::string, std::map<std::string, top::xstake::uint128_t >> & table_nodes_rewards,
                                              std::map<std::string, top::xstake::uint128_t> & contract_rewards,
                                              std::map<std::string, std::map<std::string, top::xstake::uint128_t >> & contract_auditor_vote_rewards,
                                              const uint64_t onchain_timer_round);

    /**
     * @brief
     *
     * @param nodes_rewards nodes rewards
     * @param contract_total_vote_awards contract total vote rewards
     * @param contract_auditor_vote_rewards contract auditor vote rewards
     * @param onchain_timer_round chian timer round
     * @return true
     * @return false
     */
    void        dispatch_all_reward(std::map<std::string, std::map<std::string, top::xstake::uint128_t> > const & table_nodes_rewards,
                                               std::map<std::string, top::xstake::uint128_t> const & contract_rewards,
                                               std::map<std::string, std::map<std::string, top::xstake::uint128_t> > const & contract_auditor_vote_rewards,
                                               uint64_t onchain_timer_round);
    /**
     * @brief
     *
     * @param nodes_rewards nodes rewards
     * @param contract_total_vote_awards contract total vote rewards
     * @param contract_auditor_vote_rewards contract auditor vote rewards
     * @param onchain_timer_round chian timer round
     * @return true
     * @return false
     */
    void dispatch_all_reward_v2(std::map<std::string, std::map<std::string, top::xstake::uint128_t>> const & table_nodes_rewards,
                                std::map<std::string, top::xstake::uint128_t> const & contract_rewards,
                                std::map<std::string, std::map<std::string, top::xstake::uint128_t>> const & contract_auditor_vote_rewards,
                                uint64_t onchain_timer_round);

    /**
     * @brief Get the task id
     *
     * @return uint32_t
     */
    uint32_t    get_task_id();

    /**
     * @brief add task
     *
     * @param task_id task id
     * @param onchain_timer_round chain timer round
     * @param contract contract address
     * @param action action to execute
     * @param params action parameters
     */
    void        add_task(const uint32_t task_id, const uint64_t onchain_timer_round, const std::string &contract, const std::string &action, const std::string &params);

    /**
     * @brief execute all tasks
     *
     */
    void        execute_task();

    /**
     * @brief print all tasks
     *
     */
    void        print_tasks();

    /**
     * @brief calculate issuance
     *
     * @param total_height chain timer height
     * @return uint64_t issuance
     */
    top::xstake::uint128_t    calc_issuance(uint64_t total_height);
    /**
     * @brief Get the accumulated record
     *
     * @param record accumulated issuance record
     * @return int
     */
    int         get_accumulated_record(xaccumulated_reward_record & record);
    /**
     * @brief update accumulated issuance record
     *
     * @param record accumulated issuance record
     * @return void
     */
    void        update_accumulated_record(const xaccumulated_reward_record & record);
    /**
     * @brief Get the reward of specified reward_type
     *
     * @param issuance total issuance of this round
     * @param reward_type reward type
     * @return top::xstake::uint128_t reward
     */
    top::xstake::uint128_t    get_reward(top::xstake::uint128_t issuance, xreward_type reward_type);

    /**
     * @brief Get the activated time
     *
     * @return uint64_t activation time
     */
    uint64_t    get_activated_time() const;
    /**
     * @brief update accumulated issuance
     *
     * @param issuance issuance of this round
     * @param timer_round chain timer round
     */
    void        update_accumulated_issuance(uint64_t const issuance, uint64_t const timer_round);

    /**
     * @brief
     *
     * @param cur_time
     */
    void        update_reg_contract_read_status(const uint64_t cur_time);

    /**
     * @brief Get the node info object
     *
     * @param map_nodes
     * @param account
     * @param node
     * @return int
     */
    int         get_node_info(const std::map<std::string, xreg_node_info> & map_nodes, const std::string & account, xreg_node_info & node);

    /**
     * @brief receive workload
     *
     * @param workload_str
     */
    void        on_receive_workload(std::string const& workload_str);

    /**
     * @brief save workload
     *
     * @param auditor
     * @param cluster_id
     * @param leader_count
     */
    void        add_cluster_workload(bool auditor, std::string const& cluster_id, std::map<std::string, uint32_t> const& leader_count);

    /**
     * @brief clear workload
     *
     */
    void        clear_workload();


    /**
     * @brief update issuance detail
     *
     * @param issue_detail
     */
    void update_issuance_detail(xissue_detail const & issue_detail);
};

NS_END2
