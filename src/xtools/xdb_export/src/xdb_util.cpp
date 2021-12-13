#include "../xdb_util.h"

#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xaction_parse.h"
#include "xdata/xcodec/xmsgpack/xelection_association_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xdata/xproposal_data.h"
#include "xdata/xslash.h"
#include "xdata/xtable_bstate.h"
#include "xstake/xstake_algorithm.h"

#define XPROPERTY_CONTRACT_ELECTION_RESULT_0_KEY  "@42_0"
#define XPROPERTY_CONTRACT_ELECTION_RESULT_1_KEY  "@42_1"
#define XPROPERTY_CONTRACT_ELECTION_RESULT_2_KEY  "@42_2"

using namespace top::data;
using namespace top::xstake;

NS_BEG2(top, db_export)

static void system_property_json(xobject_ptr_t<base::xvbstate_t> const & state, std::string const & property, json & j);
static void user_property_json(xobject_ptr_t<base::xvbstate_t> const & state, std::string const & property, json & j);
static void parse_pledge_vote_map(std::map<std::string, std::string> const & map, json & j);
static void parse_tx_info_map(std::map<std::string, std::string> const & map, json & j);
static void parse_lock_token_map(std::map<std::string, std::string> const & map, json & j);
static void parse_reg_node_map(std::map<std::string, std::string> const & map, json & j);
static void parse_zec_workload_map(std::map<std::string, std::string> const & map, json & j);
static void parse_zec_votes_map(std::map<std::string, std::string> const & map, json & j);
static void parse_pollable_votes_map(std::map<std::string, std::string> const & map, json & j);
static void parse_zec_tasks_map(std::map<std::string, std::string> const & map, json & j);
static void parse_table_votes_map(std::map<std::string, std::string> const & map, json & j);
static void parse_voter_dividend_map(std::map<std::string, std::string> const & map, json & j);
static void parse_node_reward_map(std::map<std::string, std::string> const & map, json & j);
static void parse_table_height_map(std::map<std::string, std::string> const & map, json & j);
static void parse_refunds_map(std::map<std::string, std::string> const & map, json & j);
static void parse_unqualified_node_map(std::map<std::string, std::string> const & map, json & j);
static void parse_slash_info_map(std::map<std::string, std::string> const & map, json & j);
static void parse_tableblock_num_map(std::map<std::string, std::string> const & map, json & j);
static void parse_extended_function_map(std::map<std::string, std::string> const & map, json & j);
static void parse_accumulated_issuance_map(std::map<std::string, std::string> const & map, json & j);
static void parse_vote_report_time_map(std::map<std::string, std::string> const & map, json & j);
static void parse_table_account_index_map(std::map<std::string, std::string> const & map, json & j);
static void parse_table_receipt_map(std::map<std::string, std::string> const & map, json & j);
static void parse_onchain_param_map(std::map<std::string, std::string> const & map, json & j);
static void parse_proposal_map(std::map<std::string, std::string> const & map, json & j);
static void parse_proposal_voting_map(std::map<std::string, std::string> const & map, json & j);

static void parse_genesis_string(std::string const & str, json & j);
static void parse_accumulated_issuance_yearly_string(std::string const & str, json & j);
static void parse_reward_detail_string(std::string const & str, json & j);
static void parse_rec_standby_pool_string(std::string const & str, json & j);
static void parse_association_result_string(std::string const & str, json & j);
static void parse_election_result_string(std::string const & str, json & j);

static std::set<std::string> system_property = {
    XPROPERTY_BALANCE_AVAILABLE,      // token
    XPROPERTY_BALANCE_BURN,           // token
    XPROPERTY_BALANCE_LOCK,           // token
    XPROPERTY_BALANCE_PLEDGE_TGAS,    // token
    XPROPERTY_BALANCE_PLEDGE_VOTE,    // token
    XPROPERTY_LOCK_TGAS,              // uint64
    XPROPERTY_USED_TGAS_KEY,          // string
    XPROPERTY_LAST_TX_HOUR_KEY,       // string
    XPROPERTY_PLEDGE_VOTE_KEY,        // map
    XPROPERTY_EXPIRE_VOTE_TOKEN_KEY,  // string
    XPROPERTY_UNVOTE_NUM,             // uint64
    XPROPERTY_TX_INFO,                // map
    XPROPERTY_ACCOUNT_CREATE_TIME,    // uint64
    XPROPERTY_LOCK_TOKEN_KEY,         // map
};

