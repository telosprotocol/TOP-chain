// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xslash/xtable_statistic_cons_contract.h"

#include "xbase/xmem.h"
#include "xcertauth/xcertauth_face.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xcommon/xip.h"
#include "xdata/xdata_common.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xmetrics/xmetrics.h"
#include "xvm/manager/xcontract_manager.h"

using namespace top::base;
using namespace top::data;
using namespace top::data::system_contract;

NS_BEG3(top, xvm, xcontract)

#define FULLTABLE_NUM "FULLTABLE_NUM"
#define FULLTABLE_HEIGHT "FULLTABLE_HEIGHT"

xtable_statistic_cons_contract::xtable_statistic_cons_contract(common::xnetwork_id_t const& network_id)
    : xbase_t { network_id }
{
}

void xtable_statistic_cons_contract::setup()
{
    // initialize map key
    MAP_CREATE(XPORPERTY_CONTRACT_WORKLOAD_KEY);

    MAP_CREATE(XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY);
    MAP_SET(XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_NUM, "0");
    MAP_SET(XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_HEIGHT, "0");
}

void xtable_statistic_cons_contract::on_collect_statistic_info_cons(xstatistics_cons_data_t const& statistic_data, xfulltableblock_statistic_accounts const& statistic_accounts,
    uint64_t block_height)
{
    XMETRICS_TIME_RECORD("sysContract_tableStatistic_on_collect_statistic_info");
    XMETRICS_CPU_TIME_RECORD("sysContract_tableStatistic_on_collect_statistic_info");
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::contract_table_statistic_exec_fullblock, 1);

    auto const& source_addr = SOURCE_ADDRESS();
    auto const& account = SELF_ADDRESS();

    std::string base_addr = "";
    uint32_t table_id = 0;

    XCONTRACT_ENSURE(data::xdatautil::extract_parts(source_addr, base_addr, table_id), "source address extract base_addr or table_id error!");
    xdbg("[xtable_statistic_cons_contract][on_collect_statistic_info] self_account %s, source_addr %s, base_addr %s\n", account.c_str(), source_addr.c_str(), base_addr.c_str());
    XCONTRACT_ENSURE(source_addr == account.value(), "invalid source addr's call!");
    XCONTRACT_ENSURE(base_addr == top::sys_contract_sharding_statistic_info_addr || source_addr == top::sys_contract_eth_table_statistic_info_addr, "invalid source base's call!");

    // check if the block processed
    uint64_t cur_statistic_height = 0;
    std::string value_str;
    try {
        XMETRICS_TIME_RECORD("sysContract_tableStatistic_get_property_contract_fulltable_height");
        if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_HEIGHT))
            value_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_HEIGHT);
    } catch (std::runtime_error const& e) {
        xwarn("[xtable_statistic_cons_contract][on_collect_statistic_info] read summarized slash info error:%s", e.what());
        throw;
    }

    if (!value_str.empty()) {
        cur_statistic_height = base::xstring_utl::touint64(value_str);
    }

    if (block_height <= cur_statistic_height) {
        xwarn("[xtable_statistic_cons_contract][on_collect_statistic_info] duplicated block, block height: %" PRIu64 ", current statistic block %" PRIu64,
            block_height,
            cur_statistic_height);
        return;
    }

    xinfo("[xtable_statistic_cons_contract][on_collect_statistic_info] enter collect statistic data, fullblock height: %" PRIu64 ", tgas: %ld, contract addr: %s, table_id: %u",
        block_height,
        tgas,
        source_addr.c_str(),
        table_id);

    uint32_t summarize_fulltableblock_num = 0;
    std::string summarize_fulltableblock_num_str;
    try {
        XMETRICS_TIME_RECORD("sysContract_tableStatistic_get_property_contract_tableblock_num_key");
        if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_NUM)) {
            summarize_fulltableblock_num_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_NUM);
        }
    } catch (std::runtime_error& e) {
        xwarn("[xtable_statistic_cons_contract][on_collect_statistic_info] read summarized tableblock num error:%s", e.what());
        throw;
    }

    if (!summarize_fulltableblock_num_str.empty()) {
        summarize_fulltableblock_num = base::xstring_utl::touint32(summarize_fulltableblock_num_str);
    }

    MAP_SET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_HEIGHT, base::xstring_utl::tostring(block_height));
    summarize_fulltableblock_num++;
    MAP_SET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_NUM, base::xstring_utl::tostring(summarize_fulltableblock_num));

    process_reward_statistic_data(statistic_data, statistic_accounts);
}

