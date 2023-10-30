// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xreward/xtable_reward_claiming_contract.h"

#include "xbase/xcontext.h"
#include "xchain_fork/xutility.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xdata/xdatautil.h"
#include "xdata/xnative_contract_address.h"
#include "xmetrics/xmetrics.h"

NS_BEG4(top, xvm, system_contracts, reward)
using namespace top::base;
xtop_table_reward_claiming_contract::xtop_table_reward_claiming_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {
}

void xtop_table_reward_claiming_contract::setup() {
    const int old_tables_count = 256;
    uint32_t table_id = 0;
    if (!EXTRACT_TABLE_ID(SELF_ADDRESS(), table_id)) {
        xwarn("[xtop_table_reward_claiming_contract::setup] EXTRACT_TABLE_ID failed, node reward pid: %d, account: %s", getpid(), SELF_ADDRESS().to_string().c_str());
        return;
    }
    xdbg("[xtop_table_reward_claiming_contract::setup] table id: %d", table_id);

    uint64_t acc_token = 0;
    uint32_t acc_token_decimals = 0;
    for (auto i = 1; i <= data::system_contract::XPROPERTY_SPLITED_NUM; i++) {
        std::string property{data::system_contract::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE};
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
                    data::system_contract::xreward_record record;
                    auto detail = _p.second;
                    base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                    record.serialize_from(stream);
                    MAP_SET(property, _p.first, _p.second);
                    acc_token += static_cast<uint64_t>(record.unclaimed / data::system_contract::REWARD_PRECISION);
                    acc_token_decimals += static_cast<uint32_t>(record.unclaimed % data::system_contract::REWARD_PRECISION);
                }
            }
        }
    }

    MAP_CREATE(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY);
    {
        for (auto i = 0; i < old_tables_count; i++) {
            auto table_addr = std::string{sys_contract_sharding_reward_claiming_addr} + "@" + base::xstring_utl::tostring(i);
            std::vector<std::pair<std::string, std::string>> db_kv_124;
            chain_data::xchain_data_processor_t::get_stake_map_property(
                common::xlegacy_account_address_t{table_addr}, data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, db_kv_124);
            for (auto const & _p : db_kv_124) {
                base::xvaccount_t vaccount{_p.first};
                auto account_table_id = vaccount.get_ledger_subaddr();
                if (static_cast<uint16_t>(account_table_id) != static_cast<uint16_t>(table_id)) {
                    continue;
                }
                MAP_SET(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, _p.first, _p.second);
                data::system_contract::xreward_node_record record;
                auto detail = _p.second;
                base::xstream_t stream{base::xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                record.serialize_from(stream);
                acc_token += static_cast<uint64_t>(record.m_unclaimed / data::system_contract::REWARD_PRECISION);
                acc_token_decimals += static_cast<uint32_t>(record.m_unclaimed % data::system_contract::REWARD_PRECISION);
            }
        }
    }

    acc_token += (acc_token_decimals / data::system_contract::REWARD_PRECISION);
    if (acc_token_decimals % data::system_contract::REWARD_PRECISION != 0) {
        acc_token += 1;
    }
    TOP_TOKEN_INCREASE(acc_token);
}

xcontract::xcontract_base * xtop_table_reward_claiming_contract::clone() {
    return new xtop_table_reward_claiming_contract{network_id()};
}

void xtop_table_reward_claiming_contract::update_vote_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_record const & record) {
    uint32_t sub_map_no = (utl::xxh32_t::digest(account.to_string()) % data::system_contract::XPROPERTY_SPLITED_NUM) + 1;
    std::string property{data::system_contract::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE};
    property += "-" + std::to_string(sub_map_no);

    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);
    auto value_str = std::string((char *)stream.data(), stream.size());
    { MAP_SET(property, account.to_string(), value_str); }
}

