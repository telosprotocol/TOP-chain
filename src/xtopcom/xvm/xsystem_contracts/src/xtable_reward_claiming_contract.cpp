// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xreward/xtable_reward_claiming_contract.h"

#include "xchain_upgrade/xchain_data_processor.h"
#include "xdata/xdatautil.h"
#include "xdata/xnative_contract_address.h"
#include "xmetrics/xmetrics.h"

NS_BEG4(top, xvm, system_contracts, reward)

xtop_table_reward_claiming_contract::xtop_table_reward_claiming_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xtop_table_reward_claiming_contract::setup() {
    const int old_tables_count = 256;
    uint32_t table_id = 0;
    if (!EXTRACT_TABLE_ID(SELF_ADDRESS(), table_id)) {
        xwarn("[xtop_table_reward_claiming_contract::setup] EXTRACT_TABLE_ID failed, node reward pid: %d, account: %s", getpid(), SELF_ADDRESS().c_str());
        return;
    }
    xdbg("[xtop_table_reward_claiming_contract::setup] table id: %d", table_id);
    
    uint64_t acc_token = 0; 
    uint32_t acc_token_decimals = 0;
    for (auto i = 1; i <= xstake::XPROPERTY_SPLITED_NUM; i++) {
        std::string property{xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE};
        property += "-" + std::to_string(i);
        MAP_CREATE(property);
        {
            for (auto j = 0; j < old_tables_count; j++) {
                auto table_addr = std::string{sys_contract_sharding_reward_claiming_addr} + "@" + base::xstring_utl::tostring(j);
                std::vector<std::pair<std::string, std::string>> db_kv_121;
                chain_data::xchain_data_processor_t::get_stake_map_property(common::xlegacy_account_address_t{table_addr}, property, db_kv_121);
                for (auto const & _p : db_kv_121) {
                    base::xvaccount_t vaccount{_p.first};
                    auto account_table_id = vaccount.get_ledger_subaddr();
                    if (static_cast<uint16_t>(account_table_id) != static_cast<uint16_t>(table_id)) {
                        continue;
                    }
                    xstake::xreward_record record;
                    auto detail = _p.second;
                    base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                    record.serialize_from(stream);
                    MAP_SET(property, _p.first, _p.second);
                    acc_token += static_cast<uint64_t>(record.unclaimed / xstake::REWARD_PRECISION);
                    acc_token_decimals += static_cast<uint32_t>(record.unclaimed % xstake::REWARD_PRECISION);
                }
            }
        }
    }
    
    MAP_CREATE(xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY);
    {
        for (auto i = 0; i < old_tables_count; i++) {
            auto table_addr = std::string{sys_contract_sharding_reward_claiming_addr} + "@" + base::xstring_utl::tostring(i);
            std::vector<std::pair<std::string, std::string>> db_kv_124;
            chain_data::xchain_data_processor_t::get_stake_map_property(common::xlegacy_account_address_t{table_addr}, XPORPERTY_CONTRACT_NODE_REWARD_KEY, db_kv_124);
            for (auto const & _p : db_kv_124) {
                base::xvaccount_t vaccount{_p.first};
                auto account_table_id = vaccount.get_ledger_subaddr();
                if (static_cast<uint16_t>(account_table_id) != static_cast<uint16_t>(table_id)) {
                    continue;
                }
                MAP_SET(XPORPERTY_CONTRACT_NODE_REWARD_KEY, _p.first, _p.second);
                xstake::xreward_node_record record;
                auto detail = _p.second;
                base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                record.serialize_from(stream);
                acc_token += static_cast<uint64_t>(record.m_unclaimed / xstake::REWARD_PRECISION);
                acc_token_decimals += static_cast<uint32_t>(record.m_unclaimed % xstake::REWARD_PRECISION);
            }
        }
    }

    acc_token += (acc_token_decimals / xstake::REWARD_PRECISION);
    if (acc_token_decimals % xstake::REWARD_PRECISION != 0) {
        acc_token += 1;
    }
    TOP_TOKEN_INCREASE(acc_token);
}

xcontract::xcontract_base * xtop_table_reward_claiming_contract::clone() {
    return new xtop_table_reward_claiming_contract{network_id()};
}

void xtop_table_reward_claiming_contract::update_vote_reward_record(common::xaccount_address_t const & account, xstake::xreward_record const & record) {
    uint32_t sub_map_no = (utl::xxh32_t::digest(account.to_string()) % xstake::XPROPERTY_SPLITED_NUM) + 1;
    std::string property{xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE};
    property += "-" + std::to_string(sub_map_no);

    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);
    auto value_str = std::string((char *)stream.data(), stream.size());
    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_set_property_contract_voter_dividend_reward_key");
        MAP_SET(property, account.to_string(), value_str);
    }
}

