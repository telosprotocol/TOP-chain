// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xdata/xslash.h"
#include "xdata/xblock_statistics_data.h"
#include "xvledger/xvcnode.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"

NS_BEG3(top, xvm, xcontract)

/**
 * @brief the zec slash contract
 *
 */
class xzec_slash_info_contract : public xcontract_base {
    using xbase_t = xcontract_base;
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xzec_slash_info_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xzec_slash_info_contract);

    explicit
    xzec_slash_info_contract(common::xnetwork_id_t const & network_id);

    xcontract_base*
    clone() override {
        return new xzec_slash_info_contract(network_id());
    }

    void
    setup();

    /**
     * @brief summarize the slash info from table slash contract
     *
     * @param slash_info  the table slash info
     */
    void
    summarize_slash_info(std::string const& slash_info);


    /**
     * @brief do slash according the summarized slash info
     *
     * @param timestamp  the logic time to do the slash
     */
    void
    do_unqualified_node_slash(common::xlogic_time_t const timestamp);

    BEGIN_CONTRACT_WITH_PARAM(xzec_slash_info_contract)
        CONTRACT_FUNCTION_PARAM(xzec_slash_info_contract, summarize_slash_info);
        CONTRACT_FUNCTION_PARAM(xzec_slash_info_contract, do_unqualified_node_slash);
    END_CONTRACT_WITH_PARAM

private:
    /**
     * @brief internal function to process summarize info
     *
     * @return true
     * @return false
     */

    bool summarize_slash_info_internal(std::string const& slash_info, std::string const& summarize_info_str, std::string const& summarize_tableblock_count_str, uint64_t const summarized_height,
                                      xunqualified_node_info_t& summarize_info,  uint32_t&  summarize_tableblock_count, std::uint64_t& cur_statistic_height);


    bool do_unqualified_node_slash_internal(std::string const& last_slash_time_str, uint32_t summarize_tableblock_count, uint32_t slash_interval_table_block_param, uint32_t slash_interval_time_block_param , common::xlogic_time_t const timestamp,
                                            xunqualified_node_info_t const & summarize_info, uint32_t slash_vote_threshold, uint32_t slash_persent_threshold, uint32_t award_vote_threshold, uint32_t award_persent_threshold, std::vector<xaction_node_info_t>& node_to_action);

    /**
     * @brief print the summarize info
     *
     * @param summarize_slash_info   the current summarized slash info to print
     */
    void print_summarize_info(data::xunqualified_node_info_t const & summarize_slash_info);

    /**
     * @brief print stored table height info
     *
     */
    void print_table_height_info();

    /**
     * @brief get current stored property info for slash, judge some pre-condition
     *
     * @param summarize_info   in&out  the slash summarize_info property
     * @param tableblock_count  in&out  the tableblock count property
     *
     */
    void pre_condition_process(xunqualified_node_info_t& summarize_info, uint32_t& tableblock_count);

    /**
     * @brief filter out the slash node according the summarized slash info
     *
     * @param summarize_info   the summarized slash info
     * @return std::vector<data::xaction_node_info_t>  the node to slash or reward
     */
    std::vector<data::xaction_node_info_t>
    filter_nodes(data::xunqualified_node_info_t const & summarize_info, uint32_t slash_vote_threshold, uint32_t slash_persent_threshold, uint32_t award_vote_threshold, uint32_t award_persent_threshold);

    /**
     * @brief filter helper to filter out the slash node
     *
     * @param node_map  the summarized node info
     * @param slash_vote_threshold the vote threshold to slash
     * @param slash_persent_threshold the persent of node to slash
     * @param award_vote_threshold the vote threshold to award
     * @param award_persent_threshold the persent of node to award
     * @return std::vector<data::xaction_node_info_t>  the node to slash or reward
     */
    std::vector<data::xaction_node_info_t>
    filter_helper(data::xunqualified_node_info_t const & node_map, uint32_t slash_vote_threshold, uint32_t slash_persent_threshold, uint32_t award_vote_threshold, uint32_t award_persent_threshold);

    /**
     * @brief get the latest tablefullblock from last read height
     *
     * @param owner the owner addr of the full tableblock
     * @param time_interval  the interval to judge the latest block to processes
     * @param last_read_height the height of full tableblock last time read
     * @
     */
    std::vector<base::xauto_ptr<xblock_t>> get_next_fulltableblock(common::xaccount_address_t const& owner, uint64_t time_interval, uint64_t last_read_height) const;


    /**
     * @brief process statistic data to get nodeinfo
     *
     * @param block_statistic_data  the statistic data of a fulltable block
     * @return xunqualified_node_info_t  the node info from statistic data
     */
    xunqualified_node_info_t process_statistic_data(top::data::xstatistics_data_t const& block_statistic_data, base::xvnodesrv_t * node_service);


    /**
     * @brief accumulate  node info of all tables
     *
     * @param  node_info  the node info to accumulate
     * @param  summarize_info  in&out  the accumulated node info
     *
     */
    void  accumulate_node_info(xunqualified_node_info_t const&  node_info, xunqualified_node_info_t& summarize_info);

    /**
     * @brief check if statisfy the slash condition
     * @param summarize_tableblock_count  current summarized table block
     * @param timestamp  current slash timestamp
     *
     * @return bool  true means statisfy the slash condition
     */
    bool slash_condition_check(std::string const& last_slash_time_str, uint32_t summarize_tableblock_count, uint32_t slash_interval_table_block_param,
                                uint32_t slash_interval_time_block_param , common::xlogic_time_t const timestamp);

    /**
     * @brief get fulltable height of table
     *
     * @param table_id
     * @return uint64_t
     */
    uint64_t read_fulltable_height_of_table(uint32_t table_id);

    /**
     * @brief process reset data
     *
     * @param db_kv_131 the reset property data
     */
    void process_reset_data(std::vector<std::pair<std::string, std::string>> const& db_kv_131);


};

NS_END3