static std::set<std::string> user_property = {
    XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT,
    XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME,
    XPROPERTY_CONTRACT_STANDBYS_KEY,
    XPROPERTY_CONTRACT_ELECTION_RESULT_0_KEY,
    XPROPERTY_CONTRACT_ELECTION_RESULT_1_KEY,
    XPROPERTY_CONTRACT_ELECTION_RESULT_2_KEY,
    XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY,
    XPROPERTY_CONTRACT_GROUP_ASSOC_KEY,
    XPORPERTY_CONTRACT_REG_KEY,
    XPORPERTY_CONTRACT_TIME_KEY,
    XPORPERTY_CONTRACT_WORKLOAD_KEY,
    // XPORPERTY_CONTRACT_SHARD_KEY,
    XPORPERTY_CONTRACT_TICKETS_KEY,
    XPORPERTY_CONTRACT_POLLABLE_KEY,
    XPORPERTY_CONTRACT_TASK_KEY,
    // XPORPERTY_CONTRACT_VOTES_KEY_BASE,
    XPORPERTY_CONTRACT_VOTES_KEY1,
    XPORPERTY_CONTRACT_VOTES_KEY2,
    XPORPERTY_CONTRACT_VOTES_KEY3,
    XPORPERTY_CONTRACT_VOTES_KEY4,
    // XPORPERTY_CONTRACT_MAX_TIME_KEY,
    // XPORPERTY_CONTRACT_AWARD_KEY,
    // XPORPERTY_CONTRACT_VALIDATOR_KEY,
    // XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE,
    XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1,
    XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2,
    XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3,
    XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4,
    XPORPERTY_CONTRACT_NODE_REWARD_KEY,
    XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY,
    XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY,
    // XPORPERTY_CONTRACT_START_HEIGHT_KEY,
    XPORPERTY_CONTRACT_REFUND_KEY,
    XPORPERTY_CONTRACT_GENESIS_STAGE_KEY,
    XPORPERTY_CONTRACT_TGAS_KEY,
    XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY,
    XPROPERTY_CONTRACT_SLASH_INFO_KEY,
    XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY,
    XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY,
    XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE,
    XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY,
    XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT,
    XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME,
    XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY,
    XPROPERTY_REWARD_DETAIL,
    ONCHAIN_PARAMS,
    SYSTEM_GENERATED_ID,
    PROPOSAL_MAP_ID,
    VOTE_MAP_ID,
    CURRENT_VOTED_PROPOSAL,
    XPROPERTY_TABLE_ACCOUNT_INDEX,
    XPROPERTY_TABLE_RECEIPTID,
};

static std::set<std::string> tx_info_key = {
    XPROPERTY_TX_INFO_LATEST_SENDTX_NUM,
    XPROPERTY_TX_INFO_LATEST_SENDTX_HASH,
    XPROPERTY_TX_INFO_RECVTX_NUM,
    XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM,
};

static std::unordered_map<common::xnode_type_t, std::string> node_type_map = {{common::xnode_type_t::consensus_auditor, "auditor"},
                                                                              {common::xnode_type_t::consensus_validator, "validator"},
                                                                              {common::xnode_type_t::edge, "edge"},
                                                                              {common::xnode_type_t::storage_archive, "archive"},
                                                                              {common::xnode_type_t::rec, "root_beacon"},
                                                                              {common::xnode_type_t::zec, "sub_beacon"},
                                                                              {common::xnode_type_t::storage_full_node, "full_node"}};

void property_json(xobject_ptr_t<base::xvbstate_t> const & state, json & j) {
    auto const & all_properties = state->get_all_property_names();
    for (auto const & property : all_properties) {
        if (system_property.count(property)) {
            system_property_json(state, property, j["system_property"]);
        } else if (user_property.count(property)) {
            user_property_json(state, property, j["user_property"]);
        } else {
            std::cerr << "account: " << state->get_account() << " property: " << property << " not found in system!" << std::endl;
        }
    }
}

static void system_property_json(xobject_ptr_t<base::xvbstate_t> const & state, std::string const & property, json & j) {
    if (property == XPROPERTY_BALANCE_AVAILABLE || property == XPROPERTY_BALANCE_BURN || property == XPROPERTY_BALANCE_LOCK || property == XPROPERTY_BALANCE_PLEDGE_TGAS ||
        property == XPROPERTY_BALANCE_PLEDGE_VOTE) {
        auto token_property = state->load_token_var(property);
        auto balance = token_property->get_balance();
        j[property] = balance;
    } else if (property == XPROPERTY_USED_TGAS_KEY || property == XPROPERTY_LAST_TX_HOUR_KEY || property == XPROPERTY_EXPIRE_VOTE_TOKEN_KEY) {
        auto string_property = state->load_string_var(property);
        auto string = string_property->query();
        j[property] = base::xstring_utl::touint64(string);
    } else if (property == XPROPERTY_LOCK_TGAS || property == XPROPERTY_UNVOTE_NUM || property == XPROPERTY_ACCOUNT_CREATE_TIME) {
        auto uint64_property = state->load_uint64_var(property);
        auto integer = uint64_property->get();
        j[property] = integer;
    } else if (property == XPROPERTY_PLEDGE_VOTE_KEY || property == XPROPERTY_TX_INFO || property == XPROPERTY_LOCK_TOKEN_KEY) {
        auto map_property = state->load_string_map_var(property);
        auto map = map_property->query();
        if (property == XPROPERTY_PLEDGE_VOTE_KEY) {
            parse_pledge_vote_map(map, j[property]);
        } else if (property == XPROPERTY_TX_INFO) {
            parse_tx_info_map(map, j[property]);
        } else if (property == XPROPERTY_LOCK_TOKEN_KEY) {
            parse_lock_token_map(map, j[property]);
        }
    } else {
        std::cerr << "account: " << state->get_account() << " property: " << property << " not found in system property set!" << std::endl;
    }
}