void xtop_table_reward_claiming_contract::recv_voter_dividend_reward(uint64_t issuance_clock_height, std::map<std::string, top::xstake::uint128_t> const & rewards) {
    XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_recv_voter_dividend_reward");


    auto const & source_address = SOURCE_ADDRESS();
    auto const & self_address = SELF_ADDRESS();

    xdbg("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] pid: %d, self address: %s, source address: %s, issuance_clock_height: %llu",
        getpid(), self_address.c_str(), source_address.c_str(), issuance_clock_height);

    if (rewards.size() == 0) {
        xwarn("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] pid: %d, rewards size 0", getpid());
        return;
    }

    XCONTRACT_ENSURE(sys_contract_zec_reward_addr == source_address, "xtop_table_reward_claiming_contract::recv_voter_dividend_reward from invalid address: " + source_address);
    std::map<std::string, std::string> adv_votes;
    std::string base_addr{};
    uint32_t table_id{static_cast<uint32_t>(-1)};
    XCONTRACT_ENSURE(data::xdatautil::extract_parts(self_address.value(), base_addr, table_id),
                     "xtop_table_reward_claiming_contract::recv_voter_dividend_reward: extract table id failed");

    try {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_get_property_contract_pollable_key");
        MAP_COPY_GET(xstake::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_votes, data::xdatautil::serialize_owner_str(sys_contract_sharding_vote_addr, table_id));
    } catch (std::runtime_error & e) {
        xdbg("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] MAP_COPY_GET XPORPERTY_CONTRACT_POLLABLE_KEY error:%s", e.what());
    }

    for (auto i = 1; i <= xstake::XPROPERTY_SPLITED_NUM; ++i) {
        std::string property_name{xstake::XPORPERTY_CONTRACT_VOTES_KEY_BASE};
        property_name += "-" + std::to_string(i);

        std::map<std::string, std::string> voters;

        {
            XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_get_property_contract_votes_key");
            MAP_COPY_GET(property_name, voters, data::xdatautil::serialize_owner_str(sys_contract_sharding_vote_addr, table_id));
        }

        xdbg("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] vote maps %s size: %d, pid: %d", property_name.c_str(), voters.size(), getpid());
        //calc_voter_reward(voters);
        for (auto const & entity : voters) {
            auto const & account = entity.first;
            auto const & vote_table_str = entity.second;
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)vote_table_str.c_str(), (uint32_t)vote_table_str.size());
            std::map<std::string, uint64_t> votes_table;
            stream >> votes_table;
            xstake::xreward_record record;
            xdbg("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] voter: %s", account.c_str()); 
            get_vote_reward_record(common::xaccount_address_t{account}, record); // not care return value hear
            add_voter_reward(issuance_clock_height, votes_table, rewards, adv_votes, record);
            update_vote_reward_record(common::xaccount_address_t{account}, record);
        }       
    }
}

void xtop_table_reward_claiming_contract::add_voter_reward(uint64_t issuance_clock_height,
                                                           std::map<std::string, uint64_t> & votes_table,
                                                           std::map<std::string, top::xstake::uint128_t> const & rewards,
                                                           std::map<std::string, std::string> const & adv_votes,
                                                           xstake::xreward_record & record) {
    top::xstake::uint128_t node_vote_reward = 0;
    top::xstake::uint128_t voter_node_reward = 0;
    uint64_t node_total_votes = 0;
    uint64_t voter_node_votes = 0;
    record.issue_time = issuance_clock_height;
    for (auto const & adv_vote : votes_table) {
        auto const & adv = adv_vote.first;
        auto iter = rewards.find(adv);
        // account total rewards
        if (iter != rewards.end()) {
            node_vote_reward = iter->second;
            //node_vote_reward = static_cast<xuint128_t>(iter->second.first * xstake::REWARD_PRECISION) + iter->second.second;
        } else {
            node_vote_reward = 0;
            continue;
        }
        // account total votes
        auto iter2 = adv_votes.find(adv);
        if (iter2 != adv_votes.end()) {
            node_total_votes = base::xstring_utl::touint64(iter2->second);
        } else {
            node_total_votes = 0;
            continue;
        }
        // voter votes
        voter_node_votes = votes_table[adv];
        // voter reward
        voter_node_reward = node_vote_reward * voter_node_votes / node_total_votes;
        // add to property
        bool found = false;
        for (auto & node_reward : record.node_rewards) {
            if (node_reward.account == adv) {
                found = true;
                node_reward.accumulated += voter_node_reward;
                node_reward.unclaimed += voter_node_reward;
                node_reward.issue_time = issuance_clock_height;
                break;
            }
        }
        if (!found) {
            xstake::node_record_t voter_node_record;
            voter_node_record.account = adv;
            voter_node_record.accumulated = voter_node_reward;
            voter_node_record.unclaimed = voter_node_reward;
            voter_node_record.issue_time = issuance_clock_height;
            record.node_rewards.push_back(voter_node_record);
        }
        record.accumulated += voter_node_reward;
        record.unclaimed += voter_node_reward;
        xdbg(
            "[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] adv node: %s, node_vote_reward: [%llu, %u], node_total_votes: %llu, voter_node_votes: "
            "%llu, voter_node_reward: "
            "[%llu, %u], pid: %d",
            adv.c_str(),
            static_cast<uint64_t>(node_vote_reward / REWARD_PRECISION),
            static_cast<uint32_t>(node_vote_reward % REWARD_PRECISION),
            node_total_votes,
            voter_node_votes,
            static_cast<uint64_t>(voter_node_reward / REWARD_PRECISION),
            static_cast<uint32_t>(voter_node_reward % REWARD_PRECISION),
            getpid());
    }
}