void xtop_table_reward_claiming_contract::get_section_voters_info(const std::string & key,
                                                                  uint32_t table_id,
                                                                  std::map<std::string, std::string> & voters,
                                                                  std::map<std::string, std::map<std::string, std::string>> & pledge_votes_map,
                                                                  std::map<std::string, uint64_t> & stored_expire_token_map) {
    { MAP_COPY_GET(key, voters, data::xdatautil::serialize_owner_str(sys_contract_sharding_vote_addr, table_id)); }
    xinfo("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] reward-claiming table_id:%d,vote_valid_rate vote maps %s size: %d, pid: %d",
          table_id,
          key.c_str(),
          voters.size(),
          getpid());

    for (auto const & entity : voters) {
        auto const & account = entity.first;
        std::map<std::string, std::string> pledge_votes;
        MAP_COPY_GET(data::XPROPERTY_PLEDGE_VOTE_KEY, pledge_votes, account);
        if (pledge_votes.empty()) {
            xdbg("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] reward-claiming pledge votes is empty,table_id:%d,account:%s", table_id, account);
            // do nothing
            continue;
        }
        pledge_votes_map.insert({account, pledge_votes});

        std::string val = STRING_GET2(data::XPROPERTY_EXPIRE_VOTE_TOKEN_KEY, account);
        stored_expire_token_map.insert({account, xstring_utl::touint64(val)});
    }
}

void xtop_table_reward_claiming_contract::calc_section_votes_table_and_adv_vote(std::map<std::string, std::string> & voters,
                                                                                std::map<std::string, std::map<std::string, std::string>> & pledge_votes_map,
                                                                                std::map<std::string, uint64_t> & stored_expire_token_map,
                                                                                std::map<std::string, std::map<std::string, uint64_t>> & votes_table_map,
                                                                                std::map<std::string, std::string> & adv_votes,
                                                                                uint64_t table_id,
                                                                                common::xlogic_time_t m_timer_height) {
    for (auto const & entity : voters) {
        auto const & account = entity.first;
        auto const & vote_table_str = entity.second;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)vote_table_str.c_str(), (uint32_t)vote_table_str.size());
        std::map<std::string, uint64_t> votes_table;
        stream >> votes_table;
        pledge_votes_map.find(account);
        std::map<std::string, std::string> pledge_votes;

        auto iter = pledge_votes_map.find(account);
        if (iter == pledge_votes_map.end()) {
            continue;
        }
        pledge_votes = iter->second;

        uint64_t stored_expire_token = 0;
        auto iter2 = stored_expire_token_map.find(account);
        if (iter2 != stored_expire_token_map.end()) {
            stored_expire_token = iter2->second;
        }
        uint64_t calc_expire_token{0};
        uint64_t unexpire_vote_num{0};
        uint64_t vote_sum{0};
        for (auto & v : pledge_votes) {
            uint64_t vote_num{0};
            uint16_t duration{0};
            uint64_t lock_time{0};
            base::xstream_t first_stream{xcontext_t::instance(), (uint8_t *)v.first.data(), static_cast<uint32_t>(v.first.size())};
            first_stream >> duration;
            first_stream >> lock_time;

            base::xstream_t second_stream{xcontext_t::instance(), (uint8_t *)v.second.data(), static_cast<uint32_t>(v.second.size())};
            second_stream >> vote_num;
            xdbg("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] reward-claiming pledge info, table_id:%d, account:%s, vote_num:%d,duration:%d,lock_time:%d",
                 table_id,
                 account,
                 vote_num,
                 duration,
                 lock_time);
            vote_sum += vote_num;

            std::cout << "account: " << account;
            std::cout << ", m_timer_height: " << m_timer_height;
            std::cout << ", lock_time: " << lock_time;
            std::cout << ", duration: " << duration;
            std::cout << ", vote_num: " << vote_num << std::endl;
#ifdef PERIOD_MOCK
            if (m_timer_height - lock_time >= duration / 60) {
#else
            if (m_timer_height - lock_time >= duration * 24 * 60 * 6) {
#endif
                std::cout << m_timer_height << " - " << lock_time << " >= " << duration * 24 * 60 * 6 << std::endl;
                if (0 != duration) {  // if not calculated in XPROPERTY_EXPIRE_VOTE_TOKEN_KEY
                    calc_expire_token += store::xaccount_context_t::get_top_by_vote(vote_num, duration);
                }
            } else {
                unexpire_vote_num += vote_num;
            }
        }
        auto valid_vote_sum = (stored_expire_token + calc_expire_token) / 1000000 + unexpire_vote_num;
        std::cout << "account: " << account;
        std::cout << ", stored_expire_token: " << stored_expire_token;
        std::cout << ", calc_expire_token: " << calc_expire_token;
        std::cout << ", unexpire_vote_num: " << unexpire_vote_num;
        std::cout << ", valid_vote_sum: " << valid_vote_sum;
        std::cout << ", vote_sum: " << vote_sum << std::endl;
        xdbg(
            "[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] reward-claiming valid vote info,table_id:%d,"
            "account:%s,stored_expire_token:%d,calc_expire_token:%d,unexpire_vote_num:%d,valid_vote_sum:%d,vote_sum:%d",
            table_id,
            account,
            stored_expire_token,
            calc_expire_token,
            unexpire_vote_num,
            valid_vote_sum,
            vote_sum);

        for (auto & v : votes_table) {
            auto adv = v.first;
            auto valid_vote = (v.second * valid_vote_sum) / vote_sum;
            std::cout << "adv: " << adv;
            std::cout << ", vote: " << v.second;
            std::cout << ", valid_vote: " << valid_vote << std::endl;
            votes_table[adv] = valid_vote;
            auto iter = adv_votes.find(adv);
            if (iter != adv_votes.end()) {
                adv_votes[adv] = std::to_string(base::xstring_utl::touint64(iter->second) + valid_vote);
            } else {
                adv_votes[adv] = std::to_string(valid_vote);
            }
        }
        votes_table_map.insert({account, votes_table});
    }
}