static void user_property_json(xobject_ptr_t<base::xvbstate_t> const & state, std::string const & property, json & j) {
    if (property == XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_BLOCK_HEIGHT || property == XPROPERTY_LAST_READ_REC_STANDBY_POOL_CONTRACT_LOGIC_TIME ||
        property == XPROPERTY_CONTRACT_STANDBYS_KEY || property == XPROPERTY_CONTRACT_ELECTION_RESULT_0_KEY || property == XPROPERTY_CONTRACT_ELECTION_RESULT_1_KEY ||
        property == XPROPERTY_CONTRACT_ELECTION_RESULT_2_KEY || property == XPROPERTY_CONTRACT_ELECTION_EXECUTED_KEY || property == XPROPERTY_CONTRACT_GROUP_ASSOC_KEY ||
        property == XPORPERTY_CONTRACT_TIME_KEY || property == XPORPERTY_CONTRACT_GENESIS_STAGE_KEY || property == XPORPERTY_CONTRACT_TGAS_KEY ||
        property == XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY || property == XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT ||
        property == XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME || property == XPROPERTY_REWARD_DETAIL || property == SYSTEM_GENERATED_ID ||
        property == CURRENT_VOTED_PROPOSAL) {
        auto const & string_property = state->load_string_var(property);
        auto const & string = string_property->query();
        if (string.empty()) {
            j[property] = nullptr;
            return;
        }
        if (property == XPORPERTY_CONTRACT_GENESIS_STAGE_KEY) {
            parse_genesis_string(string, j[property]);
        } else if (property == XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY) {
            parse_accumulated_issuance_yearly_string(string, j[property]);
        } else if (property == XPROPERTY_REWARD_DETAIL) {
            parse_reward_detail_string(string, j[property]);
        } else if (property == XPROPERTY_CONTRACT_STANDBYS_KEY) {
            // parse_rec_standby_pool_string(string, j[property]);
            j[property] = base::xstring_utl::to_hex(string);
        } else if (property == XPROPERTY_CONTRACT_ELECTION_RESULT_0_KEY || property == XPROPERTY_CONTRACT_ELECTION_RESULT_1_KEY ||
                   property == XPROPERTY_CONTRACT_ELECTION_RESULT_2_KEY) {
            parse_election_result_string(string, j[property]);
        } else if (property == XPROPERTY_CONTRACT_GROUP_ASSOC_KEY) {
            parse_association_result_string(string, j[property]);
        } else if (property == CURRENT_VOTED_PROPOSAL) {
            j[property] = base::xstring_utl::to_hex(string);
        } else {
            j[property] = base::xstring_utl::touint64(string);
        }
    } else if (property == XPORPERTY_CONTRACT_REG_KEY || property == XPORPERTY_CONTRACT_WORKLOAD_KEY || property == XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY ||
               property == XPORPERTY_CONTRACT_TICKETS_KEY || property == XPORPERTY_CONTRACT_POLLABLE_KEY || property == XPORPERTY_CONTRACT_TASK_KEY ||
               property == XPORPERTY_CONTRACT_VOTES_KEY1 || property == XPORPERTY_CONTRACT_VOTES_KEY2 || property == XPORPERTY_CONTRACT_VOTES_KEY3 ||
               property == XPORPERTY_CONTRACT_VOTES_KEY4 || property == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1 ||
               property == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2 || property == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3 ||
               property == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4 || property == XPORPERTY_CONTRACT_NODE_REWARD_KEY ||
               property == XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY || property == XPORPERTY_CONTRACT_REFUND_KEY || property == XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY ||
               property == XPROPERTY_CONTRACT_SLASH_INFO_KEY || property == XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY || property == XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY ||
               property == XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE || property == XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY || property == XPROPERTY_TABLE_ACCOUNT_INDEX ||
               property == XPROPERTY_TABLE_RECEIPTID || property == ONCHAIN_PARAMS || property == PROPOSAL_MAP_ID || property == VOTE_MAP_ID) {
        auto map_property = state->load_string_map_var(property);
        auto map = map_property->query();
        if (property == XPORPERTY_CONTRACT_REG_KEY) {
            parse_reg_node_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_WORKLOAD_KEY || property == XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY) {
            parse_zec_workload_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_TICKETS_KEY) {
            parse_zec_votes_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_POLLABLE_KEY) {
            parse_pollable_votes_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_TASK_KEY) {
            parse_zec_tasks_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_VOTES_KEY1 || property == XPORPERTY_CONTRACT_VOTES_KEY2 || property == XPORPERTY_CONTRACT_VOTES_KEY3 ||
                   property == XPORPERTY_CONTRACT_VOTES_KEY4) {
            parse_table_votes_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY1 || property == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY2 ||
                   property == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY3 || property == XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY4) {
            parse_voter_dividend_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_NODE_REWARD_KEY) {
            parse_node_reward_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY) {
            parse_table_height_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_REFUND_KEY) {
            parse_refunds_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY) {
            parse_unqualified_node_map(map, j[property]);
        } else if (property == XPROPERTY_CONTRACT_SLASH_INFO_KEY) {
            parse_slash_info_map(map, j[property]);
        } else if (property == XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY) {
            parse_tableblock_num_map(map, j[property]);
        } else if (property == XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY) {
            parse_extended_function_map(map, j[property]);
        } else if (property == XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE) {
            parse_accumulated_issuance_map(map, j[property]);
        } else if (property == XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY) {
            parse_vote_report_time_map(map, j[property]);
        } else if (property == XPROPERTY_TABLE_ACCOUNT_INDEX) {
            parse_table_account_index_map(map, j[property]);
        } else if (property == XPROPERTY_TABLE_RECEIPTID) {
            parse_table_receipt_map(map, j[property]);
        } else if (property == ONCHAIN_PARAMS) {
            parse_onchain_param_map(map, j[property]);
        } else if (property == PROPOSAL_MAP_ID) {
            parse_proposal_map(map, j[property]);
        } else if (property == VOTE_MAP_ID) {
            parse_proposal_voting_map(map, j[property]);
        }
    }
}

