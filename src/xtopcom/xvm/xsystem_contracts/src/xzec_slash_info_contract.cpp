// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xslash/xzec_slash_info_contract.h"

#include "xchain_fork/xutility.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xcommon/xip.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/manager/xcontract_manager.h"


using namespace top::data;

NS_BEG3(top, xvm, xcontract)

#define SLASH_DELETE_PROPERTY  "SLASH_DELETE_PROPERTY"
#define LAST_SLASH_TIME  "LAST_SLASH_TIME"
#define SLASH_TABLE_ROUND "SLASH_TABLE_ROUND"

xzec_slash_info_contract::xzec_slash_info_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xzec_slash_info_contract::setup() {
    // initialize map key
    MAP_CREATE(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY);
    std::vector<std::pair<std::string, std::string>> db_kv_131;
    chain_data::xchain_data_processor_t::get_stake_map_property(
        common::xlegacy_account_address_t{SELF_ADDRESS()}, data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, db_kv_131);
    process_reset_data(db_kv_131);
    MAP_CREATE(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY);

    MAP_CREATE(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY);
    MAP_SET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, SLASH_DELETE_PROPERTY, "false");
    MAP_SET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, LAST_SLASH_TIME, "0");
    MAP_SET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, SLASH_TABLE_ROUND, "0");
}


void xzec_slash_info_contract::summarize_slash_info(std::string const & slash_info) {
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::contract_zec_slash_summarize_fullblock, 1);

#if defined(DEBUG)
    auto const & account = SELF_ADDRESS();
#endif
    auto const & source_addr = SOURCE_ADDRESS();

    std::string base_addr;
    uint32_t table_id = 0;
    XCONTRACT_ENSURE(data::xdatautil::extract_parts(source_addr, base_addr, table_id), "source address extract base_addr or table_id error!");
    xdbg("[xzec_slash_info_contract][summarize_slash_info] self_account %s, source_addr %s, base_addr %s", account.to_string().c_str(), source_addr.c_str(), base_addr.c_str());
    XCONTRACT_ENSURE(base_addr == top::sys_contract_sharding_statistic_info_addr || base_addr == top::sys_contract_eth_table_statistic_info_addr, "invalid source addr's call!");

    xinfo("[xzec_slash_info_contract][summarize_slash_info] enter table contract report slash info, SOURCE_ADDRESS: %s", source_addr.c_str());

    auto summarized_height = read_fulltable_height_of_table(table_id);
    // get current summarized info string
    std::string summarize_info_str;
    try {
        if (MAP_FIELD_EXIST(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, "UNQUALIFIED_NODE"))
            summarize_info_str = MAP_GET(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, "UNQUALIFIED_NODE");
    } catch (std::runtime_error const & e) {
        xwarn("[xzec_slash_info_contract][summarize_slash_info] read summarized slash info error:%s", e.what());
        throw;
    }

    std::string summarize_tableblock_count_str;
    try {
        if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, "TABLEBLOCK_NUM")) {
            summarize_tableblock_count_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, "TABLEBLOCK_NUM");
        }
    } catch (std::runtime_error & e) {
        xwarn("[xzec_slash_info_contract][summarize_slash_info] read summarized tableblock num error:%s", e.what());
        throw;
    }



    data::system_contract::xunqualified_node_info_v1_t summarize_info;
    uint32_t summarize_tableblock_count = 0;
    std::uint64_t cur_statistic_height = 0;

    if (summarize_slash_info_internal(slash_info, summarize_info_str, summarize_tableblock_count_str, summarized_height,
                                     summarize_info, summarize_tableblock_count, cur_statistic_height)) {
        // set summarize info
        base::xstream_t stream(base::xcontext_t::instance());
        summarize_info.serialize_to(stream);

        {
            MAP_SET(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, "UNQUALIFIED_NODE", std::string((char *)stream.data(), stream.size()));
        }

        stream.reset();
        stream << summarize_tableblock_count;
        {
            MAP_SET(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, "TABLEBLOCK_NUM", std::string((char *)stream.data(), stream.size()));
        }

        stream.reset();
        stream << cur_statistic_height;
        {
            MAP_SET(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, base::xstring_utl::tostring(table_id), std::string((char *)stream.data(), stream.size()));
        }

    } else {
        xinfo("[xzec_slash_info_contract][summarize_slash_info] condition not statisfy!");
    }

    xkinfo("[xzec_slash_info_contract][summarize_slash_info] effective table contract report slash info, auditor size: %zu, validator size: %zu, summarized tableblock num: %u",
           summarize_info.auditor_info.size(),
           summarize_info.validator_info.size(),
           summarize_tableblock_count);
}