void xtop_table_reward_claiming_contract::calc_votes_table_and_adv_vote(std::map<std::string, std::map<std::string, uint64_t>> & votes_table_map,
                                                                        std::map<std::string, std::string> & adv_votes,
                                                                        uint32_t table_id) {
    auto m_timer_height = TIME();
    for (auto i = 1; i <= data::system_contract::XPROPERTY_SPLITED_NUM; ++i) {
        std::string property_name{data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY_BASE};
        property_name += "-" + std::to_string(i);
        std::map<std::string, std::string> voters;
        std::map<std::string, std::map<std::string, std::string>> pledge_votes_map;
        std::map<std::string, uint64_t> stored_expire_token_map;
        get_section_voters_info(property_name, table_id, voters, pledge_votes_map, stored_expire_token_map);
        calc_section_votes_table_and_adv_vote(voters, pledge_votes_map, stored_expire_token_map, votes_table_map, adv_votes, table_id, m_timer_height);
    }
}

void xtop_table_reward_claiming_contract::recv_voter_dividend_reward(uint64_t issuance_clock_height, std::map<std::string, ::uint128_t> const & rewards) {
    auto const & source_address = SOURCE_ADDRESS();
    auto const & self_address = SELF_ADDRESS();

    xinfo("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] self address: %s, source address: %s, issuance_clock_height: %llu",
          self_address.to_string().c_str(),
          source_address.c_str(),
          issuance_clock_height);

    if (rewards.size() == 0) {
        xwarn("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] rewards size 0");
        return;
    }

    XCONTRACT_ENSURE(sys_contract_zec_reward_addr == source_address, "xtop_table_reward_claiming_contract::recv_voter_dividend_reward from invalid address: " + source_address);
    std::map<std::string, std::string> adv_votes;
    std::string base_addr{};
    uint32_t table_id{static_cast<uint32_t>(-1)};
    XCONTRACT_ENSURE(data::xdatautil::extract_parts(self_address.to_string(), base_addr, table_id),
                     "xtop_table_reward_claiming_contract::recv_voter_dividend_reward: extract table id failed");

    std::map<std::string, std::map<std::string, uint64_t>> votes_table_map;
    auto is_firked = chain_fork::xutility_t::is_forked(fork_points::v11400_vote_reward_claiming, TIME());
    if (is_firked == true) {
        calc_votes_table_and_adv_vote(votes_table_map, adv_votes, table_id);
    } else {
        try {
            MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_votes, data::xdatautil::serialize_owner_str(sys_contract_sharding_vote_addr, table_id));
        } catch (std::runtime_error & e) {
            xwarn("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] MAP_COPY_GET XPORPERTY_CONTRACT_POLLABLE_KEY error:%s", e.what());
        }
    }

    for (auto i = 1; i <= data::system_contract::XPROPERTY_SPLITED_NUM; ++i) {
        std::string property_name{data::system_contract::XPORPERTY_CONTRACT_VOTES_KEY_BASE};
        property_name += "-" + std::to_string(i);

        std::map<std::string, std::string> voters;
        { MAP_COPY_GET(property_name, voters, data::xdatautil::serialize_owner_str(sys_contract_sharding_vote_addr, table_id)); }

        xdbg("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] vote maps %s size: %d, pid: %d", property_name.c_str(), voters.size(), getpid());
        // calc_voter_reward(voters);
        for (auto const & entity : voters) {
            auto const & account = entity.first;
            std::map<std::string, uint64_t> votes_table;

            if (is_firked == true) {
                auto iter = votes_table_map.find(account);
                if (iter != votes_table_map.end()) {
                    votes_table = iter->second;
                }
            } else {
                auto const & vote_table_str = entity.second;
                base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)vote_table_str.c_str(), (uint32_t)vote_table_str.size());
                stream >> votes_table;
            }

            data::system_contract::xreward_record record;
            xinfo("[xtop_table_reward_claiming_contract::recv_voter_dividend_reward] voter: %s", account.c_str());
            get_vote_reward_record(common::xaccount_address_t{account}, record);  // not care return value hear
            add_voter_reward(issuance_clock_height, votes_table, rewards, adv_votes, record);
            update_vote_reward_record(common::xaccount_address_t{account}, record);
        }
    }
}