static void parse_pledge_vote_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & pair : map) {
        uint64_t vote_num{0};
        uint16_t duration{0};
        uint64_t lock_time{0};
        base::xstream_t key{base::xcontext_t::instance(), (uint8_t *)pair.first.data(), static_cast<uint32_t>(pair.first.size())};
        key >> duration;
        key >> lock_time;
        base::xstream_t val{base::xcontext_t::instance(), (uint8_t *)pair.second.data(), static_cast<uint32_t>(pair.second.size())};
        val >> vote_num;
        json jv;
        jv["vote_num"] = vote_num;
        jv["duration"] = duration;
        jv["lock_time"] = lock_time;
        j += jv;
    }
}

static void parse_tx_info_map(std::map<std::string, std::string> const & map, json & j) {
    {
        auto iter = map.find(XPROPERTY_TX_INFO_LATEST_SENDTX_NUM);
        if (iter != map.end()) {
            j[XPROPERTY_TX_INFO_LATEST_SENDTX_NUM] = base::xstring_utl::touint64(iter->second);
        } else {
            j[XPROPERTY_TX_INFO_LATEST_SENDTX_NUM] = nullptr;
        }
    }
    {
        auto iter = map.find(XPROPERTY_TX_INFO_LATEST_SENDTX_HASH);
        if (iter != map.end()) {
            j[XPROPERTY_TX_INFO_LATEST_SENDTX_HASH] = base::xstring_utl::to_hex(iter->second);
        } else {
            j[XPROPERTY_TX_INFO_LATEST_SENDTX_HASH] = nullptr;
        }
    }
    {
        auto iter = map.find(XPROPERTY_TX_INFO_RECVTX_NUM);
        if (iter != map.end()) {
            j[XPROPERTY_TX_INFO_RECVTX_NUM] = base::xstring_utl::touint64(iter->second);
        } else {
            j[XPROPERTY_TX_INFO_RECVTX_NUM] = nullptr;
        }
    }
    {
        auto iter = map.find(XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM);
        if (iter != map.end()) {
            j[XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM] = base::xstring_utl::touint64(iter->second);
        } else {
            j[XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM] = nullptr;
        }
    }
}

static void parse_lock_token_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & tx : map) {
        uint64_t clock_timer{0};
        std::string raw_input{};
        std::string key{base::xstring_utl::to_hex(tx.first)};
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)tx.second.c_str(), (uint32_t)tx.second.size());
        stream >> clock_timer;
        stream >> raw_input;
        data::xaction_lock_account_token lock_action;
        lock_action.parse_param(raw_input);
        j[key]["version"] = lock_action.m_version;
        j[key]["amount"] = lock_action.m_amount;
        j[key]["unlock_type"] = lock_action.m_unlock_type;
        for (auto const & unlock_value : lock_action.m_unlock_values) {
            j[key]["unlock_values"] += unlock_value;
        }
        j[key]["param"] = base::xstring_utl::to_hex(lock_action.m_params);
    }
}

