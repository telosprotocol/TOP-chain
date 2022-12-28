

#include "xzec_consortium_reward_contract.h"
#include "xbase/xutl.h"
#include "xbasic/xuint.hpp"
#include "xbasic/xutility.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xstore/xstore_error.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xblock_statistics_data.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/manager/xcontract_manager.h"
#include <iomanip>

using top::base::xcontext_t;
using top::base::xstream_t;
using top::base::xstring_utl;
using namespace top::data;

#if !defined(XZEC_MODULE)
#define XZEC_MODULE "sysContract_"
#endif

#define XCONTRACT_PREFIX "consortium_reward_contract_"

#define CONS_REWARD_CONTRACT XZEC_MODULE XCONTRACT_PREFIX

NS_BEG3(top, xvm, consortium)

xzec_consortium_reward_contract::xzec_consortium_reward_contract(common::xnetwork_id_t const& network_id)
    : xbase_t { network_id }
{
}

void xzec_consortium_reward_contract::setup()
{
     MAP_CREATE(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY);  // save dispatch tasks
    std::vector<std::pair<std::string, std::string>> db_kv_111;
    chain_data::xchain_data_processor_t::get_stake_map_property(common::xlegacy_account_address_t{SELF_ADDRESS()}, data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, db_kv_111);
    for (auto const & _p : db_kv_111) {
        MAP_SET(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, _p.first, _p.second);
    }

    MAP_CREATE(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE);  // save issuance
    std::vector<std::pair<std::string, std::string>> db_kv_141;
    chain_data::xchain_data_processor_t::get_stake_map_property(
        common::xlegacy_account_address_t{SELF_ADDRESS()}, data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, db_kv_141);
    for (auto const & _p : db_kv_141) {
        MAP_SET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, _p.first, _p.second);
    }

    STRING_CREATE(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY);
    std::string db_kv_142;
    chain_data::xchain_data_processor_t::get_stake_string_property(
        common::xlegacy_account_address_t{SELF_ADDRESS()}, data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY, db_kv_142);
    if (!db_kv_142.empty()) {
        STRING_SET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY, db_kv_142);
    } else {
        data::system_contract::xaccumulated_reward_record record;
        update_accumulated_record(record);
    }

    STRING_CREATE(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT);
    std::string last_read_rec_reg_contract_height{"0"};
    STRING_SET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT, last_read_rec_reg_contract_height);
    STRING_CREATE(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME);
    std::string last_read_rec_reg_contract_logic_time{"0"};
    STRING_SET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME, last_read_rec_reg_contract_logic_time);

    STRING_CREATE(data::system_contract::XPROPERTY_REWARD_DETAIL);
    MAP_CREATE(data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY);
    std::vector<std::pair<std::string, std::string>> db_kv_103;
    chain_data::xchain_data_processor_t::get_stake_map_property(common::xlegacy_account_address_t{SELF_ADDRESS()}, data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY, db_kv_103);
    for (auto const & _p : db_kv_103) {
        MAP_SET(data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY, _p.first, _p.second);
    }

     MAP_CREATE(data::system_contract::XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY);
}

void xzec_consortium_reward_contract::on_receive_reward(std::string const& table_info_str)
{
    XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "on_receive_reward");
    XMETRICS_COUNTER_INCREMENT(CONS_REWARD_CONTRACT "on_receive_reward", 1);
    XCONTRACT_ENSURE(!table_info_str.empty(), "workload_str empty");

    auto const& source_address = SOURCE_ADDRESS();
    std::string base_addr;
    uint32_t table_id;
    XCONTRACT_ENSURE(data::xdatautil::extract_parts(source_address, base_addr, table_id), "source address extract base_addr or table_id error!");
    XCONTRACT_ENSURE(base_addr == top::sys_contract_consortium_table_statistic_addr || source_address == top::sys_contract_consortium_eth_table_statistic_addr, "invalid source base's call!");

    xdbg("[xzec_consortium_reward_contract::on_receive_reward]  call from %s, pid: %d, ", source_address.c_str(), getpid());

    std::map<std::string, std::string> workload_str;
    std::string height_str;
    std::map<std::string, std::string> workload_str_new;

    {
        XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "on_receive_workload_map_get");
        MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY, workload_str);
        MAP_GET2(data::system_contract::XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY, std::to_string(table_id), height_str);
    }

    handle_reward_str(table_info_str, workload_str, height_str, workload_str_new);

    XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "on_receive_workload_map_set");
    xinfo("[xzec_consortium_reward_contract::on_receive_reward] data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY workload_str_new size %ld ", workload_str_new.size());
    for (auto it = workload_str_new.cbegin(); it != workload_str_new.end(); it++) {
        xinfo("[xzec_consortium_reward_contract::on_receive_reward] data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY add ");
        MAP_SET(data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY, it->first, it->second);
    }

    update_table_height(table_id, xstring_utl::touint64(height_str));
}