bool xzec_slash_info_contract::summarize_slash_info_internal(std::string const & slash_info,
                                                             std::string const & summarize_info_str,
                                                             std::string const & summarize_tableblock_count_str,
                                                             uint64_t const summarized_height,
                                                             data::system_contract::xunqualified_node_info_v1_t & summarize_info,
                                                             uint32_t & summarize_tableblock_count,
                                                             std::uint64_t & cur_statistic_height) {
    // get report info
    data::system_contract::xunqualified_node_info_v1_t node_info;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)slash_info.data(), slash_info.size());
    node_info.serialize_from(stream);
    stream >> cur_statistic_height;

    if (cur_statistic_height <= summarized_height) {
        xwarn("[xzec_slash_info_contract][summarize_slash_info] report older slash info, summarized height: %" PRIu64 ", report height: %" PRIu64,
        summarized_height,
        cur_statistic_height);
        return false;
    }


    // get serialized summarized info from str
    if (!summarize_info_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)summarize_info_str.data(), summarize_info_str.size());
        summarize_info.serialize_from(stream);
    }

    if (!summarize_tableblock_count_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)summarize_tableblock_count_str.data(), summarize_tableblock_count_str.size());
        stream >> summarize_tableblock_count;
    }


    // accoumulate node info
    accumulate_node_info(node_info, summarize_info);
    summarize_tableblock_count += cur_statistic_height - summarized_height;

    return true;

}



void xzec_slash_info_contract::do_unqualified_node_slash(common::xlogic_time_t const timestamp) {
    auto const & account = SELF_ADDRESS();
    auto const & source_addr = SOURCE_ADDRESS();

    std::string base_addr = "";
    uint32_t table_id = 0;
    XCONTRACT_ENSURE(data::xdatautil::extract_parts(source_addr, base_addr, table_id), "source address extract base_addr or table_id error!");
    xdbg("[xzec_slash_info_contract][do_unqualified_node_slash] self_account %s, source_addr %s, base_addr %s\n",
         account.to_string().c_str(),
         source_addr.c_str(),
         base_addr.c_str());
    XCONTRACT_ENSURE(source_addr == account.to_string() && source_addr == top::sys_contract_zec_slash_info_addr, "invalid source addr's call!");

    xinfo("[xzec_slash_info_contract][do_unqualified_node_slash] do unqualified node slash info, time round: %" PRIu64 ": SOURCE_ADDRESS: %s", timestamp, SOURCE_ADDRESS().c_str());

    /**
     *
     * get stored processed slash info
     *
     */
    data::system_contract::xunqualified_node_info_v1_t present_summarize_info;
    uint32_t present_tableblock_count = 0;
    pre_condition_process(present_summarize_info, present_tableblock_count);

#ifdef DEBUG
    print_table_height_info();
    print_summarize_info(present_summarize_info);
    xdbg_info("[xzec_slash_info_contract][do_unqualified_node_slash] present tableblock num is %u", present_tableblock_count);
#endif

    data::system_contract::xunqualified_node_info_v1_t summarize_info = present_summarize_info;
    uint32_t summarize_tableblock_count = present_tableblock_count;

    // get check params
    std::string last_slash_time_str;
    try {
        if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, LAST_SLASH_TIME)) {
            last_slash_time_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, LAST_SLASH_TIME);
        }
    } catch (std::runtime_error & e) {
        xwarn("[xzec_slash_info_contract][get_next_fulltableblock] read last slash time error:%s", e.what());
        throw;
    }

    auto slash_interval_table_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_interval_table_block);
    auto slash_interval_time_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(slash_interval_time_block);

    // get filter param
    auto slash_vote_threshold = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_slash_threshold_value);
    auto slash_persent_threshold = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_slash_threshold_value);
    auto award_vote_threshold = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_awardcredit_threshold_value);
    auto award_persent_threshold = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_awardcredit_threshold_value);

    // do slash
    std::vector<data::system_contract::xaction_node_info_t> node_to_action;
    if (do_unqualified_node_slash_internal(last_slash_time_str,
                                           summarize_tableblock_count,
                                           slash_interval_table_block_param,
                                           slash_interval_time_block_param,
                                           timestamp,
                                           summarize_info,
                                           slash_vote_threshold,
                                           slash_persent_threshold,
                                           award_vote_threshold,
                                           award_persent_threshold,
                                           node_to_action)) {
        {
            MAP_SET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, SLASH_DELETE_PROPERTY, "true");
            MAP_SET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, LAST_SLASH_TIME, std::to_string(timestamp));
        }

        base::xstream_t stream{base::xcontext_t::instance()};
        VECTOR_OBJECT_SERIALIZE2(stream, node_to_action);
        std::string punish_node_str = std::string((char *)stream.data(), stream.size());
        {
            stream.reset();
            stream << punish_node_str;
            xinfo("[xzec_slash_info_contract][do_unqualified_node_slash] call register contract slash_unqualified_node, time round: %" PRIu64, timestamp);
            XMETRICS_PACKET_INFO("sysContract_zecSlash", "effective slash timer round", std::to_string(timestamp));
            CALL(common::xaccount_address_t{sys_contract_rec_registration_addr}, "slash_unqualified_node", std::string((char *)stream.data(), stream.size()));
        }

    } else {
        xdbg("[xzec_slash_info_contract][do_unqualified_node_slash] condition not satisfied! time round %" PRIu64, timestamp);
    }
}