static void parse_reg_node_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        xstake::xreg_node_info reg_node_info;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)m.second.data(), m.second.size());
        reg_node_info.serialize_from(stream);
        json j_node;
        j_node["account_addr"] = reg_node_info.m_account.value();
        j_node["node_deposit"] = static_cast<unsigned long long>(reg_node_info.m_account_mortgage);
        if (reg_node_info.m_genesis_node) {
            j_node["registered_node_type"] = std::string{"advance,validator,edge"};
        } else {
            j_node["registered_node_type"] = common::to_string(reg_node_info.m_registered_role);
        }
        j_node["vote_amount"] = static_cast<unsigned long long>(reg_node_info.m_vote_amount);
        {
            auto credit = static_cast<double>(reg_node_info.m_auditor_credit_numerator) / reg_node_info.m_auditor_credit_denominator;
            std::stringstream ss;
            ss << std::fixed << std::setprecision(6) << credit;
            j_node["auditor_credit"] = ss.str();
        }
        {
            auto credit = static_cast<double>(reg_node_info.m_validator_credit_numerator) / reg_node_info.m_validator_credit_denominator;
            std::stringstream ss;
            ss << std::fixed << std::setprecision(6) << credit;
            j_node["validator_credit"] = ss.str();
        }
        j_node["dividend_ratio"] = reg_node_info.m_support_ratio_numerator * 100 / reg_node_info.m_support_ratio_denominator;
        // j_node["m_stake"] = static_cast<unsigned long long>(reg_node_info.m_stake);
        j_node["auditor_stake"] = static_cast<unsigned long long>(reg_node_info.get_auditor_stake());
        j_node["validator_stake"] = static_cast<unsigned long long>(reg_node_info.get_validator_stake());
        j_node["rec_stake"] = static_cast<unsigned long long>(reg_node_info.rec_stake());
        j_node["zec_stake"] = static_cast<unsigned long long>(reg_node_info.zec_stake());
        std::string network_ids;
        for (auto const & net_id : reg_node_info.m_network_ids) {
            network_ids += net_id.to_string() + ' ';
        }
        j_node["network_id"] = network_ids;
        j_node["nodename"] = reg_node_info.nickname;
        j_node["node_sign_key"] = reg_node_info.consensus_public_key.to_string();
        j[m.first] = j_node;
    }
}

static void parse_zec_workload_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        auto const & group_id = m.first;
        auto const & detail = m.second;
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
        xstake::cluster_workload_t workload;
        workload.serialize_from(stream);
        json jn;
        jn["cluster_total_workload"] = workload.cluster_total_workload;
        common::xcluster_address_t cluster;
        base::xstream_t key_stream(base::xcontext_t::instance(), (uint8_t *)group_id.data(), group_id.size());
        key_stream >> cluster;
        for (auto node : workload.m_leader_count) {
            jn[node.first] = node.second;
        }
        j[cluster.group_id().to_string()] = jn;
    }
}

static void parse_zec_votes_map(std::map<std::string, std::string> const & map, json & j) {
    std::map<std::string, std::string> votes_table;
    for (auto const & m : map) {
        auto detail = m.second;
        if (!detail.empty()) {
            votes_table.clear();
            base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
            stream >> votes_table;
        }

        json jv;
        json jvn;
        for (auto v : votes_table) {
            jvn[v.first] = base::xstring_utl::touint64(v.second);
        }
        jv["vote_infos"] = jvn;
        j[m.first] = jv;
    }
}

static void parse_pollable_votes_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        j[m.first] = base::xstring_utl::touint64(m.second);
    }
}

static void parse_zec_tasks_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        auto const & detail = m.second;
        if (detail.empty()) {
            continue;
        }
        xstake::xreward_dispatch_task task;
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
        task.serialize_from(stream);
        json jv;
        json jvn;
        jv["task_id"] = m.first;
        jv["onchain_timer_round"] = task.onchain_timer_round;
        jv["contract"] = task.contract;
        jv["action"] = task.action;
        if (task.action == xstake::XREWARD_CLAIMING_ADD_NODE_REWARD || task.action == xstake::XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD) {
            base::xstream_t stream_params{base::xcontext_t::instance(), (uint8_t *)task.params.data(), static_cast<uint32_t>(task.params.size())};
            uint64_t onchain_timer_round;
            std::map<std::string, top::xstake::uint128_t> rewards;
            stream_params >> onchain_timer_round;
            stream_params >> rewards;
            for (auto v : rewards) {
                jvn[v.first] = base::xstring_utl::tostring(static_cast<uint64_t>(v.second / xstake::REWARD_PRECISION)) + std::string(".") +
                               base::xstring_utl::tostring(static_cast<uint32_t>(v.second % xstake::REWARD_PRECISION));
            }
            jv["rewards"] = jvn;
        } else if (task.action == xstake::XTRANSFER_ACTION) {
            std::map<std::string, uint64_t> issuances;
            base::xstream_t seo_stream(base::xcontext_t::instance(), (uint8_t *)task.params.c_str(), (uint32_t)task.params.size());
            seo_stream >> issuances;
            for (auto const & issue : issuances) {
                jvn[issue.first] = base::xstring_utl::tostring(issue.second);
            }
        }
        j += jv;
    }
}

static void parse_table_votes_map(std::map<std::string, std::string> const & map, json & j) {
    std::map<std::string, uint64_t> vote_info;
    for (auto const & m : map) {
        auto detail = m.second;
        if (!detail.empty()) {
            vote_info.clear();
            base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
            stream >> vote_info;
        }
        json jv;
        json jvn;
        for (auto v : vote_info) {
            jvn[v.first] = v.second;
        }
        jv["vote_infos"] = jvn;
        j[m.first] = jv;
    }
}