void xzec_consortium_reward_contract::handle_reward_str(const std::string& table_info_str,
    const std::map<std::string, std::string>& workload_str,
    const std::string& height_str,
    std::map<std::string, std::string>& workload_str_new)
{
    XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "handle_reward_str");
    std::map<common::xgroup_address_t, data::system_contract::xgroup_cons_reward_t> group_reward;

    uint64_t height = 0;
    {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)table_info_str.data(), table_info_str.size());
        MAP_OBJECT_DESERIALZE2(stream, group_reward);
        stream >> height;
    }
    xinfo("[xzec_workload_contract::handle_reward_str] group_reward size: %zu, height: %lu, last height: %lu \n",
        group_reward.size(),
        height,
        xstring_utl::touint64(height_str));

    XCONTRACT_ENSURE(xstring_utl::touint64(height_str) < height, "zec_last_read_height >= table_previous_height");

    update_reward(group_reward, workload_str, workload_str_new);
}

void xzec_consortium_reward_contract::update_reward(const std::map<common::xgroup_address_t, data::system_contract::xgroup_cons_reward_t>& group_workload,
    const std::map<std::string, std::string>& workload_str,
    std::map<std::string, std::string>& workload_new)
{
    XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "update_reward");
    for (auto const& one_group_workload : group_workload) {
        auto const& group_address = one_group_workload.first;
        auto const& cons_reward = one_group_workload.second;
        // get
        std::string group_address_str;
        {
            xstream_t stream(xcontext_t::instance());
            stream << group_address;
            group_address_str = std::string((const char*)stream.data(), stream.size());
        }
        data::system_contract::xgroup_cons_reward_t total_workload;
        {
            auto it = workload_str.find(group_address_str);
            if (it == workload_str.end()) {
                xdbg("[xzec_consortium_reward_contract::update_reward] group not exist: %s", group_address.to_string().c_str());
            } else {
                xstream_t stream(xcontext_t::instance(), (uint8_t*)it->second.data(), it->second.size());
                total_workload.serialize_from(stream);
            }
        }
        // update
        for (auto const& leader_workload : cons_reward.m_leader_reward) {
            auto const& leader = leader_workload.first;
            auto const& reward = leader_workload.second;
            total_workload.m_leader_reward[leader] += reward;

            xdbg("[xzec_consortium_reward_contract::update_reward] group: %u, leader: %s, reward: %d, total_reward: %d ",
                group_address.group_id().value(),
                leader.c_str(),
                reward,
                total_workload.m_leader_reward[leader]);
        }
        // set
        {
            xstream_t stream(xcontext_t::instance());
            total_workload.serialize_to(stream);
            workload_new.insert(std::make_pair(group_address_str, std::string((const char*)stream.data(), stream.size())));
            xdbg("[xzec_consortium_reward_contract::update_reward] workload_new size: %ld ", workload_new.size());
        }
    }
}

void xzec_consortium_reward_contract::on_timer(const common::xlogic_time_t onchain_timer_round)
{
    XMETRICS_COUNTER_INCREMENT(CONS_REWARD_CONTRACT "on_timer_Called", 1);
    XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "on_timer_ExecutionTime");
    XMETRICS_CPU_TIME_RECORD(CONS_REWARD_CONTRACT "on_timer_cpu_time");

    std::string source_address = SOURCE_ADDRESS();
    if (SELF_ADDRESS().to_string() != source_address) {
        xwarn("[xzec_consortium_reward_contract::on_timer] invalid call from %s", source_address.c_str());
        return;
    }

    if (MAP_SIZE(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY) > 0) {
        execute_task();
    } else {
        if (reward_expire_check(onchain_timer_round)) {
            reward_dispatch(onchain_timer_round);
        } else {
            update_reg_contract_read_status(onchain_timer_round);
        }
    }

    XMETRICS_COUNTER_INCREMENT(CONS_REWARD_CONTRACT "on_timer_Executed", 1);
}