bool xzec_slash_info_contract::do_unqualified_node_slash_internal(std::string const & last_slash_time_str,
                                                                  uint32_t summarize_tableblock_count,
                                                                  uint32_t slash_interval_table_block_param,
                                                                  uint32_t slash_interval_time_block_param,
                                                                  common::xlogic_time_t const timestamp,
                                                                  data::system_contract::xunqualified_node_info_v1_t const & summarize_info,
                                                                  uint32_t slash_vote_threshold,
                                                                  uint32_t slash_persent_threshold,
                                                                  uint32_t award_vote_threshold,
                                                                  uint32_t award_persent_threshold,
                                                                  std::vector<data::system_contract::xaction_node_info_t> & node_to_action) {
    // check slash interval time
    auto result = slash_condition_check(last_slash_time_str, summarize_tableblock_count, slash_interval_table_block_param, slash_interval_time_block_param, timestamp);
    if (!result) {
        xinfo("[xzec_slash_info_contract][do_unqualified_node_slash_internal] slash condition not statisfied, time round: %" PRIu64, timestamp);
        return false;
    }

    node_to_action = filter_nodes(summarize_info, slash_vote_threshold, slash_persent_threshold, award_vote_threshold, award_persent_threshold);
    if (node_to_action.empty()) {
        xinfo("[xzec_slash_info_contract][do_unqualified_node_slash_internal] filter nodes empty, time round: %" PRIu64, timestamp);
        return false;
    }

    return true;
}

