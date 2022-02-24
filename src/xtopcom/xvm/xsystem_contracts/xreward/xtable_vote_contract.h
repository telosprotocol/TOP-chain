// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstake/xstake_algorithm.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xcontract_helper.h"

NS_BEG2(top, xstake)

using namespace xvm;
using namespace xvm::xcontract;

const std::size_t XVOTE_TRX_LIMIT = 1000;  // ~= 50K/(40+8)

class xtable_vote_contract : public xcontract_base {
    using xbase_t = xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtable_vote_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtable_vote_contract);

    explicit xtable_vote_contract(common::xnetwork_id_t const & network_id);

    using vote_info_map_t = std::map<std::string, uint64_t>;

    xcontract_base * clone() override { return new xtable_vote_contract(network_id()); }

    /**
     * @brief setup the contract
     *
     */
    void setup();

    BEGIN_CONTRACT_WITH_PARAM(xtable_vote_contract)
    CONTRACT_FUNCTION_PARAM(xtable_vote_contract, voteNode);
    CONTRACT_FUNCTION_PARAM(xtable_vote_contract, unvoteNode);
    END_CONTRACT_WITH_PARAM

private:
    /**
     * @brief vote the nodes
     *
     * @param vote_info vote info map, <node account, votes> format
     */
    void voteNode(vote_info_map_t const & vote_info);
    /**
     * @brief unvote the nodes
     *
     * @param vote_info vote info map, <node account, votes> format
     */
    void unvoteNode(vote_info_map_t const & vote_info);

    // vote related
    /**
     * @brief Set the vote info
     *
     * @param account voter account
     * @param vote_info vote info map, <node account, votes> format
     * @param b_vote true - vote, false - unvote
     */
    void set_vote_info(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool b_vote);

    /**
     * @brief report the stakes to the zec
     *
     */
    void commit_stake();

    /**
     * @brief add vote info
     *
     * @param account voter account
     * @param vote_info vote info map, <node account, votes> format
     * @param b_vote true - vote, false - unvote
     * @return true
     * @return false
     */
    void handle_votes(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool b_vote);

    /**
     * @brief add vote info
     *
     * @param adv_account account
     * @param votes votes to add/delete
     * @param votes_table votes table
     * @param b_vote true - vote, false - unvote
     * @param node_total_votes node total votes to calculate
     */
    void calc_advance_tickets(common::xaccount_address_t const & adv_account, uint64_t votes, std::map<std::string, uint64_t> & votes_table, bool b_vote, uint64_t & node_total_votes);

    /**
     * @brief Get the node info
     *
     * @param account node account
     * @param reg_node_info node registration info
     * @return int32_t
     */
    int32_t get_node_info(const common::xaccount_address_t & account, xreg_node_info & reg_node_info);

    /**
     * @brief write the vote info
     *
     * @param advance_account node account
     * @param tickets votes
     */
    void add_advance_tickets(common::xaccount_address_t const & advance_account, uint64_t tickets);

    /**
     * @brief Get the advance node votes
     *
     * @param advance_account node account
     * @return uint64_t votes
     */
    uint64_t get_advance_tickets(common::xaccount_address_t const & advance_account);

    /**
     * @brief update table votes detail
     *
     * @param account node account
     * @param votes_table votes table
     */
    void update_table_votes_detail(common::xaccount_address_t const & account, std::map<std::string, uint64_t> const & votes_table);

    /**
     * @brief get table votes detail
     *
     * @param account node account
     * @return votes table
     */
    std::map<std::string, uint64_t> get_table_votes_detail(common::xaccount_address_t const & account);

    /**
     * @brief check if we can report stakes to zec now
     *
     * @param onchain_timer_round chain timer round
     * @return true
     * @return false
     */
    bool is_expire(const uint64_t onchain_timer_round);

    /**
     * @brief report total votes to zec
     *
     */
    void commit_total_votes_num();

    /**
     * @brief split table vote report tx then report
     *
     * @param report_contract  the target report contract
     * @param report_func  the target report function
     * @param report_content  the report content
     *
     */
    void split_and_report(std::string const& report_contract, std::string const& report_func, std::map<std::string, std::string> const& report_content);

    /**
     *
     * @brief split algorithm helper
     *
     * @param report_contect the report content
     * @param limit the vote trx limit
     *
     * @return std::vector<std::map<std::string, std::string>>  splited map array
     *
     */
    std::vector<std::map<std::string, std::string>> trx_split_helper(std::map<std::string, std::string> const& report_content, std::size_t limit = XVOTE_TRX_LIMIT);

};

NS_END2