void xzec_consortium_reward_contract::reward_dispatch(const common::xlogic_time_t current_time)
{
    XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "reward_ExecutionTime");
    XMETRICS_CPU_TIME_RECORD(CONS_REWARD_CONTRACT "reward_cpu_time");
    xdbg("[xzec_consortium_reward_contract::reward] pid:%d\n", getpid());

    // step1 get related params
    common::xlogic_time_t activation_time; // system activation time

    xreward_cons_property_param_t property_param; // property from self and other contracts
    data::system_contract::xissue_detail_v2 issue_detail; // issue details this round

    get_reward_param(current_time, activation_time, property_param, issue_detail);
    XCONTRACT_ENSURE(current_time > activation_time, "current_time <= activation_time");

    // step2 calculate node and table rewards
    std::map<common::xaccount_address_t, ::uint128_t> node_reward_detail; // <node, self reward>

    calc_nodes_rewards(current_time, current_time - activation_time, property_param, issue_detail, node_reward_detail);
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>> table_nodes_rewards; // <table, <node, reward>>
    std::map<common::xaccount_address_t, ::uint128_t> contract_rewards; // <table, total reward>
    calc_table_rewards(property_param, node_reward_detail, table_nodes_rewards, contract_rewards);

    dispatch_all_reward_v3(current_time, contract_rewards, table_nodes_rewards);
    // step4 update property
    property_param.accumulated_reward_record.last_issuance_time = current_time;
    update_property(current_time, property_param.accumulated_reward_record, issue_detail);
}

void xzec_consortium_reward_contract::update_property(const uint64_t current_time,
    data::system_contract::xaccumulated_reward_record const& record,
    data::system_contract::xissue_detail_v2 const& issue_detail)
{
    xdbg("[xzec_consortium_reward_contract::update_property]  current_time: %lu", current_time);
    update_accumulated_issuance(current_time);
    update_accumulated_record(record);
    update_issuance_detail(issue_detail);
}

void xzec_consortium_reward_contract::get_reward_param(const common::xlogic_time_t current_time,
    common::xlogic_time_t& activation_time,
    xreward_cons_property_param_t& property_param,
    data::system_contract::xissue_detail_v2& issue_detail)
{
    // get time
    std::string activation_str;
    activation_str = STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
    XCONTRACT_ENSURE(activation_str.size() != 0, "STRING GET XPORPERTY_CONTRACT_GENESIS_STAGE_KEY empty");

    data::system_contract::xactivation_record record;
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)activation_str.c_str(), (uint32_t)activation_str.size());
    record.serialize_from(stream);
    activation_time = record.activation_time;
   

    // get map nodes
    std::map<std::string, std::string> map_nodes;
    auto const last_read_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT)));
    GET_MAP_PROPERTY(data::system_contract::XPORPERTY_CONTRACT_REG_KEY, map_nodes, last_read_height, sys_contract_rec_registration_addr);
    XCONTRACT_ENSURE(map_nodes.size() != 0, "MAP GET PROPERTY XPORPERTY_CONTRACT_REG_KEY empty");
    xdbg("[xzec_consortium_reward_contract::get_reward_param] last_read_height: %llu, map_nodes size: %d", last_read_height, map_nodes.size());
    for (auto const& entity : map_nodes) {
        auto const& account = entity.first;
        auto const& value_str = entity.second;
        data::system_contract::xreg_node_info node;
        xstream_t stream(xcontext_t::instance(), (uint8_t*)value_str.data(), value_str.size());
        node.serialize_from(stream);
        common::xaccount_address_t address { account };
        property_param.map_nodes[address] = node;
    }

    // get reward
    std::map<std::string, std::string> all_group_rewards;
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY, all_group_rewards);
    clear_workload();

    for (auto it = all_group_rewards.begin(); it != all_group_rewards.end(); it++) {
        auto const& key_str = it->first;
        common::xgroup_address_t group_address;
        xstream_t key_stream(xcontext_t::instance(), (uint8_t*)key_str.data(), key_str.size());
        key_stream >> group_address;
        auto const& value_str = it->second;
        data::system_contract::xgroup_cons_reward_t workload;

        xstream_t stream(xcontext_t::instance(), (uint8_t*)value_str.data(), value_str.size());
        workload.serialize_from(stream);

        property_param.map_node_reward_detail[group_address] = workload;
        xdbg("[xzec_consortium_reward_contract::get_reward_param]  property_param.auditor_workloads_detail workload:");
    }
    
    issue_detail.onchain_timer_round = current_time;
    issue_detail.m_zec_reward_contract_height = get_blockchain_height(sys_contract_zec_consortium_reward_addr);
    issue_detail.m_auditor_group_count = property_param.map_node_reward_detail.size();

    xdbg("[xzec_consortium_reward_contract::get_reward_param] auditor_group_count: %d", issue_detail.m_auditor_group_count);

    // get accumulated reward
    std::string value_str = STRING_GET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY);
    if (value_str.size() != 0) {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)value_str.c_str(), (uint32_t)value_str.size());
        property_param.accumulated_reward_record.serialize_from(stream);
    }
    xdbg("[xzec_consortium_reward_contract::get_reward_param] accumulated_reward_record: %lu, [%lu, %u]",
        property_param.accumulated_reward_record.last_issuance_time,
        static_cast<uint64_t>(property_param.accumulated_reward_record.issued_until_last_year_end / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(property_param.accumulated_reward_record.issued_until_last_year_end % data::system_contract::REWARD_PRECISION));
}