void xzec_slash_info_contract::pre_condition_process(data::system_contract::xunqualified_node_info_v1_t & summarize_info, uint32_t & tableblock_count) {
    std::string value_str;

    std::string delete_property = "false";
    try {
        delete_property = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, SLASH_DELETE_PROPERTY);
    } catch (std::runtime_error const & e) {
        xwarn("[xzec_slash_info_contract][[do_unqualified_node_slash] read extend key error:%s", e.what());
        throw;
    }
    xdbg("[xzec_slash_info_contract][do_unqualified_node_slash] extend key value: %s\n", delete_property.c_str());

    if (delete_property == "true") {
        {
            MAP_REMOVE(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, "UNQUALIFIED_NODE");
        }
        {
            MAP_REMOVE(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, "TABLEBLOCK_NUM");
        }

        {
            MAP_SET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, SLASH_DELETE_PROPERTY, "false");
        }

    } else {
        try {
            if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, "TABLEBLOCK_NUM")) {
                value_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, "TABLEBLOCK_NUM");
            }
        } catch (std::runtime_error const & e) {
            xwarn("[xzec_slash_info_contract][[do_unqualified_node_slash] read summarized tableblock num error:%s", e.what());
            throw;
        }
        // normally only first time will be empty(property not create yet), means tableblock count is zero, so no need else branch
        if (!value_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
            stream >> tableblock_count;
            xdbg("[xzec_slash_info_contract][do_unqualified_node_slash]  current summarized tableblock num is: %u", tableblock_count);
        }

        value_str.clear();
        try {
            if (MAP_FIELD_EXIST(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, "UNQUALIFIED_NODE")) {
                value_str = MAP_GET(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, "UNQUALIFIED_NODE");
            }
        } catch (std::runtime_error const & e) {
            xwarn("[xzec_slash_info_contract][do_unqualified_node_slash] read summarized slash info error:%s", e.what());
            throw;
        }
        // normally only first time will be empty(property not create yet), means height is zero, so no need else branch
        if (!value_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
            summarize_info.serialize_from(stream);
        }

    }

}

std::vector<data::system_contract::xaction_node_info_t> xzec_slash_info_contract::filter_nodes(data::system_contract::xunqualified_node_info_v1_t const & summarize_info,
                                                                                               uint32_t slash_vote_threshold,
                                                                                               uint32_t slash_persent_threshold,
                                                                                               uint32_t award_vote_threshold,
                                                                                               uint32_t award_persent_threshold) {
    std::vector<data::system_contract::xaction_node_info_t> nodes_to_slash{};

    if (summarize_info.auditor_info.empty() && summarize_info.validator_info.empty()) {
        xwarn("[xzec_slash_info_contract][filter_slashed_nodes] the summarized node info is empty!");
        return nodes_to_slash;
    } else {
        // summarized info
        nodes_to_slash = filter_helper(summarize_info, slash_vote_threshold, slash_persent_threshold, award_vote_threshold, award_persent_threshold);

    }

    xkinfo("[xzec_slash_info_contract][filter_slashed_nodes] the filter node to slash: size: %zu", nodes_to_slash.size());
    return nodes_to_slash;
}