void xtop_table_reward_claiming_contract::add_voter_reward(uint64_t issuance_clock_height,
                                                           std::map<std::string, uint64_t> & votes_table,
                                                           std::map<std::string, ::uint128_t> const & rewards,
                                                           std::map<std::string, std::string> const & adv_votes,
                                                           data::system_contract::xreward_record & record) {
    ::uint128_t node_vote_reward = 0;
    ::uint128_t voter_node_reward = 0;
    uint64_t node_total_votes = 0;
    uint64_t voter_node_votes = 0;
    record.issue_time = issuance_clock_height;
    for (auto const & adv_vote : votes_table) {
        auto const & adv = adv_vote.first;
        auto iter = rewards.find(adv);
        // account total rewards
        if (iter != rewards.end()) {
            node_vote_reward = iter->second;
            // node_vote_reward = static_cast<xuint128_t>(iter->second.first * xstake::REWARD_PRECISION) + iter->second.second;
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
            data::system_contract::node_record_t voter_node_record;
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
            static_cast<uint64_t>(node_vote_reward / data::system_contract::REWARD_PRECISION),
            static_cast<uint32_t>(node_vote_reward % data::system_contract::REWARD_PRECISION),
            node_total_votes,
            voter_node_votes,
            static_cast<uint64_t>(voter_node_reward / data::system_contract::REWARD_PRECISION),
            static_cast<uint32_t>(voter_node_reward % data::system_contract::REWARD_PRECISION),
            getpid());
    }
}