void xzec_consortium_reward_contract::calc_nodes_rewards(common::xlogic_time_t const current_time,
    common::xlogic_time_t const issue_time_length,
    xreward_cons_property_param_t& property_param,
    data::system_contract::xissue_detail_v2& issue_detail,
    std::map<common::xaccount_address_t, ::uint128_t>& node_reward_detail)
{

    // TODO: voter to zero workload account has no workload reward
    for (auto const& entity : property_param.map_nodes) {
        auto const& account = entity.first;
        auto const& node = entity.second;
        ::uint128_t self_reward = 0;
        calc_node_rewards(node, property_param.map_node_reward_detail, self_reward);
        if (self_reward > 0) {
            issue_detail.m_node_rewards[account.to_string()].m_self_reward = self_reward;
            // calc table reward
            xinfo("[node_reward_detail] acocunt: %s", account.to_string().c_str());
            node_reward_detail[account] = self_reward;
        }
    }
    XMETRICS_COUNTER_INCREMENT(CONS_REWARD_CONTRACT "calc_nodes_rewards_Executed", 1);
}

void xzec_consortium_reward_contract::calc_node_rewards(data::system_contract::xreg_node_info const& node,
    std::map<common::xgroup_address_t, data::system_contract::xgroup_cons_reward_t> node_reward_detail,
    ::uint128_t& reward_to_self)
{
    xdbg("[xzec_consortium_reward_contract::calc_auditor_worklaod_rewards] account: %s, node_reward_detail size %d \n",
        node.m_account.to_string().c_str(),
        node_reward_detail.size());

    reward_to_self = 0;
    for (auto& workload : node_reward_detail) {
        xdbg("[xzec_consortium_reward_contract::calc_auditor_worklaod_rewards] account: %s, group: %s, group size: %d\n",
            node.m_account.to_string().c_str(),
            workload.first.to_string().c_str(),
            workload.second.m_leader_reward.size());
        auto it = workload.second.m_leader_reward.find(node.m_account.to_string());
        if (it != workload.second.m_leader_reward.end()) {
            auto const& reward = it->second;
            reward_to_self += (reward * data::system_contract::REWARD_PRECISION);
            xdbg(
                "[xzec_consortium_reward_contract::calc_auditor_worklaod_rewards] account: %s, group: %s, reward: %d, new reward: %u \n",
                node.m_account.to_string().c_str(),
                workload.first.to_string().c_str(),
                reward,
                reward_to_self);
        }
    }
    return;
}

void xzec_consortium_reward_contract::calc_table_rewards(xreward_cons_property_param_t& property_param,
    std::map<common::xaccount_address_t, ::uint128_t> const& node_reward_detail,
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>>& table_node_reward_detail,
    std::map<common::xaccount_address_t, ::uint128_t>& table_total_rewards)
{

    for (auto reward : node_reward_detail) {
        xinfo("[xzec_consortium_reward_contract::calc_table_rewards] acocunt: %s", reward.first.to_string().c_str());
        common::xaccount_address_t table_address = calc_table_contract_address(common::xaccount_address_t { reward.first });
        if (table_address.empty()) {
            continue;
        }
        calc_table_node_reward_detail(table_address, reward.first, reward.second, table_total_rewards, table_node_reward_detail);
    }
}