static void parse_voter_dividend_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        xstake::xreward_record record;
        auto detail = m.second;
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
        record.serialize_from(stream);

        json jv;
        jv["accumulated"] = static_cast<uint64_t>(record.accumulated / xstake::REWARD_PRECISION);
        jv["accumulated_decimals"] = static_cast<uint32_t>(record.accumulated % xstake::REWARD_PRECISION);
        jv["unclaimed"] = static_cast<uint64_t>(record.unclaimed / xstake::REWARD_PRECISION);
        jv["unclaimed_decimals"] = static_cast<uint32_t>(record.unclaimed % xstake::REWARD_PRECISION);
        jv["last_claim_time"] = record.last_claim_time;
        jv["issue_time"] = record.issue_time;
        json jvm;
        int no = 0;
        for (auto n : record.node_rewards) {
            json jvn;
            jvn["account_addr"] = n.account;
            jvn["accumulated"] = static_cast<uint64_t>(n.accumulated / xstake::REWARD_PRECISION);
            jvn["accumulated_decimals"] = static_cast<uint32_t>(n.accumulated % xstake::REWARD_PRECISION);
            jvn["unclaimed"] = static_cast<uint64_t>(n.unclaimed / xstake::REWARD_PRECISION);
            jvn["unclaimed_decimals"] = static_cast<uint32_t>(n.unclaimed % xstake::REWARD_PRECISION);
            jvn["last_claim_time"] = n.last_claim_time;
            jvn["issue_time"] = n.issue_time;
            jvm[no++] = jvn;
        }
        jv["node_dividend"] = jvm;
        j[m.first] = jv;
    }
}

static void parse_node_reward_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        auto detail = m.second;
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
        xstake::xreward_node_record record;
        record.serialize_from(stream);
        json jv;
        jv["accumulated"] = static_cast<uint64_t>(record.m_accumulated / xstake::REWARD_PRECISION);
        jv["accumulated_decimals"] = static_cast<uint32_t>(record.m_accumulated % xstake::REWARD_PRECISION);
        jv["unclaimed"] = static_cast<uint64_t>(record.m_unclaimed / xstake::REWARD_PRECISION);
        jv["unclaimed_decimals"] = static_cast<uint32_t>(record.m_unclaimed % xstake::REWARD_PRECISION);
        jv["last_claim_time"] = record.m_last_claim_time;
        jv["issue_time"] = record.m_issue_time;
        j[m.first] = jv;
    }
}

static void parse_table_height_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        j[m.first] = base::xstring_utl::touint64(m.second);
    }
}

static void parse_refunds_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        auto detail = m.second;
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
        xstake::xrefund_info refund;
        refund.serialize_from(stream);
        json jv;
        jv["refund_amount"] = refund.refund_amount;
        jv["create_time"] = refund.create_time;
        j[m.first] = jv;
    }
}

static void parse_genesis_string(std::string const & str, json & j) {
    xstake::xactivation_record record;
    if (!str.empty()) {
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)str.data(), static_cast<uint32_t>(str.size())};
        record.serialize_from(stream);
    }
    j["activated"] = record.activated;
    j["activation_time"] = record.activation_time;
}

static void parse_unqualified_node_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        auto detail = m.second;
        data::xunqualified_node_info_t summarize_info;
        if (!detail.empty()) {
            base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), (uint32_t)detail.size()};
            summarize_info.serialize_from(stream);
        }

        json jvn;
        json jvn_auditor;
        for (auto const & v : summarize_info.auditor_info) {
            json auditor_info;
            auditor_info["vote_num"] = v.second.block_count;
            auditor_info["subset_num"] = v.second.subset_count;
            jvn_auditor[v.first.value()] = auditor_info;
        }
        json jvn_validator;
        for (auto const & v : summarize_info.validator_info) {
            json validator_info;
            validator_info["vote_num"] = v.second.block_count;
            validator_info["subset_num"] = v.second.subset_count;
            jvn_validator[v.first.value()] = validator_info;
        }
        jvn["auditor"] = jvn_auditor;
        jvn["validator"] = jvn_validator;
        j["unqualified_node"] = jvn;
    }
}

static void parse_slash_info_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        auto detail = m.second;
        xstake::xslash_info s_info;
        if (!detail.empty()) {
            base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), (uint32_t)detail.size()};
            s_info.serialize_from(stream);
        }
        json jvn;
        jvn["punish_time"] = s_info.m_punish_time;
        jvn["staking_lock_time"] = s_info.m_staking_lock_time;
        jvn["punish_staking"] = s_info.m_punish_staking;

        j[m.first] = jvn;
    }
}

static void parse_tableblock_num_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & item : map) {
        if (item.first == "TABLEBLOCK_NUM") {
            uint32_t summarize_tableblock_count;
            base::xstream_t stream{base::xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(item.second.data())), static_cast<uint32_t>(item.second.size())};
            try {
                stream >> summarize_tableblock_count;
            } catch (...) {
                xdbg("[get_tableblock_num] deserialize height error");
                return;
            }
            j[item.first] = summarize_tableblock_count;
        } else {
            uint64_t height;
            base::xstream_t stream{base::xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(item.second.data())), static_cast<uint32_t>(item.second.size())};
            try {
                stream >> height;
            } catch (...) {
                xdbg("[get_tableblock_num] deserialize height error");
                return;
            }
            j[item.first] = height;
        }
    }
}