void xtop_table_reward_claiming_contract::claimVoterDividend() {
    XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_claim_voter_dividend");

    common::xaccount_address_t account{SOURCE_ADDRESS()};
    xstake::xreward_record reward_record;
    XCONTRACT_ENSURE(get_vote_reward_record(account, reward_record) == 0, "claimVoterDividend account no reward");
    uint64_t cur_time = TIME();
    xdbg("[xtop_table_reward_claiming_contract::claimVoterDividend] balance:%llu, account: %s, pid: %d, cur_time: %llu, last_claim_time: %llu\n",
         GET_BALANCE(),
         account.c_str(),
         getpid(),
         cur_time,
         reward_record.last_claim_time);
    auto min_voter_dividend = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_voter_dividend);
    XCONTRACT_ENSURE(reward_record.unclaimed > min_voter_dividend, "claimVoterDividend no enough reward");

    // transfer to account
    xinfo("[xtop_table_reward_claiming_contract::claimVoterDividend] timer round: %" PRIu64 ", account: %s, reward:: [%llu, %u], pid:%d\n",
         TIME(),
         account.c_str(),
         static_cast<uint64_t>(reward_record.unclaimed / REWARD_PRECISION),
         static_cast<uint32_t>(reward_record.unclaimed % REWARD_PRECISION),
         getpid());
    XMETRICS_PACKET_INFO("sysContract_tableRewardClaiming_claim_voter_dividend", "timer round", std::to_string(TIME()), "source address", account.c_str(), "reward", std::to_string(static_cast<uint64_t>(reward_record.unclaimed / REWARD_PRECISION)));

    TRANSFER(account.to_string(), static_cast<uint64_t>(reward_record.unclaimed / REWARD_PRECISION));
    reward_record.unclaimed = reward_record.unclaimed % REWARD_PRECISION;
    reward_record.last_claim_time = cur_time;
    for (auto & node_reward : reward_record.node_rewards) {
        node_reward.unclaimed = 0;
        node_reward.last_claim_time = cur_time;
    }
    update_vote_reward_record(account, reward_record);
}

void xtop_table_reward_claiming_contract::update_working_reward_record(common::xaccount_address_t const & account, xstake::xreward_node_record const & record) {
    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);
    auto value_str = std::string((char *)stream.data(), stream.size());
    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_set_property_contract_node_reward_key");
        MAP_SET(xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY, account.to_string(), value_str);
    }
}

void xtop_table_reward_claiming_contract::update(common::xaccount_address_t const & node_account, uint64_t issuance_clock_height, top::xstake::uint128_t reward) {
    auto const & self_address = SELF_ADDRESS();
    xdbg("[xtop_table_reward_claiming_contract::update] self_address: %s, account: %s, reward: [%llu, %u], pid: %d",
        self_address.c_str(), node_account.c_str(),
        static_cast<uint64_t>(reward / REWARD_PRECISION),
        static_cast<uint32_t>(reward % REWARD_PRECISION),
        getpid());

    // update node rewards table
    std::string value_str;
    xstake::xreward_node_record record;
    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_get_property_contract_node_reward_key");
        int32_t ret = MAP_GET2(xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY, node_account.to_string(), value_str);
        // here if not success, means account has no reward record yet, so value_str is empty, using above record directly
        if (ret) xwarn("[xtop_table_reward_claiming_contract::update] get property empty, node account %s", node_account.c_str());
        if (!value_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
            record.serialize_from(stream);
        }
    }


    record.m_accumulated    += reward;
    record.m_unclaimed      += reward;
    record.m_issue_time     = issuance_clock_height;

    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);

    value_str = std::string((char *)stream.data(), stream.size());
    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_set_property_contract_node_reward_key");
        MAP_SET(xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY, node_account.to_string(), value_str);
    }
}