void xzec_consortium_reward_contract::calc_table_node_reward_detail(common::xaccount_address_t const& table_address,
    common::xaccount_address_t const& account,
    ::uint128_t node_reward,
    std::map<common::xaccount_address_t, ::uint128_t>& table_total_rewards,
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>>& table_node_reward_detail)
{
    table_total_rewards[table_address] += node_reward;
    table_node_reward_detail[table_address][account] = node_reward;
    xdbg("[xzec_consortium_reward_contract::calc_table_node_reward_detail] node reward, pid:%d, reward_contract: %s, account: %s, reward: [%llu, %u]\n",
        getpid(),
        table_address.to_string().c_str(),
        account.to_string().c_str(),
        static_cast<uint64_t>(node_reward / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(node_reward % data::system_contract::REWARD_PRECISION));
}

void xzec_consortium_reward_contract::dispatch_all_reward_v3(const common::xlogic_time_t current_time,
    std::map<common::xaccount_address_t, ::uint128_t>& table_total_rewards,
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>>& table_node_reward_detail)
{
    XMETRICS_COUNTER_INCREMENT(CONS_REWARD_CONTRACT "dispatch_all_reward_Called", 1);
    XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "dispatch_all_reward");
    xdbg("[xzec_consortium_reward_contract::dispatch_all_reward] pid:%d", getpid());
    // dispatch table reward
    uint64_t issuance = 0;
    uint32_t task_id = get_task_id();
    for (auto const& entity : table_total_rewards) {
        auto const& contract = entity.first;
        auto const& total_award = entity.second;

        uint64_t reward = static_cast<uint64_t>(total_award);
        if (total_award % data::system_contract::REWARD_PRECISION != 0) {
            reward += 1;
        }
        issuance += reward;
        std::map<std::string, uint64_t> issuances;
        issuances.emplace(contract.to_string(), reward);
        base::xstream_t seo_stream(base::xcontext_t::instance());
        seo_stream << issuances;

        add_task(task_id, current_time, "", data::system_contract::XTRANSFER_ACTION, std::string((char*)seo_stream.data(), seo_stream.size()));
        task_id++;
    }
    xinfo("[xzec_consortium_reward_contract::dispatch_all_reward] actual issuance: %lu", issuance);

    // generate tasks
    const int task_limit = 1000;
    xinfo("[xzec_consortium_reward_contract::dispatch_all_reward] pid: %d, table_node_reward_detail size: %d\n", getpid(), table_node_reward_detail.size());
    for (auto& entity : table_node_reward_detail) {
        auto const& contract = entity.first;
        auto const& account_awards = entity.second;

        int count = 0;
        std::map<std::string, ::uint128_t> account_awards2;
        for (auto it = account_awards.begin(); it != account_awards.end(); it++) {
            account_awards2[it->first.to_string()] = it->second;
            if (++count % task_limit == 0) {
                base::xstream_t reward_stream(base::xcontext_t::instance());
                reward_stream << current_time;
                reward_stream << account_awards2;
                add_task(task_id,
                    current_time,
                    contract.to_string(),
                    data::system_contract::XREWARD_CLAIMING_ADD_NODE_REWARD,
                    std::string((char*)reward_stream.data(), reward_stream.size()));
                task_id++;

                account_awards2.clear();
            }
        }
        if (account_awards2.size() > 0) {
            base::xstream_t reward_stream(base::xcontext_t::instance());
            reward_stream << current_time;
            reward_stream << account_awards2;
            add_task(task_id,
                current_time,
                contract.to_string(),
                data::system_contract::XREWARD_CLAIMING_ADD_NODE_REWARD,
                std::string((char*)reward_stream.data(), reward_stream.size()));
            task_id++;
        }
    }

    XMETRICS_COUNTER_INCREMENT(CONS_REWARD_CONTRACT "dispatch_all_reward_Executed", 1);
    return;
}

void xzec_consortium_reward_contract::update_accumulated_issuance(uint64_t const timer_round)
{
    XMETRICS_COUNTER_INCREMENT(CONS_REWARD_CONTRACT "update_accumulated_issuance_Called", 1);
    auto current_year = (timer_round - get_activated_time()) / data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR + 1;

    uint64_t cur_year_issuances{0}, total_issuances{0};
    std::string cur_year_issuances_str = "", total_issuances_str = "";
    try {
        XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_GetExecutionTime");
        if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, base::xstring_utl::tostring(current_year))) {
            cur_year_issuances_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, base::xstring_utl::tostring(current_year));
        }
        if (MAP_FIELD_EXIST(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, "total")) {
            total_issuances_str = MAP_GET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, "total");
        }
    } catch (std::runtime_error & e) {
        xwarn("[xzec_reward_contract][update_accumulated_issuance] read accumulated issuances error:%s", e.what());
    }

    if (!cur_year_issuances_str.empty()) {
        cur_year_issuances = base::xstring_utl::touint64(cur_year_issuances_str);
    }

    cur_year_issuances = 0;
    total_issuances = 0;   

    {
        MAP_SET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, base::xstring_utl::tostring(current_year), base::xstring_utl::tostring(cur_year_issuances));
        MAP_SET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, "total", base::xstring_utl::tostring(total_issuances));
    }
    xkinfo("[xzec_consortium_reward_contract][update_accumulated_issuance] get stored accumulated issuance, year: %d, issuance: [%" PRIu64 ", total issuance: [%" PRIu64 ", timer round : %" PRIu64 "\n",
          current_year,
          cur_year_issuances,
          total_issuances,
          timer_round);
}

