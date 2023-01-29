// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

#include <string>

NS_BEG2(top, xstake)

const std::size_t XVOTE_TRX_LIMIT = 1000;  // ~= 50K/(40+8)

std::string calc_voter_tickets_storage_property_name(common::xaccount_address_t const & voter);

class xtable_vote_contract : public xvm::xcontract::xcontract_base {
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
    CONTRACT_FUNCTION_PARAM(xtable_vote_contract, voteNode)
    CONTRACT_FUNCTION_PARAM(xtable_vote_contract, unvoteNode)
    CONTRACT_FUNCTION_PARAM(xtable_vote_contract, on_timer)
    END_CONTRACT_WITH_PARAM

    std::string flag() const;
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, uint64_t>> tickets_data(std::string const & property_name) const;
    std::map<common::xaccount_address_t, std::map<common::xlogic_time_t, std::map<common::xaccount_address_t, uint64_t>>> ineffective_data() const;
    std::map<common::xaccount_address_t, uint64_t> table_tickets_data() const;

    static std::string const flag_upload_tickets_10900;
    static std::string const flag_withdraw_tickets_10900;
    static std::string const flag_reset_tickets;
    static std::string const flag_upload_tickets_10901;
    static std::string const flag_withdraw_tickets_10901;
    static std::string const flag_upload_tickets_10902;
    static std::string const flag_withdraw_tickets_10902;

#if defined(XBUILD_CI) || defined(XBUILD_GALILEO) || defined(XBUILD_BOUNTY)
    static constexpr common::xlogic_time_t ineffective_period{1};
#elif defined(XBUILD_DEV)
    static constexpr common::xlogic_time_t ineffective_period{90};
#else
    static constexpr common::xlogic_time_t ineffective_period{8640};
#endif

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

    /**
     * @brief schedule
     *
     * @param timestamp time
     */
    void on_timer(common::xlogic_time_t const timestamp);

    // vote related
    /**
     * @brief Set the vote info
     *
     * @param account voter account
     * @param vote_info vote info map, <node account, votes> format
     * @param b_vote true - vote, false - unvote
     */
    void set_vote_info(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool b_vote);
    void set_vote_info_v2(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool b_vote);

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
     */
    void handle_votes(common::xaccount_address_t const & account, vote_info_map_t const & vote_info, bool b_vote, bool check_tickets_recver);

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
    int32_t get_node_info(const common::xaccount_address_t & account, data::system_contract::xreg_node_info & reg_node_info);

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
     * @param report_content the report content
     * @param limit the vote trx limit
     *
     * @return std::vector<std::map<std::string, std::string>>  splited map array
     *
     */
    std::vector<std::map<std::string, std::string>> trx_split_helper(std::map<std::string, std::string> const& report_content, std::size_t limit = XVOTE_TRX_LIMIT);

    std::map<common::xaccount_address_t, xtable_vote_contract::vote_info_map_t> get_and_update_all_effective_votes_of_all_account(uint64_t const timestamp);
    std::map<std::uint64_t, vote_info_map_t> get_all_time_ineffective_votes(common::xaccount_address_t const & account);
    void set_all_time_ineffective_votes(common::xaccount_address_t const & account, std::map<std::uint64_t, vote_info_map_t> const & all_time_ineffective_votes);
    void add_all_time_ineffective_votes(uint64_t const timestamp, vote_info_map_t const & vote_info, std::map<std::uint64_t, vote_info_map_t> & all_time_ineffective_votes);
    void del_all_time_ineffective_votes(vote_info_map_t & vote_info_to_del, std::map<std::uint64_t, vote_info_map_t> & all_time_ineffective_votes);

    bool reset_v10901(std::string const & flag,
                      std::map<common::xaccount_address_t, vote_info_map_t> const & contract_ticket_reset_data,
                      std::vector<common::xaccount_address_t> const & contract_ticket_clear_data);

    bool reset_v10902(std::string const & flag,
                      std::map<common::xaccount_address_t, vote_info_map_t> const & contract_ticket_reset_data,
                      std::vector<common::xaccount_address_t> const & contract_ticket_clear_data);

    void read_tickets_property_raw_data(std::string const & property_name, std::vector<std::string> & raw_data) const;
    vote_info_map_t get_origin_pollable_reset_data(std::vector<std::string> const & serialized_origin_data) const;
    void reset_table_tickets_data(vote_info_map_t const & reset_data);
    void reset_table_tickets_data();
};

NS_END2