void xtop_table_reward_claiming_contract::claimVoterDividend() {
    common::xaccount_address_t account{SOURCE_ADDRESS()};
    data::system_contract::xreward_record reward_record;
    XCONTRACT_ENSURE(get_vote_reward_record(account, reward_record) == 0, "claimVoterDividend account no reward");
    uint64_t cur_time = TIME();
    xinfo("[xtop_table_reward_claiming_contract::claimVoterDividend] balance:%llu, account: %s, pid: %d, cur_time: %llu, last_claim_time: %llu\n",
          GET_BALANCE(),
          account.to_string().c_str(),
          getpid(),
          cur_time,
          reward_record.last_claim_time);
    auto min_voter_dividend = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_voter_dividend);
    XCONTRACT_ENSURE(reward_record.unclaimed > min_voter_dividend, "claimVoterDividend no enough reward");

    // transfer to account
    xinfo("[xtop_table_reward_claiming_contract::claimVoterDividend] timer round: %" PRIu64 ", account: %s, reward:: [%llu, %u], pid:%d\n",
          TIME(),
          account.to_string().c_str(),
          static_cast<uint64_t>(reward_record.unclaimed / data::system_contract::REWARD_PRECISION),
          static_cast<uint32_t>(reward_record.unclaimed % data::system_contract::REWARD_PRECISION),
          getpid());
    XMETRICS_PACKET_INFO("sysContract_tableRewardClaiming_claim_voter_dividend",
                         "timer round",
                         std::to_string(TIME()),
                         "source address",
                         account.to_string().c_str(),
                         "reward",
                         std::to_string(static_cast<uint64_t>(reward_record.unclaimed / data::system_contract::REWARD_PRECISION)));

    TRANSFER(account.to_string(), static_cast<uint64_t>(reward_record.unclaimed / data::system_contract::REWARD_PRECISION));
    reward_record.unclaimed = reward_record.unclaimed % data::system_contract::REWARD_PRECISION;
    reward_record.last_claim_time = cur_time;
    for (auto & node_reward : reward_record.node_rewards) {
        node_reward.unclaimed = 0;
        node_reward.last_claim_time = cur_time;
    }
    update_vote_reward_record(account, reward_record);
}

void xtop_table_reward_claiming_contract::update_working_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_node_record const & record) {
    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);
    auto value_str = std::string((char *)stream.data(), stream.size());
    { MAP_SET(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, account.to_string(), value_str); }
}

void xtop_table_reward_claiming_contract::update(common::xaccount_address_t const & node_account, uint64_t issuance_clock_height, ::uint128_t reward) {
    auto const & self_address = SELF_ADDRESS();
    xinfo("[xtop_table_reward_claiming_contract::update] self_address: %s, account: %s, reward: [%llu, %u], pid: %d",
          self_address.to_string().c_str(),
          node_account.to_string().c_str(),
          static_cast<uint64_t>(reward / data::system_contract::REWARD_PRECISION),
          static_cast<uint32_t>(reward % data::system_contract::REWARD_PRECISION),
          getpid());

    // update node rewards table
    std::string value_str;
    data::system_contract::xreward_node_record record;
    {
        int32_t ret = MAP_GET2(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, node_account.to_string(), value_str);
        // here if not success, means account has no reward record yet, so value_str is empty, using above record directly
        if (ret)
            xwarn("[xtop_table_reward_claiming_contract::update] get property empty, node account %s", node_account.to_string().c_str());
        if (!value_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
            record.serialize_from(stream);
        }
    }

    record.m_accumulated += reward;
    record.m_unclaimed += reward;
    record.m_issue_time = issuance_clock_height;

    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);

    value_str = std::string((char *)stream.data(), stream.size());
    { MAP_SET(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, node_account.to_string(), value_str); }
}

void xtop_table_reward_claiming_contract::recv_node_reward(uint64_t issuance_clock_height, std::map<std::string, ::uint128_t> const & rewards) {
    auto const & source_address = SOURCE_ADDRESS();
    auto const & self_address = SELF_ADDRESS();

    xinfo("[xtop_table_reward_claiming_contract::recv_node_reward] pid: %d, source_address: %s, self_address: %s, issuance_clock_height:%llu, rewards size: %d",
          getpid(),
          source_address.c_str(),
          self_address.to_string().c_str(),
          issuance_clock_height,
          rewards.size());

    XCONTRACT_ENSURE(sys_contract_zec_reward_addr == source_address,
                     "[xtop_table_reward_claiming_contract::recv_node_reward] working reward is not from zec workload contract but from " + source_address);

    for (auto const & account_reward : rewards) {
        auto const & account = account_reward.first;
        auto const & reward = account_reward.second;

        xinfo("[xtop_table_reward_claiming_contract::recv_node_reward] account: %s, reward: [%llu, %u]\n",
              account.c_str(),
              static_cast<uint64_t>(reward / data::system_contract::REWARD_PRECISION),
              static_cast<uint32_t>(reward % data::system_contract::REWARD_PRECISION));

        // update node rewards
        update(common::xaccount_address_t{account}, issuance_clock_height, reward);
    }
}