void xzec_consortium_reward_contract::update_accumulated_record(const data::system_contract::xaccumulated_reward_record& record)
{
    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);
    auto value_str = std::string((char*)stream.data(), stream.size());
    STRING_SET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY, value_str);

    return;
}

void xzec_consortium_reward_contract::update_issuance_detail(data::system_contract::xissue_detail_v2 const& issue_detail)
{
    xdbg("[xzec_consortium_reward_contract::update_issuance_detail] onchain_timer_round: %llu, m_zec_vote_contract_height: %llu, "
         "m_zec_workload_contract_height: %llu, m_zec_reward_contract_height: %llu, "
         "m_edge_reward_ratio: %u, m_archive_reward_ratio: %u "
         "m_validator_reward_ratio: %u, m_auditor_reward_ratio: %u, m_vote_reward_ratio: %u, m_governance_reward_ratio: %u",
        issue_detail.onchain_timer_round,
        issue_detail.m_zec_vote_contract_height,
        issue_detail.m_zec_workload_contract_height,
        issue_detail.m_zec_reward_contract_height,
        issue_detail.m_edge_reward_ratio,
        issue_detail.m_archive_reward_ratio,
        issue_detail.m_validator_reward_ratio,
        issue_detail.m_auditor_reward_ratio,
        issue_detail.m_vote_reward_ratio,
        issue_detail.m_governance_reward_ratio);
    std::string issue_detail_str = issue_detail.to_string();

    try {
        STRING_SET(data::system_contract::XPROPERTY_REWARD_DETAIL, issue_detail_str);
    } catch (std::runtime_error& e) {
        xdbg("[xzec_consortium_reward_contract::update_issuance_detail] STRING_SET XPROPERTY_REWARD_DETAIL error:%s", e.what());
    }
}

bool xzec_consortium_reward_contract::reward_expire_check(const uint64_t onchain_timer_round)
{
    uint64_t new_time_height = onchain_timer_round;

    auto get_activation_record = [&](data::system_contract::xactivation_record& record) {
        std::string value_str;

        value_str = STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
        if (!value_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)value_str.c_str(), (uint32_t)value_str.size());
            record.serialize_from(stream);
        }
        xdbg("[get_activation_record] activated: %d, pid:%d\n", record.activated, getpid());
        return record.activated;
    };

    data::system_contract::xactivation_record record;
    if (!get_activation_record(record)) {
        xinfo("[xzec_consortium_reward_contract::reward_is_expire] mainnet not activated, onchain_timer_round: %llu", onchain_timer_round);
        return false;
    }

    data::system_contract::xaccumulated_reward_record rew_record;
    get_accumulated_record(rew_record); // no need to check return value, rew_record has default value
    uint64_t old_time_height = record.activation_time + rew_record.last_issuance_time;
    auto reward_distribute_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_distribute_interval);
    xdbg("[xzec_consortium_reward_contract::reward_is_expire]  new_time_height %llu, old_time_height %llu, reward_distribute_interval: %u\n",
        new_time_height, old_time_height, reward_distribute_interval);
    if (new_time_height <= old_time_height || new_time_height - old_time_height < reward_distribute_interval) {
        return false;
    }

    xinfo("[xzec_consortium_reward_contract::reward_is_expire] will reward, new_time_height %llu, old_time_height %llu, reward_distribute_interval: %u ok\n",
        new_time_height, old_time_height, reward_distribute_interval);
    return true;
}

int xzec_consortium_reward_contract::get_accumulated_record(data::system_contract::xaccumulated_reward_record& record)
{
    std::string value_str = STRING_GET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY);
    xstream_t stream(xcontext_t::instance(), (uint8_t*)value_str.c_str(), (uint32_t)value_str.size());
    record.serialize_from(stream);

    return 0;
}

void xzec_consortium_reward_contract::update_table_height(const uint32_t table_id, const uint64_t cur_read_height)
{
    XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "set_property_contract_fulltableblock_height");
    MAP_SET(data::system_contract::XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY, std::to_string(table_id), xstring_utl::tostring(cur_read_height));
}