static void parse_extended_function_map(std::map<std::string, std::string> const & map, json & j) {
    auto it = map.find("FULLTABLE_NUM");
    if (it != std::end(map)) {
        auto & value = it->second;
        if (!value.empty()) {
            j["fulltableblock_count"] = base::xstring_utl::touint32(value);
        } else {
            j["fulltableblock_count"] = 0;
        }
    }

    it = map.find("FULLTABLE_HEIGHT");
    if (it != std::end(map)) {
        auto & value = it->second;
        if (!value.empty()) {
            j["fulltableblock_height"] = base::xstring_utl::touint32(value);
        } else {
            j["fulltableblock_height"] = 0;
        }
    }
}

static void parse_accumulated_issuance_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        j[m.first] = m.second;
    }
}

static void parse_accumulated_issuance_yearly_string(std::string const & str, json & j) {
    xstake::xaccumulated_reward_record record;
    if (!str.empty()) {
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)str.data(), static_cast<uint32_t>(str.size())};
        record.serialize_from(stream);
    }
    j["last_issuance_time"] = record.last_issuance_time;
    j["issued_until_last_year_end"] = static_cast<uint64_t>(record.issued_until_last_year_end / xstake::REWARD_PRECISION);
    j["issued_until_last_year_end_decimals"] = static_cast<uint32_t>(record.issued_until_last_year_end % xstake::REWARD_PRECISION);
}

static void parse_vote_report_time_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        j[m.first] = base::xstring_utl::touint64(m.second);
    }
}

static void parse_reward_detail_string(std::string const & str, json & j) {
    if (str.empty()) {
        return;
    }
    xstake::xissue_detail issue_detail;
    issue_detail.from_string(str);
    json jv;
    jv["onchain_timer_round"] = issue_detail.onchain_timer_round;
    jv["zec_vote_contract_height"] = issue_detail.m_zec_vote_contract_height;
    jv["zec_workload_contract_height"] = issue_detail.m_zec_workload_contract_height;
    jv["zec_reward_contract_height"] = issue_detail.m_zec_reward_contract_height;
    jv["edge_reward_ratio"] = issue_detail.m_edge_reward_ratio;
    jv["archive_reward_ratio"] = issue_detail.m_archive_reward_ratio;
    jv["validator_reward_ratio"] = issue_detail.m_validator_reward_ratio;
    jv["auditor_reward_ratio"] = issue_detail.m_auditor_reward_ratio;
    jv["vote_reward_ratio"] = issue_detail.m_vote_reward_ratio;
    jv["governance_reward_ratio"] = issue_detail.m_governance_reward_ratio;
    jv["validator_group_count"] = issue_detail.m_validator_group_count;
    jv["auditor_group_count"] = issue_detail.m_auditor_group_count;
    json jr;
    for (auto const & node_reward : issue_detail.m_node_rewards) {
        std::stringstream ss;
        ss << "edge_reward: " << static_cast<uint64_t>(node_reward.second.m_edge_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_edge_reward % xstake::REWARD_PRECISION)
           << ", archive_reward: " << static_cast<uint64_t>(node_reward.second.m_archive_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_archive_reward % xstake::REWARD_PRECISION)
           << ", validator_reward: " << static_cast<uint64_t>(node_reward.second.m_validator_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_validator_reward % xstake::REWARD_PRECISION)
           << ", auditor_reward: " << static_cast<uint64_t>(node_reward.second.m_auditor_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_auditor_reward % xstake::REWARD_PRECISION)
           << ", voter_reward: " << static_cast<uint64_t>(node_reward.second.m_vote_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_vote_reward % xstake::REWARD_PRECISION)
           << ", self_reward: " << static_cast<uint64_t>(node_reward.second.m_self_reward / xstake::REWARD_PRECISION) << "." << std::setw(6) << std::setfill('0')
           << static_cast<uint32_t>(node_reward.second.m_self_reward % xstake::REWARD_PRECISION);
        jr[node_reward.first] = ss.str();
    }
    jv["node_rewards"] = jr;
    j = jv;
}

static void parse_table_account_index_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & pair : map) {
        json j_index;
        base::xaccount_index_t account_index;
        account_index.serialize_from(pair.second);
        j_index["latest_unit_height"] = account_index.get_latest_unit_height();
        j_index["latest_unit_viewid"] = account_index.get_latest_unit_viewid();
        j_index["latest_tx_nonce"] = account_index.get_latest_tx_nonce();
        j_index["latest_unit_class"] = account_index.get_latest_unit_class();
        j_index["latest_unit_type"] = account_index.get_latest_unit_type();
        j_index["latest_unit_consensus_type"] = account_index.get_latest_unit_consensus_type();
        j_index["index_flag"] = account_index.get_account_index_flag();
        j[pair.first] = j_index;
    }
}

static void parse_table_receipt_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & pair : map) {
        json j_id;
        base::xreceiptid_pair_t receiptid_pair;
        receiptid_pair.serialize_from(pair.second);
        j_id["sendid_max"] = receiptid_pair.get_sendid_max();
        j_id["recvid_max"] = receiptid_pair.get_recvid_max();
        j_id["confirmid_max"] = receiptid_pair.get_confirmid_max();
        j_id["unconfirm_num"] = receiptid_pair.get_unconfirm_num();
        j[pair.first] = j_id;
    }
}

static void parse_onchain_param_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto const & m : map) {
        j[m.first] = m.second;
    }
}