std::vector<data::system_contract::xaction_node_info_t> xzec_slash_info_contract::filter_helper(data::system_contract::xunqualified_node_info_v1_t const & node_map,
                                                                                                uint32_t slash_vote_threshold,
                                                                                                uint32_t slash_persent_threshold,
                                                                                                uint32_t award_vote_threshold,
                                                                                                uint32_t award_persent_threshold) {
    std::vector<data::system_contract::xaction_node_info_t> res{};

    // do filter
    std::vector<data::system_contract::xunqualified_filter_info_t> node_to_action{};
    for (auto const & node : node_map.auditor_info) {
        data::system_contract::xunqualified_filter_info_t info;
        info.node_id = node.first;
        info.node_type = common::xnode_type_t::consensus_auditor;
        info.vote_percent = node.second.block_count * 100 / node.second.subset_count;
        node_to_action.emplace_back(info);
    }

    std::sort(node_to_action.begin(),
              node_to_action.end(),
              [](data::system_contract::xunqualified_filter_info_t const & lhs, data::system_contract::xunqualified_filter_info_t const & rhs) {
        return lhs.vote_percent < rhs.vote_percent;
    });
    // uint32_t slash_size = node_to_slash.size() * slash_persent_threshold / 100 ?  node_to_slash.size() * slash_persent_threshold / 100 : 1;
    uint32_t slash_size = node_to_action.size() * slash_persent_threshold / 100;
    for (size_t i = 0; i < slash_size; ++i) {
        if (node_to_action[i].vote_percent < slash_vote_threshold || 0 == node_to_action[i].vote_percent) {
            res.push_back(data::system_contract::xaction_node_info_t{node_to_action[i].node_id, node_to_action[i].node_type});
        }
    }

    uint32_t award_size = node_to_action.size() * award_persent_threshold / 100;
    for (int i = (int)node_to_action.size() - 1; i >= (int)(node_to_action.size() - award_size); --i) {
        if (node_to_action[i].vote_percent > award_vote_threshold) {
            res.push_back(data::system_contract::xaction_node_info_t{node_to_action[i].node_id, node_to_action[i].node_type, false});
        }
    }

    node_to_action.clear();
    for (auto const & node : node_map.validator_info) {
        data::system_contract::xunqualified_filter_info_t info;
        info.node_id = node.first;
        info.node_type = common::xnode_type_t::consensus_validator;
        info.vote_percent = node.second.block_count * 100 / node.second.subset_count;
        node_to_action.emplace_back(info);
    }

    std::sort(node_to_action.begin(),
              node_to_action.end(),
              [](data::system_contract::xunqualified_filter_info_t const & lhs, data::system_contract::xunqualified_filter_info_t const & rhs) {
        return lhs.vote_percent < rhs.vote_percent;
    });

    // uint32_t slash_size = node_to_slash.size() * slash_persent_threshold / 100 ?  node_to_slash.size() * slash_persent_threshold / 100 : 1;
    slash_size = node_to_action.size() * slash_persent_threshold / 100;
    for (size_t i = 0; i < slash_size; ++i) {
        if (node_to_action[i].vote_percent < slash_vote_threshold || 0 == node_to_action[i].vote_percent) {
            res.push_back(data::system_contract::xaction_node_info_t{node_to_action[i].node_id, node_to_action[i].node_type});
        }
    }

    award_size = node_to_action.size() * award_persent_threshold / 100;
    for (int i = (int)node_to_action.size() - 1; i >= (int)(node_to_action.size() - award_size); --i) {
        if (node_to_action[i].vote_percent > award_vote_threshold) {
            res.push_back(data::system_contract::xaction_node_info_t{node_to_action[i].node_id, node_to_action[i].node_type, false});
        }
    }

    return res;
}

void xzec_slash_info_contract::print_summarize_info(data::system_contract::xunqualified_node_info_v1_t const & summarize_slash_info) {
    std::string out = "";
    for (auto const & item : summarize_slash_info.auditor_info) {
        out += item.first.to_string();
        out += "|" + std::to_string(item.second.block_count);
        out += "|" + std::to_string(item.second.subset_count) + "|";
    }

    for (auto const & item : summarize_slash_info.validator_info) {
        out += item.first.to_string();
        out += "|" + std::to_string(item.second.block_count);
        out += "|" + std::to_string(item.second.subset_count) + "|";
    }

    xdbg("[xzec_slash_info_contract][print_summarize_info] summarize info: %s", out.c_str());
}

void xzec_slash_info_contract::print_table_height_info() {
    std::string out = "|";

    for (auto i = 0; i < enum_vledger_const::enum_vbucket_has_tables_count; ++i) {
        std::string height_key = std::to_string(i);
        std::string value_str;
        try {
            if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, height_key)) {
                value_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, height_key);
            }
        } catch (std::runtime_error & e) {
            xwarn("[xzec_slash_info_contract][get_next_fulltableblock] read full tableblock height error:%s", e.what());
            throw;
        }

        if (!value_str.empty()) {
            uint64_t height;
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
            stream >> height;
            if (height != 0) {
                out += std::to_string(i) + "-" + std::to_string(height) + "|";
            }
        }

    }

    xdbg("[xzec_slash_info_contract][print_table_height_info] table height info: %s", out.c_str());
}

void xzec_slash_info_contract::accumulate_node_info(data::system_contract::xunqualified_node_info_v1_t const & node_info,
                                                    data::system_contract::xunqualified_node_info_v1_t & summarize_info) {
    for (auto const & item : node_info.auditor_info) {
        summarize_info.auditor_info[item.first].block_count += item.second.block_count;
        summarize_info.auditor_info[item.first].subset_count += item.second.subset_count;
    }

    for (auto const & item : node_info.validator_info) {
        summarize_info.validator_info[item.first].block_count += item.second.block_count;
        summarize_info.validator_info[item.first].subset_count += item.second.subset_count;
    }
}