void xtop_table_reward_claiming_contract::recv_node_reward(uint64_t issuance_clock_height, std::map<std::string, top::xstake::uint128_t> const & rewards) {
    XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_recv_node_reward");

    auto const & source_address = SOURCE_ADDRESS();
    auto const & self_address = SELF_ADDRESS();

    xdbg("[xtop_table_reward_claiming_contract::recv_node_reward] pid: %d, source_address: %s, self_address: %s, issuance_clock_height:%llu, rewards size: %d",
         getpid(),
         source_address.c_str(),
         self_address.c_str(),
         issuance_clock_height,
         rewards.size());

    XCONTRACT_ENSURE(sys_contract_zec_reward_addr == source_address,
                     "[xtop_table_reward_claiming_contract::recv_node_reward] working reward is not from zec workload contract but from " + source_address);

    for (auto const & account_reward : rewards) {
        auto const & account = account_reward.first;
        auto const & reward = account_reward.second;

        xdbg("[xtop_table_reward_claiming_contract::recv_node_reward] pid:%d, account: %s, reward: [%llu, %u]\n",
            getpid(), account.c_str(),
            static_cast<uint64_t>(reward / REWARD_PRECISION),
            static_cast<uint32_t>(reward % REWARD_PRECISION));

        // update node rewards
        update(common::xaccount_address_t{account}, issuance_clock_height, reward);
    }
}

void xtop_table_reward_claiming_contract::claimNodeReward() {
    XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_claim_node_reward");

    common::xaccount_address_t account{SOURCE_ADDRESS()};
    xstake::xreward_node_record reward_record;
    XCONTRACT_ENSURE(get_working_reward_record(account, reward_record) == 0, "claimNodeReward node account no reward");
    uint64_t cur_time = TIME();
    xinfo("[xtop_table_reward_claiming_contract::claimNodeReward] balance: %" PRIu64 ", account: %s, cur_time: %" PRIu64, GET_BALANCE(), account.c_str(), cur_time);
    auto min_node_reward = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward);
    // transfer to account
    xinfo("[xtop_table_reward_claiming_contract::claimNodeReward] reward: [%" PRIu64 ", %" PRIu32 "], reward_str: %s, reward_upper: %" PRIu64 ", reward_lower: %" PRIu64
          ", last_claim_time: %" PRIu64 ", min_node_reward: %" PRIu64,
          static_cast<uint64_t>(reward_record.m_unclaimed / REWARD_PRECISION),
          static_cast<uint32_t>(reward_record.m_unclaimed % REWARD_PRECISION),
          reward_record.m_unclaimed.str().c_str(),
          reward_record.m_unclaimed.upper(),
          reward_record.m_unclaimed.lower(),
          reward_record.m_last_claim_time,
          min_node_reward);
    XCONTRACT_ENSURE(static_cast<uint64_t>(reward_record.m_unclaimed / REWARD_PRECISION) > min_node_reward, "claimNodeReward: node no enough reward");

    XMETRICS_PACKET_INFO("sysContract_tableRewardClaiming_claim_node_reward", "timer round", std::to_string(TIME()), "source address", account.c_str(), "reward", std::to_string(static_cast<uint64_t>(reward_record.m_unclaimed / REWARD_PRECISION)));

    TRANSFER(account.to_string(), static_cast<uint64_t>(reward_record.m_unclaimed / REWARD_PRECISION));

    reward_record.m_unclaimed -= reward_record.m_unclaimed / REWARD_PRECISION * REWARD_PRECISION;
    reward_record.m_last_claim_time = cur_time;
    update_working_reward_record(account, reward_record);
}

int32_t xtop_table_reward_claiming_contract::get_working_reward_record(common::xaccount_address_t const & account, xstake::xreward_node_record & record) {
    std::string value_str;

    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_get_property_contract_node_reward_key");
        int32_t ret = MAP_GET2(xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY, account.to_string(), value_str);
        if (ret) {
            xdbg("[xtop_table_reward_claiming_contract::get_working_reward_record] account: %s not exist", account.c_str());
            return -1;
        }
    }

    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        record.serialize_from(stream);
        return 0;
    }
    return -1;
}

int32_t xtop_table_reward_claiming_contract::get_vote_reward_record(common::xaccount_address_t const & account, xstake::xreward_record & record) {
    uint32_t sub_map_no = (utl::xxh32_t::digest(account.to_string()) % xstake::XPROPERTY_SPLITED_NUM) + 1;
    std::string property;
    property = property + xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE + "-" + std::to_string(sub_map_no);

    std::string value_str;

    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_get_property_contract_voter_dividend_reward_key");
        if (!MAP_FIELD_EXIST(property, account.to_string())) {
            xdbg("[xtop_table_reward_claiming_contract::get_vote_reward_record] property: %s, account %s not exist", property.c_str(), account.c_str());
            return -1;
        } else {
            value_str = MAP_GET(property, account.to_string());
        }
    }

    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        record.serialize_from(stream);
        return 0;
    }
    return -1;
}

NS_END4