void xzec_consortium_reward_contract::clear_workload()
{
    XMETRICS_TIME_RECORD("zec_reward_clear_workload_all_time");

    {
        XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "XPORPERTY_CONTRACT_WORKLOAD_KEY_SetExecutionTime");
        CLEAR(enum_type_t::map, data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY);
    }
}

void xzec_consortium_reward_contract::execute_task()
{
    XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "execute_task_ExecutionTime");
    XMETRICS_CPU_TIME_RECORD(CONS_REWARD_CONTRACT "execute_task_cpu_time");
    std::map<std::string, std::string> dispatch_tasks;
    data::system_contract::xreward_dispatch_task task;

    {
        XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "XPORPERTY_CONTRACT_TASK_KEY_CopyGetExecutionTime");
        MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, dispatch_tasks);
    }

    xdbg("[xzec_consortium_reward_contract::execute_task] map size: %d\n", dispatch_tasks.size());
    XMETRICS_COUNTER_SET(CONS_REWARD_CONTRACT "currentTaskCnt", dispatch_tasks.size());

    const int32_t task_num_per_round = 16;
    for (auto i = 0; i < task_num_per_round; i++) {
        auto it = dispatch_tasks.begin();
        if (it == dispatch_tasks.end())
            return;

        xstream_t stream(xcontext_t::instance(), (uint8_t*)it->second.c_str(), (uint32_t)it->second.size());
        task.serialize_from(stream);

        XMETRICS_PACKET_INFO(CONS_REWARD_CONTRACT "executeTask",
            "id",
            it->first,
            "logicTime",
            task.onchain_timer_round,
            "targetContractAddr",
            task.contract,
            "action",
            task.action,
            "onChainParamTaskNumPerRound",
            task_num_per_round);

        // debug output
        if (task.action == data::system_contract::XREWARD_CLAIMING_ADD_NODE_REWARD) {
            xstream_t stream_params(xcontext_t::instance(), (uint8_t*)task.params.c_str(), (uint32_t)task.params.size());
            uint64_t onchain_timer_round;
            std::map<std::string, ::uint128_t> rewards;
            stream_params >> onchain_timer_round;
            stream_params >> rewards;
            for (auto const& r : rewards) {
                xinfo("[xzec_consortium_reward_contract::execute_task] contract: %s, action: %s, account: %s, reward: [%llu, %u], onchain_timer_round: %llu",
                    task.contract.c_str(),
                    task.action.c_str(),
                    r.first.c_str(),
                    static_cast<uint64_t>(r.second / data::system_contract::REWARD_PRECISION),
                    static_cast<uint32_t>(r.second % data::system_contract::REWARD_PRECISION),
                    task.onchain_timer_round);
            }
        } else if (task.action == data::system_contract::XTRANSFER_ACTION) {
            std::map<std::string, uint64_t> issuances;
            base::xstream_t seo_stream(base::xcontext_t::instance(), (uint8_t*)task.params.c_str(), (uint32_t)task.params.size());
            seo_stream >> issuances;
            for (auto const& issue : issuances) {
                xinfo("[xzec_consortium_reward_contract::execute_task] action: %s, contract account: %s, issuance: %llu, onchain_timer_round: %llu",
                    task.action.c_str(),
                    issue.first.c_str(),
                    issue.second,
                    task.onchain_timer_round);
                TRANSFER(issue.first, issue.second);
            }
        }

        if (task.action != data::system_contract::XTRANSFER_ACTION) {
            CALL(common::xaccount_address_t { task.contract }, task.action, task.params);
        }

        {
            XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "XPORPERTY_CONTRACT_TASK_KEY_RemoveExecutionTime");
            MAP_REMOVE(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, it->first);
        }

        dispatch_tasks.erase(it);
    }
}

