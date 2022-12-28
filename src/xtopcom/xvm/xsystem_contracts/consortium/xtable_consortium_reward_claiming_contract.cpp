// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "xtable_consortium_reward_claiming_contract.h"
#include "xdata/xdatautil.h"
#include "xdata/xnative_contract_address.h"
#include "xmetrics/xmetrics.h"

NS_BEG3(top, xvm, consortium)

xtop_table_consortium_reward_claiming_contract::xtop_table_consortium_reward_claiming_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {
}

void xtop_table_consortium_reward_claiming_contract::setup() {
    const int old_tables_count = 256;
    uint32_t table_id = 0;
    if (!EXTRACT_TABLE_ID(SELF_ADDRESS(), table_id)) {
        xwarn("[xtop_table_consortium_reward_claiming_contract::setup] EXTRACT_TABLE_ID failed, node reward pid: %d, account: %s", getpid(), SELF_ADDRESS().to_string().c_str());
        return;
    }
    xdbg("[xtop_table_consortium_reward_claiming_contract::setup] table id: %d", table_id);

    
    MAP_CREATE(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY);
   
}

xcontract::xcontract_base * xtop_table_consortium_reward_claiming_contract::clone() {
    return new xtop_table_consortium_reward_claiming_contract{network_id()};
}

void xtop_table_consortium_reward_claiming_contract::update_working_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_node_record const & record) {
    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);
    auto value_str = std::string((char *)stream.data(), stream.size());
    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_set_property_contract_node_reward_key");
        MAP_SET(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, account.to_string(), value_str);
    }
}

void xtop_table_consortium_reward_claiming_contract::update(common::xaccount_address_t const & node_account, uint64_t issuance_clock_height, ::uint128_t reward) {
    auto const & self_address = SELF_ADDRESS();
    xinfo("[xtop_table_consortium_reward_claiming_contract::update] self_address: %s, account: %s, reward: [%llu, %u], pid: %d",
          self_address.to_string().c_str(),
          node_account.to_string().c_str(),
          static_cast<uint64_t>(reward / data::system_contract::REWARD_PRECISION),
          static_cast<uint32_t>(reward % data::system_contract::REWARD_PRECISION),
          getpid());

    // update node rewards table
    std::string value_str;
    data::system_contract::xreward_node_record record;
    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_get_property_contract_node_reward_key");
        int32_t ret = MAP_GET2(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, node_account.to_string(), value_str);
        // here if not success, means account has no reward record yet, so value_str is empty, using above record directly
        if (ret)
            xwarn("[xtop_table_consortium_reward_claiming_contract::update] get property empty, node account %s", node_account.to_string().c_str());
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
    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_set_property_contract_node_reward_key");
        MAP_SET(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, node_account.to_string(), value_str);
    }
}

void xtop_table_consortium_reward_claiming_contract::recv_node_reward(uint64_t issuance_clock_height, std::map<std::string, ::uint128_t> const & rewards) {
    XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_recv_node_reward");

    auto const & source_address = SOURCE_ADDRESS();
    auto const & self_address = SELF_ADDRESS();

    xinfo("[xtop_table_consortium_reward_claiming_contract::recv_node_reward] pid: %d, source_address: %s, self_address: %s, issuance_clock_height:%llu, rewards size: %d",
          getpid(),
          source_address.c_str(),
          self_address.to_string().c_str(),
          issuance_clock_height,
          rewards.size());

    XCONTRACT_ENSURE((sys_contract_zec_consortium_reward_addr == source_address), "[xtop_table_consortium_reward_claiming_contract::recv_node_reward] working reward is not from zec workload contract but from " + source_address);
 

    for (auto const & account_reward : rewards) {
        auto const & account = account_reward.first;
        auto const & reward = account_reward.second;

        xinfo("[xtop_table_consortium_reward_claiming_contract::recv_node_reward] account: %s, reward: [%llu, %u]\n",
              account.c_str(),
              static_cast<uint64_t>(reward / data::system_contract::REWARD_PRECISION),
              static_cast<uint32_t>(reward % data::system_contract::REWARD_PRECISION));

        // update node rewards
        update(common::xaccount_address_t{account}, issuance_clock_height, reward);
    }
}

void xtop_table_consortium_reward_claiming_contract::claimNodeReward() {
    XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_claim_node_reward");

    common::xaccount_address_t account{SOURCE_ADDRESS()};
    data::system_contract::xreward_node_record reward_record;
    XCONTRACT_ENSURE(get_working_reward_record(account, reward_record) == 0, "claimNodeReward node account no reward");
    uint64_t cur_time = TIME();
    xinfo("[xtop_table_consortium_reward_claiming_contract::claimNodeReward] balance: %" PRIu64 ", account: %s, cur_time: %" PRIu64, GET_BALANCE(), account.to_string().c_str(), cur_time);
    auto min_node_reward = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_node_reward);
    // transfer to account
    xinfo("[xtop_table_consortium_reward_claiming_contract::claimNodeReward] reward: [%" PRIu64 ", %" PRIu32 "], reward_str: %s, reward_upper: %" PRIu64 ", reward_lower: %" PRIu64
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

int32_t xtop_table_consortium_reward_claiming_contract::get_working_reward_record(common::xaccount_address_t const & account, data::system_contract::xreward_node_record & record) {
    std::string value_str;

    {
        XMETRICS_TIME_RECORD("sysContract_tableRewardClaiming_get_property_contract_node_reward_key");
        int32_t ret = MAP_GET2(data::system_contract::XPORPERTY_CONTRACT_NODE_REWARD_KEY, account.to_string(), value_str);
        if (ret) {
            xdbg("[xtop_table_consortium_reward_claiming_contract::get_working_reward_record] account: %s not exist", account.to_string().c_str());
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

NS_END3