void xtable_statistic_cons_contract::process_reward_statistic_data(xstatistics_cons_data_t const& statistic_data, xfulltableblock_statistic_accounts const& statistic_accounts)
{
    XMETRICS_TIME_RECORD("sysContract_tableStatistic_process_workload_statistic_data");
    XMETRICS_CPU_TIME_RECORD("sysContract_tableStatistic_process_workload_statistic_data");
    auto const& group_reward = calc_reward_from_data(statistic_data, statistic_accounts);
    if (!group_reward.empty()) {
        update_reward(group_workload);
    }
}

std::map<common::xgroup_address_t, xgroup_cons_reward_t> xtable_statistic_cons_contract::calc_reward_from_data(xstatistics_cons_data_t const& statistic_data,
    xfulltableblock_statistic_accounts const& statistic_accounts)
{
    XMETRICS_TIME_RECORD("sysContractc_workload_get_workload_from_data");
    XMETRICS_CPU_TIME_RECORD("sysContractc_workload_get_workload_from_data");
    std::map<common::xgroup_address_t, xgroup_cons_reward_t> group_reward;

    for (auto const& static_item : statistic_data.detail) {
        auto elect_statistic = static_item.second;
        for (auto const& group_item : elect_statistic.group_statistics_data) {
            common::xgroup_address_t const& group_addr = group_item.first;
            xgroup_statistics_cons_data_t const& group_account_data = group_item.second;
            xvip2_t const& group_xvip2 = top::common::xip2_t { group_addr.network_id(),
                group_addr.zone_id(),
                group_addr.cluster_id(),
                group_addr.group_id(),
                (uint16_t)group_account_data.account_statistics_data.size(),
                static_item.first };
            xdbg("[xtable_statistic_cons_contract::calc_reward_from_data] group xvip2: %llu, %llu", group_xvip2.high_addr, group_xvip2.low_addr);

            auto account_group = statistic_accounts.accounts_detail.at(static_item.first);
            auto group_accounts = account_group.group_data[group_addr];
            for (size_t slotid = 0; slotid < group_account_data.account_statistics_data.size(); ++slotid) {
                auto account_str = group_accounts.account_data[slotid];
                uint64_t account_reward = group_account_data.account_statistics_data[slotid].burn_gas_value;
                if (account_reward > 0) {
                    auto it2 = group_reward.find(group_addr);
                    if (it2 == group_reward.end()) {
                        xgroup_cons_reward_t group_workload_info;
                        auto ret = group_reward.insert(std::make_pair(group_addr, group_workload_info));
                        XCONTRACT_ENSURE(ret.second, "insert workload failed");
                        it2 = ret.first;
                    }

                    it2->second.m_leader_reward[account_str.value()] += account_reward;
                }

                xdbg(
                    "[xtable_statistic_cons_contract::calc_reward_from_data] group_addr: [%s, network_id: %u, zone_id: %u, cluster_id: %u, group_id: %u], leader: %s, account_reward: %u",
                    group_addr.to_string().c_str(),
                    group_addr.network_id().value(),
                    group_addr.zone_id().value(),
                    group_addr.cluster_id().value(),
                    group_addr.group_id().value(),
                    account_str.c_str(),
                    account_reward);
            }
        }
    }
    return group_reward;
}


void xtable_statistic_cons_contract::update_reward(std::map<common::xgroup_address_t, xgroup_cons_reward_t> const& group_workload)
{
    XMETRICS_TIME_RECORD("sysContractc_workload_update_workload");
    XMETRICS_CPU_TIME_RECORD("sysContractc_workload_update_workload");
    for (auto const& one_group_workload : group_workload) {
        auto const& group_address = one_group_workload.first;
        auto const& cons_reward = one_group_workload.second;
        // get
        auto total_reward = get_reward(group_address);
        // update
        for (auto const& leader_workload : cons_reward.m_leader_reward) {
            auto const& leader = leader_workload.first;
            auto const& reward = leader_workload.second;
            total_reward.m_leader_reward[leader] += reward;

            xdbg("[xtable_statistic_cons_contract::update_reward] group: %u, leader: %s, reward: %d, total_reward: %d",
                group_address.group_id().value(),
                leader.c_str(),
                reward,
                total_reward.m_leader_count[leader]);
        }
        // set
        save_reward(group_address, total_reward);
    }
}