void xzec_consortium_reward_contract::update_reg_contract_read_status(const uint64_t cur_time)
{
    bool update_rec_reg_contract_read_status { false };

    auto const last_read_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT)));
    auto const last_read_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME)));

    auto const height_step_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_height_step_limitation);
    auto const timeout_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_logic_timeout_limitation);

    uint64_t latest_height = get_blockchain_height(sys_contract_rec_registration_addr);
    xdbg("[xzec_reward_contract::update_reg_contract_read_status] cur_time: %llu, last_read_time: %llu, last_read_height: %llu, latest_height: %" PRIu64,
        cur_time,
        last_read_time,
        last_read_height,
        latest_height);
    XCONTRACT_ENSURE(latest_height >= last_read_height, u8"xzec_reward_contract::update_reg_contract_read_status latest_height < last_read_height");
    if (latest_height == last_read_height) {
        XMETRICS_PACKET_INFO(CONS_REWARD_CONTRACT "update_status", "next_read_height", last_read_height, "current_time", cur_time);
        STRING_SET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME, std::to_string(cur_time));
        return; 
    }
    // calc current_read_height:
    uint64_t next_read_height = last_read_height;
    update_rec_reg_contract_read_status = check_reg_contract_read_time(cur_time, last_read_time, last_read_height, latest_height, height_step_limitation, timeout_limitation, next_read_height);
    xinfo("[xzec_reward_contract::update_reg_contract_read_status] next_read_height: %" PRIu64 ", latest_height: %llu, update_rec_reg_contract_read_status: %d",
        next_read_height, latest_height, update_rec_reg_contract_read_status);

    if (update_rec_reg_contract_read_status) {
        base::xauto_ptr<xblock_t> block_ptr = get_block_by_height(sys_contract_rec_registration_addr, next_read_height);
        XCONTRACT_ENSURE(block_ptr != nullptr, "fail to get the rec_reg data");
        XMETRICS_PACKET_INFO(CONS_REWARD_CONTRACT "update_status", "next_read_height", next_read_height, "current_time", cur_time);
        STRING_SET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT, std::to_string(next_read_height));
        STRING_SET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME, std::to_string(cur_time));
    }
    return;
}

common::xaccount_address_t xzec_consortium_reward_contract::calc_table_contract_address(common::xaccount_address_t const& account)
{
    uint32_t table_id = 0;
    if (!EXTRACT_TABLE_ID(account, table_id)) {
        xwarn("[xzec_reward_contract::calc_table_contract_address] EXTRACT_TABLE_ID failed, node reward pid: %d, account: %s\n",
            getpid(),
            account.to_string().c_str());
        return {};
    }
    auto const& table_address = CALC_CONTRACT_ADDRESS(sys_contract_consortium_reward_claiming_addr, table_id);
    return common::xaccount_address_t { table_address };
}

uint32_t xzec_consortium_reward_contract::get_task_id()
{
    std::map<std::string, std::string> dispatch_tasks;

    {
        XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "XPORPERTY_CONTRACT_TASK_KEY_GetExecutionTime");
        MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, dispatch_tasks);
    }

    uint32_t task_id = 0;
    if (dispatch_tasks.size() > 0) {
        auto it = dispatch_tasks.end();
        it--;
        task_id = base::xstring_utl::touint32(it->first);
        task_id++;
    }
    return task_id;
}

void xzec_consortium_reward_contract::add_task(const uint32_t task_id,
    const uint64_t onchain_timer_round,
    const std::string& contract,
    const std::string& action,
    const std::string& params)
{
    data::system_contract::xreward_dispatch_task task;

    task.onchain_timer_round = onchain_timer_round;
    task.contract = contract;
    task.action = action;
    task.params = params;

    base::xstream_t stream(base::xcontext_t::instance());
    task.serialize_to(stream);
    std::stringstream ss;
    ss << std::setw(10) << std::setfill('0') << task_id;
    auto key = ss.str();
    {
        XMETRICS_TIME_RECORD(CONS_REWARD_CONTRACT "XPORPERTY_CONTRACT_TASK_KEY_SetExecutionTime");
        MAP_SET(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, key, std::string((char*)stream.data(), stream.size()));
    }
}

uint64_t xzec_consortium_reward_contract::get_activated_time() const
{
    data::system_contract::xactivation_record record;
    std::string value_str = STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)value_str.c_str(), (uint32_t)value_str.size());
        record.serialize_from(stream);
    }
    xdbg("[xzec_reward_contract::is_mainnet_activated] activated: %d, activation_time: %llu, pid:%d\n", record.activated, record.activation_time, getpid());
    return record.activation_time;
}

bool xzec_consortium_reward_contract::check_reg_contract_read_time(const uint64_t cur_time,
    const uint64_t last_read_time,
    const uint64_t last_read_height,
    const uint64_t latest_height,
    const uint64_t height_step_limitation,
    const common::xlogic_time_t timeout_limitation,
    uint64_t& next_read_height)
{
    if (latest_height < last_read_height)
        return false;

    if (latest_height - last_read_height >= height_step_limitation) {
        next_read_height = last_read_height + height_step_limitation;
        return true;
    } else if (cur_time - last_read_time > timeout_limitation) {
        next_read_height = latest_height;
        return true;
    }
    return false;
}

NS_END3

#undef CONS_REWARD_CONTRACT
#undef XCONTRACT_PREFIX
#undef XZEC_MODULE