static void parse_proposal_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto m : map) {
        auto detail = m.second;
        tcc::proposal_info pi;
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
        pi.deserialize(stream);

        json jv;
        jv["proposal_id"] = pi.proposal_id;
        jv["proposal_type"] = pi.type;
        jv["target"] = pi.parameter;
        jv["value"] = pi.new_value;
        // jv["modification_description"] = pi.modification_description;
        jv["proposer_account_addr"] = pi.proposal_client_address;
        jv["proposal_deposit"] = (pi.deposit);
        jv["effective_timer_height"] = (pi.effective_timer_height);
        jv["priority"] = pi.priority;
        // jv["cosigning_status"] = pi.cosigning_status;
        jv["voting_status"] = pi.voting_status;
        jv["expire_time"] = (pi.end_time);

        j[m.first] = jv;
    }
}

static void parse_proposal_voting_map(std::map<std::string, std::string> const & map, json & j) {
    for (auto m : map) {
        auto detail = m.second;
        base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
        std::map<std::string, bool> nodes;
        stream >> nodes;
        json jn;
        for (auto node : nodes) {
            jn[node.first] = node.second;
        }
        j[m.first] = jn;
    }
}

static void parse_association_result_string(std::string const & str, json & j) {
    auto const & association_result_store = codec::msgpack_decode<data::election::xelection_association_result_store_t>({std::begin(str), std::end(str)});
    for (auto const & election_association_result : association_result_store) {
        for (auto const & association_result : election_association_result.second) {
            j[association_result.second.to_string()] += association_result.first.value();
        }
    }
}

static void parse_rec_standby_pool_string(std::string const & str, json & j) {
    printf("dtandby_data: %s\n", base::xstring_utl::to_hex(str).c_str());
    auto const & standby_result_store = codec::msgpack_decode<data::election::xstandby_result_store_t>({std::begin(str), std::end(str)});
    for (auto const & standby_network_result_info : standby_result_store) {
        auto const network_id = top::get<common::xnetwork_id_t const>(standby_network_result_info);
        auto const & standby_network_result = top::get<data::election::xstandby_network_storage_result_t>(standby_network_result_info).all_network_result();
        for (auto const & standby_result_info : standby_network_result) {
            auto const node_type = top::get<common::xnode_type_t const>(standby_result_info);
            std::string node_type_str = node_type_map.at(node_type);
            auto const standby_result = top::get<data::election::xstandby_result_t>(standby_result_info);
            for (auto const & node_info : standby_result) {
                auto const & node_id = top::get<common::xnode_id_t const>(node_info);
                json jn;
                auto const & standby_node_info = top::get<data::election::xstandby_node_info_t>(node_info);
                jn["consensus_public_key"] = standby_node_info.consensus_public_key.to_string();
                jn["node_id"] = node_id.value();
                jn["stake"] = standby_node_info.stake(node_type);
                jn["is_genesis_node"] = std::string{standby_node_info.is_genesis_node ? "true" : "false"};
                jn["program_version"] = standby_node_info.program_version;
                j[node_type_str] += jn;
            }
        }
        j["activated_state"] = std::string{standby_result_store.result_of(network_id).activated_state() ? "activated" : "not activated"};
    }
}

static void parse_election_result_string(std::string const & str, json & j) {
    auto election_result_store = codec::msgpack_decode<data::election::xelection_result_store_t>({std::begin(str), std::end(str)});
    for (auto const & election_network_result_info : election_result_store) {
        auto const network_id = top::get<common::xnetwork_id_t const>(election_network_result_info);
        auto const & election_network_result = top::get<data::election::xelection_network_result_t>(election_network_result_info);
        json jn;
        for (auto const & election_result_info : election_network_result) {
            auto const node_type = top::get<common::xnode_type_t const>(election_result_info);
            std::string node_type_str = to_string(node_type);
            if (node_type_str.back() == '.') {
                node_type_str = node_type_str.substr(0, node_type_str.size() - 1);
            }
            auto const & election_result = top::get<data::election::xelection_result_t>(election_result_info);
            for (auto const & election_cluster_result_info : election_result) {
                // auto const & cluster_id = top::get<common::xcluster_id_t const>(election_cluster_result_info);
                auto const & election_cluster_result = top::get<data::election::xelection_cluster_result_t>(election_cluster_result_info);
                for (auto const & group_result_info : election_cluster_result) {
                    auto const & group_id = top::get<common::xgroup_id_t const>(group_result_info);
                    auto const & election_group_result = top::get<data::election::xelection_group_result_t>(group_result_info);
                    for (auto const & node_info : election_group_result) {
                        auto const & node_id = top::get<data::election::xelection_info_bundle_t>(node_info).node_id();
                        if (node_id.empty()) {
                            continue;
                        }
                        auto const & election_info = top::get<data::election::xelection_info_bundle_t>(node_info).election_info();
                        json jv;
                        jv["group_id"] = group_id.value();
                        jv["stake"] = election_info.stake;
                        jv["round"] = election_group_result.group_version().value();
                        jn[node_id.to_string()] += jv;
                    }
                }
            }
            j[common::to_string(network_id)][node_type_str] = jn;
        }
    }
}

NS_END2