xgroup_cons_reward_t xtable_statistic_cons_contract::get_reward(common::xgroup_address_t const& group_address)
{
    std::string group_address_str;
    xstream_t stream(xcontext_t::instance());
    stream << group_address;
    group_address_str = std::string((const char*)stream.data(), stream.size());
    xgroup_cons_reward_t total_workload;
    {
        std::string value_str;
        if (MAP_GET2(XPORPERTY_CONTRACT_WORKLOAD_KEY, group_address_str, value_str)) {
            xdbg("[xtable_statistic_cons_contract::get_reward] group not exist: %s", group_address.to_string().c_str());
            total_workload.group_address_str = group_address_str;
        } else {
            xstream_t stream(xcontext_t::instance(), (uint8_t*)value_str.data(), value_str.size());
            total_workload.serialize_from(stream);
        }
    }
    return total_workload;
}


void xtable_statistic_cons_contract::save_reward(common::xgroup_address_t const& group_address, xgroup_cons_reward_t const& group_workload)
{
    xstream_t key_stream(xcontext_t::instance());
    key_stream << group_address;
    std::string group_address_str = std::string((const char*)key_stream.data(), key_stream.size());
    xstream_t stream(xcontext_t::instance());
    group_workload.serialize_to(stream);
    std::string value_str = std::string((const char*)stream.data(), stream.size());
    MAP_SET(XPORPERTY_CONTRACT_WORKLOAD_KEY, group_address_str, value_str);
}

void xtable_statistic_cons_contract::report_summarized_statistic_info_cons(common::xlogic_time_t timestamp)
{
    XMETRICS_TIME_RECORD("sysContract_tableStatistic_report_summarized_statistic_info");
    XMETRICS_CPU_TIME_RECORD("sysContract_tableStatistic_report_summarized_statistic_info");
    auto const& source_addr = SOURCE_ADDRESS();
    auto const& account = SELF_ADDRESS();

    std::string base_addr = "";
    uint32_t table_id = 0;

    XCONTRACT_ENSURE(data::xdatautil::extract_parts(source_addr, base_addr, table_id), "source address extract base_addr or table_id error!");
    xdbg("[xtable_statistic_cons_contract][report_summarized_statistic_info] self_account %s, source_addr %s, base_addr %s\n", account.c_str(), source_addr.c_str(), base_addr.c_str());
    XCONTRACT_ENSURE(source_addr == account.value(), "invalid source addr's call!");
    XCONTRACT_ENSURE(base_addr == top::sys_contract_sharding_statistic_info_addr || source_addr == top::sys_contract_eth_table_statistic_info_addr, "invalid source base's call!");

    uint32_t summarize_fulltableblock_num = 0;
    std::string value_str;
    try {
        XMETRICS_TIME_RECORD("sysContract_tableStatistic_get_property_contract_tableblock_num_key");
        if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_NUM)) {
            value_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_NUM);
        }
    } catch (std::runtime_error& e) {
        xwarn("[xtable_statistic_cons_contract][report_summarized_statistic_info] read summarized tableblock num error:%s", e.what());
        throw;
    }

    if (!value_str.empty()) {
        summarize_fulltableblock_num = base::xstring_utl::touint32(value_str);
    }

    if (0 == summarize_fulltableblock_num) {
        xinfo("[xtable_statistic_cons_contract][report_summarized_statistic_info] no summarized fulltable info, timer round %" PRIu64 ", table_id: %d",
            timestamp,
            table_id);
        return;
    }

    xinfo("[xtable_statistic_cons_contract][report_summarized_statistic_info] enter report summarized info, timer round: %" PRIu64 ", table_id: %u, contract addr: %s",
        timestamp,
        table_id,
        source_addr.c_str());

    {
        value_str.clear();
        uint64_t cur_statistic_height = 0;
        try {
            XMETRICS_TIME_RECORD("sysContract_tableStatistic_get_property_contract_fulltable_height_key");
            if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_HEIGHT))
                value_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_HEIGHT);
        } catch (std::runtime_error const& e) {
            xwarn("[xtable_statistic_cons_contract][report_summarized_statistic_info] read fulltable num error:%s", e.what());
            throw;
        }

        if (!value_str.empty()) {
            cur_statistic_height = base::xstring_utl::touint64(value_str);
        }

        base::xstream_t stream(base::xcontext_t::instance());
        stream << cur_statistic_height;

        xkinfo("[xtable_statistic_cons_contract][report_summarized_statistic_info] effective reprot summarized info, timer round %" PRIu64
               ", fulltableblock num: %u, cur_statistic_height: %" PRIu64 ", table_id: %u, contract addr: %s",
            timestamp,
            summarize_fulltableblock_num,
            cur_statistic_height,
            table_id,
            account.c_str());

        {
            XMETRICS_TIME_RECORD("sysContract_tableStatistic_remove_property_contract_tableblock_num_key");
            MAP_REMOVE(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_NUM);
        }

        XMETRICS_GAUGE(metrics::xmetrics_tag_t::contract_table_statistic_report_fullblock, 1);

    }

    upload_reward();
}