void xtop_table_reward_claiming_contract::claimNodeReward() {
    common::xaccount_address_t account{SOURCE_ADDRESS()};
    data::system_contract::xreward_node_record reward_record;
    XCONTRACT_ENSURE(get_working_reward_record(account, reward_record) == 0, "claimNodeReward node account no reward");
    uint64_t cur_time = TIME();
    xinfo("[xtop_table_reward_claiming_contract::claimNodeReward] balance: %" PRIu64 ", account: %s, cur_time: %" PRIu64, GET_BALANCE(), account.to_string().c_str(), cur_time);
    auto min_node_reward = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward);
    // transfer to account
    xinfo("[xtop_table_reward_claiming_contract::claimNodeReward] reward: [%" PRIu64 ", %" PRIu32 "], reward_str: %s, reward_upper: %" PRIu64 ", reward_lower: %" PRIu64
          ", last_claim_time: %" PRIu64 ", min_node_reward: %" PRIu64,
          static_cast<uint64_t>(reward_record.m_unclaimed / data::system_contract::REWARD_PRECISION),
          static_cast<uint32_t>(reward_record.m_unclaimed % data::system_contract::REWARD_PRECISION),
          reward_record.m_unclaimed.str().c_str(),
          reward_record.m_unclaimed.upper(),
          reward_record.m_unclaimed.lower(),
          reward_record.m_last_claim_time,
          min_node_reward);
    XCONTRACT_ENSURE(static_cast<uint64_t>(reward_record.m_unclaimed / data::system_contract::REWARD_PRECISION) > min_node_reward, "claimNodeReward: node no enough reward");

    XMETRICS_PACKET_INFO("sysContract_tableRewardClaiming_claim_node_reward",
                         "timer round",
                         std::to_string(TIME()),
                         "source address",
                         account.to_string().c_str(),
                         "reward",
                         std::to_string(static_cast<uint64_t>(reward_record.m_unclaimed / data::system_contract::REWARD_PRECISION)));

    TRANSFER(account.to_string(), static_cast<uint64_t>(reward_record.m_unclaimed / data::system_contract::REWARD_PRECISION));

    reward_record.m_unclaimed -= reward_record.m_unclaimed / data::system_contract::REWARD_PRECISION * data::system_contract::REWARD_PRECISION;
    reward_record.m_last_claim_time = cur_time;
    update_working_reward_record(account, reward_record);
}

int32_t xtop_table_reward_claiming_contract::get_working_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_node_record & record) {
    std::string value_str;

    {
        int32_t ret = MAP_GET2(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, account.to_string(), value_str);
        if (ret) {
            xdbg("[xtop_table_reward_claiming_contract::get_working_reward_record] account: %s not exist", account.to_string().c_str());
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

int32_t xtop_table_reward_claiming_contract::get_vote_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_record & record) {
    uint32_t sub_map_no = (utl::xxh32_t::digest(account.to_string()) % data::system_contract::XPROPERTY_SPLITED_NUM) + 1;
    std::string property;
    property = property + data::system_contract::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE + "-" + std::to_string(sub_map_no);

    std::string value_str;

    {
        if (!MAP_FIELD_EXIST(property, account.to_string())) {
            xdbg("[xtop_table_reward_claiming_contract::get_vote_reward_record] property: %s, account %s not exist", property.c_str(), account.to_string().c_str());
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