bool xzec_slash_info_contract::slash_condition_check(std::string const& last_slash_time_str, uint32_t summarize_tableblock_count, uint32_t slash_interval_table_block_param, uint32_t slash_interval_time_block_param , common::xlogic_time_t const timestamp) {

    if (summarize_tableblock_count < slash_interval_table_block_param) {
        xinfo("[xzec_slash_info_contract][do_unqualified_node_slash] summarize_tableblock_count not enought, time round: %" PRIu64 ", tableblock_count:%u",
            timestamp,
            summarize_tableblock_count);
        return false;
    }

    // check slash interval time
    XCONTRACT_ENSURE(!last_slash_time_str.empty(), "read last slash time error, it is empty");
    uint64_t last_slash_time = base::xstring_utl::toint64(last_slash_time_str);
    XCONTRACT_ENSURE(timestamp > last_slash_time, "current timestamp smaller than last slash time");
    if (timestamp - last_slash_time < slash_interval_time_block_param) {
        xinfo("[xzec_slash_info_contract][do_unqualified_node_slash] punish interval time not enought, time round: %" PRIu64 ", last slash time: %s",
            timestamp,
            last_slash_time_str.c_str());
        return false;
    } else {
        xinfo("[xzec_slash_info_contract][do_unqualified_node_slash] statisfy punish interval condition, time round: %" PRIu64 ", last slash time: %s",
            timestamp,
            last_slash_time_str.c_str());
    }

    return true;

}

 uint64_t xzec_slash_info_contract::read_fulltable_height_of_table(uint32_t table_id) {
    std::string height_key = std::to_string(table_id);
    uint64_t read_height = 0;
    std::string value_str;
    try {
        if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, height_key)) {
            value_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY, height_key);
        }
    } catch (std::runtime_error & e) {
        xwarn("[xzec_slash_info_contract][do_unqualified_node_slash] read full tableblock height error:%s", e.what());
        throw;
    }

    // normally only first time will be empty(property not create yet), means height is zero, so no need else branch
    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
        stream >> read_height;
        xdbg("[xzec_slash_info_contract][do_unqualified_node_slash]  last read full tableblock height is: %" PRIu64, read_height);
    }

    return read_height;
 }

 void  xzec_slash_info_contract::process_reset_data(std::vector<std::pair<std::string, std::string>> const& db_kv_131) {
    for (auto const & _p : db_kv_131) {
        if (_p.first == "UNQUALIFIED_NODE") {
            data::system_contract::xunqualified_node_info_v1_t node_info;
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)_p.second.data(), _p.second.size());
            base::xstream_t internal_stream{ base::xcontext_t::instance() };
            stream >> internal_stream;
            int32_t count;
            internal_stream >> count;
            for (int32_t i = 0; i < count; i++) {
                std::string id;
                data::system_contract::xnode_vote_percent_t value;
                base::xstream_t &internal_stream_temp = internal_stream;
                base::xstream_t internal_key_stream{ base::xcontext_t::instance() };
                internal_stream_temp >> internal_key_stream;
                internal_key_stream >> id;
                common::xnode_id_t node_id(id);
                value.serialize_from(internal_stream);
                node_info.auditor_info.emplace(std::make_pair(std::move(node_id), std::move(value)));                                                                                                      \
            }
            internal_stream >> count;
            for (int32_t i = 0; i < count; i++) {
                common::xnode_id_t node_id;
                data::system_contract::xnode_vote_percent_t value;
                base::xstream_t &internal_stream_temp = internal_stream;
                base::xstream_t internal_key_stream{ base::xcontext_t::instance() };
                internal_stream_temp >> internal_key_stream;
                internal_key_stream >> node_id;
                value.serialize_from(internal_stream);
                node_info.validator_info.emplace(std::make_pair(std::move(node_id), std::move(value)));
            }
            stream.reset();
            node_info.serialize_to(stream);
            MAP_SET(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, _p.first, std::string((char *)stream.data(), stream.size()));
        } else {
            MAP_SET(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, _p.first, _p.second);
        }
    }
 }

NS_END3