void xtable_statistic_cons_contract::upload_reward()
{
    XMETRICS_TIME_RECORD("sysContractc_workload_upload_workload");
    XMETRICS_CPU_TIME_RECORD("sysContractc_workload_upload_workload");
    std::map<std::string, std::string> group_workload_str;
    std::map<common::xgroup_address_t, xgroup_cons_reward_t> group_workload_upload;

    MAP_COPY_GET(XPORPERTY_CONTRACT_WORKLOAD_KEY, group_workload_str);
    for (auto it = group_workload_str.begin(); it != group_workload_str.end(); it++) {
        xstream_t key_stream(xcontext_t::instance(), (uint8_t*)it->first.data(), it->first.size());
        common::xgroup_address_t group_address;
        key_stream >> group_address;
        xstream_t stream(xcontext_t::instance(), (uint8_t*)it->second.data(), it->second.size());
        xgroup_cons_reward_t group_workload;
        group_workload.serialize_from(stream);

        for (auto const& leader_workload : group_workload.m_leader_reward) {
            auto const& leader = leader_workload.first;
            auto const& reward = leader_workload.second;

            auto it2 = group_workload_upload.find(group_address);
            if (it2 == group_workload_upload.end()) {
                xgroup_workload_t empty_workload;
                auto ret = group_workload_upload.insert(std::make_pair(group_address, empty_workload));
                it2 = ret.first;
            }
            it2->second.m_leader_reward[leader] += reward;
        }
    }
    if (group_workload_upload.size() > 0) {

        uint64_t height = 0;
        std::string value_str;
        if (MAP_GET2(XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, FULLTABLE_HEIGHT, value_str)) {
            xwarn("[xtable_statistic_cons_contract::upload_reward] table height not exist!");
        } else {
            if (!value_str.empty()) {
                height = base::xstring_utl::touint64(value_str);
            }
        }
      
        std::string group_workload_upload_str;
        {
            xstream_t stream(xcontext_t::instance());
            MAP_OBJECT_SERIALIZE2(stream, group_workload_upload);
            stream << height;
            group_workload_upload_str = std::string((char*)stream.data(), stream.size());
            xinfo("[xtable_statistic_cons_contract::upload_reward] %s upload workload to zec reward, group_workload_upload size: %d,  height: %lu ",
                SOURCE_ADDRESS().c_str(),
                group_workload_upload.size(),
                height);
        }
        {
            xstream_t stream(xcontext_t::instance());
            stream << group_workload_upload_str;
            //new contrace
            CALL(common::xaccount_address_t { sys_contract_zec_workload_addr }, "on_receive_reward", std::string((char*)stream.data(), stream.size()));
            group_workload_upload.clear();
        }

        MAP_CLEAR(XPORPERTY_CONTRACT_WORKLOAD_KEY);
    }
}

NS_END3
