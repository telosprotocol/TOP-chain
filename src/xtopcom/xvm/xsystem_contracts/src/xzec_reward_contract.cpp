// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xreward/xzec_reward_contract.h"

#include "xbase/xutl.h"
#include "xbasic/xuint.hpp"
#include "xbasic/xutility.h"
#include "xchain_fork/xutility.h"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xstore/xstore_error.h"

#include <iomanip>

#if !defined(XZEC_MODULE)
#    define XZEC_MODULE "sysContract_"
#endif

#define XCONTRACT_PREFIX "reward_"

#define XREWARD_CONTRACT XZEC_MODULE XCONTRACT_PREFIX

#define VALID_EDGER(node) (node.deposit() > 0)
#define VALID_AUDITOR(node) (node.deposit() > 0 && node.can_be_auditor())
#define VALID_VALIDATOR(node) (node.deposit() > 0)

enum { total_idx = 0, valid_idx, deposit_zero_num, num_type_idx_num } /*xreward_num_type_e*/;
enum { edger_idx = 0, archiver_idx, auditor_idx, validator_idx, evm_auditor_idx, evm_validator_idx, role_type_idx_num } /*xreward_role_type_e*/;

NS_BEG2(top, xstake)

xzec_reward_contract::xzec_reward_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {
}

void xzec_reward_contract::setup() {
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
    chain_data::xchain_data_processor_t::get_stake_map_property(
        common::xlegacy_account_address_t{SELF_ADDRESS()}, data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY, db_kv_103);
    for (auto const & _p : db_kv_103) {
        MAP_SET(data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY, _p.first, _p.second);
    }
    MAP_CREATE(data::system_contract::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY);
    std::vector<std::pair<std::string, std::string>> db_kv_125;
    chain_data::xchain_data_processor_t::get_stake_map_property(
        common::xlegacy_account_address_t{SELF_ADDRESS()}, data::system_contract::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY, db_kv_125);
    for (auto const & _p : db_kv_125) {
        MAP_SET(data::system_contract::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY, _p.first, _p.second);
    }
}

void xzec_reward_contract::on_timer(const common::xlogic_time_t onchain_timer_round) {
    std::string source_address = SOURCE_ADDRESS();
    if (SELF_ADDRESS().to_string() != source_address) {
        xwarn("[xzec_reward_contract::on_timer] invalid call from %s", source_address.c_str());
        return;
    }
    xinfo("[xzec_reward_contract::on_timer] on timer call, timer: %lu", onchain_timer_round);

    auto task_num = MAP_SIZE(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY);
    if (task_num > 0) {
        xinfo("[xzec_reward_contract::on_timer] left task: %d, execute tasks", task_num);
        execute_task();
    } else {
        if (reward_is_expire_v2(onchain_timer_round)) {
            xinfo("[xzec_reward_contract::on_timer] reward");
            reward(onchain_timer_round);
        } else {
            xinfo("[xzec_reward_contract::on_timer] update_reg_contract_read_status");
            update_reg_contract_read_status(onchain_timer_round);
        }
    }

    XMETRICS_COUNTER_INCREMENT(XREWARD_CONTRACT "on_timer_Executed", 1);
}

bool xzec_reward_contract::check_reg_contract_read_time(const uint64_t cur_time,
                                                        const uint64_t last_read_time,
                                                        const uint64_t last_read_height,
                                                        const uint64_t latest_height,
                                                        const uint64_t height_step_limitation,
                                                        const common::xlogic_time_t timeout_limitation,
                                                        uint64_t & next_read_height) {
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

void xzec_reward_contract::update_reg_contract_read_status(const uint64_t cur_time) {
    bool update_rec_reg_contract_read_status{false};

    auto const last_read_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT)));
    auto const last_read_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME)));

    auto const height_step_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_height_step_limitation);
    auto const timeout_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_logic_timeout_limitation);

    uint64_t latest_height = get_blockchain_height(sys_contract_rec_registration_addr);
    xinfo("[xzec_reward_contract::update_reg_contract_read_status] cur_time: %llu, last_read_time: %llu, last_read_height: %llu, latest_height: %" PRIu64,
          cur_time,
          last_read_time,
          last_read_height,
          latest_height);
    XCONTRACT_ENSURE(latest_height >= last_read_height, "xzec_reward_contract::update_reg_contract_read_status latest_height < last_read_height");
    if (latest_height == last_read_height) {
        XMETRICS_PACKET_INFO(XREWARD_CONTRACT "update_status", "next_read_height", last_read_height, "current_time", cur_time);
        STRING_SET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME, std::to_string(cur_time));
        xinfo("[xzec_reward_contract::update_reg_contract_read_status] last_read_height equal latest_height, exit");
        return;
    }
    // calc current_read_height:
    uint64_t next_read_height = last_read_height;
    update_rec_reg_contract_read_status =
        check_reg_contract_read_time(cur_time, last_read_time, last_read_height, latest_height, height_step_limitation, timeout_limitation, next_read_height);
    xinfo("[xzec_reward_contract::update_reg_contract_read_status] next_read_height: %" PRIu64 ", latest_height: %llu, update_rec_reg_contract_read_status: %d",
          next_read_height,
          latest_height,
          update_rec_reg_contract_read_status);

    if (update_rec_reg_contract_read_status) {
        base::xauto_ptr<data::xblock_t> block_ptr = get_block_by_height(sys_contract_rec_registration_addr, next_read_height);
        XCONTRACT_ENSURE(block_ptr != nullptr, "fail to get the rec_reg data");
        XMETRICS_PACKET_INFO(XREWARD_CONTRACT "update_status", "next_read_height", next_read_height, "current_time", cur_time);
        STRING_SET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT, std::to_string(next_read_height));
        STRING_SET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME, std::to_string(cur_time));
        xinfo("[xzec_reward_contract::update_reg_contract_read_status] update property, %s->%lu, %s, %lu",
              data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT,
              next_read_height,
              data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_LOGIC_TIME,
              cur_time);
    }
    return;
}

void xzec_reward_contract::calculate_reward(common::xlogic_time_t current_time, std::string const & workload_str) {
    std::string source_address = SOURCE_ADDRESS();
    xinfo("[xzec_reward_contract::calculate_reward] called from address: %s", source_address.c_str());
    if (sys_contract_zec_workload_addr != source_address) {
        xwarn("[xzec_reward_contract::calculate_reward] from invalid address: %s", source_address.c_str());
        return;
    }
    on_receive_workload(workload_str);
}

void xzec_reward_contract::reward(const common::xlogic_time_t current_time) {
    xinfo("[xzec_reward_contract::reward] start reward");
    // step1 get related params
    common::xlogic_time_t activation_time;                 // system activation time
    xreward_onchain_param_t onchain_param;                 // onchain params
    xreward_property_param_t property_param;               // property from self and other contracts
    data::system_contract::xissue_detail_v2 issue_detail;  // issue details this round
    get_reward_param(current_time, activation_time, onchain_param, property_param, issue_detail);
    XCONTRACT_ENSURE(current_time > activation_time, "current_time <= activation_time");
    // step2 calculate node and table rewards
    std::map<common::xaccount_address_t, ::uint128_t> node_reward_detail;    // <node, self reward>
    std::map<common::xaccount_address_t, ::uint128_t> node_dividend_detail;  // <node, dividend reward>
    ::uint128_t community_reward;                                            // community reward
    calc_nodes_rewards_v5(current_time, current_time - activation_time, onchain_param, property_param, issue_detail, node_reward_detail, node_dividend_detail, community_reward);
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>> table_nodes_rewards;  // <table, <node, reward>>
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>> table_vote_rewards;   // <table, <node be voted, reward>>
    std::map<common::xaccount_address_t, ::uint128_t> contract_rewards;                                           // <table, total reward>
    calc_table_rewards(property_param, node_reward_detail, node_dividend_detail, table_nodes_rewards, table_vote_rewards, contract_rewards);
    // step3 dispatch rewards
    uint64_t actual_issuance;
    dispatch_all_reward_v3(current_time, contract_rewards, table_nodes_rewards, table_vote_rewards, community_reward, actual_issuance);
    // step4 update property
    update_property(current_time, actual_issuance, property_param.accumulated_reward_record, issue_detail);
}

bool xzec_reward_contract::reward_is_expire_v2(const uint64_t onchain_timer_round) {
    auto get_activation_record = [&](data::system_contract::xactivation_record & record) {
        std::string value_str;

        value_str = STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
        if (!value_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
            record.serialize_from(stream);
        }
        xinfo("[get_activation_record] activated: %d", record.activated);
        return record.activated;
    };

    data::system_contract::xactivation_record record;
    if (!get_activation_record(record)) {
        xinfo("[xzec_reward_contract::reward_is_expire] mainnet not activated, onchain_timer_round: %llu", onchain_timer_round);
        return false;
    }

    data::system_contract::xaccumulated_reward_record rew_record;
    get_accumulated_record(rew_record);  // no need to check return value, rew_record has default value
    auto old_time_height = record.activation_time + rew_record.last_issuance_time;
    auto reward_distribute_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_distribute_interval);
    xinfo("[xzec_reward_contract::reward_is_expire] new_time_height %llu, old_time_height %llu, reward_distribute_interval: %u",
          onchain_timer_round,
          old_time_height,
          reward_distribute_interval);
    if (onchain_timer_round <= old_time_height || onchain_timer_round - old_time_height < reward_distribute_interval) {
        xinfo("[xzec_reward_contract::reward_is_expire] not reward");
        return false;
    }

    xinfo("[xzec_reward_contract::reward_is_expire] will reward");
    return true;
}

uint32_t xzec_reward_contract::get_task_id() {
    std::map<std::string, std::string> dispatch_tasks;

    {
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

void xzec_reward_contract::add_task(const uint32_t task_id,
                                    const uint64_t onchain_timer_round,
                                    const std::string & contract,
                                    const std::string & action,
                                    const std::string & params) {
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
        MAP_SET(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, key, std::string((char *)stream.data(), stream.size()));
    }
}

void xzec_reward_contract::execute_task() {
    std::map<std::string, std::string> dispatch_tasks;
    data::system_contract::xreward_dispatch_task task;

    {
        MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, dispatch_tasks);
    }

    xinfo("[xzec_reward_contract::execute_task] map size: %d", dispatch_tasks.size());
    XMETRICS_COUNTER_SET(XREWARD_CONTRACT "currentTaskCnt", dispatch_tasks.size());

    const int32_t task_num_per_round = 16;
    for (auto i = 0; i < task_num_per_round; i++) {
        auto it = dispatch_tasks.begin();
        if (it == dispatch_tasks.end())
            return;

        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)it->second.c_str(), (uint32_t)it->second.size());
        task.serialize_from(stream);

        XMETRICS_PACKET_INFO(XREWARD_CONTRACT "executeTask",
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
        if (task.action == data::system_contract::XREWARD_CLAIMING_ADD_NODE_REWARD || task.action == data::system_contract::XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD) {
            base::xstream_t stream_params(base::xcontext_t::instance(), (uint8_t *)task.params.c_str(), (uint32_t)task.params.size());
            uint64_t onchain_timer_round;
            std::map<std::string, ::uint128_t> rewards;
            stream_params >> onchain_timer_round;
            stream_params >> rewards;
            for (auto const & r : rewards) {
                xinfo("[xzec_reward_contract::execute_task] call task, contract: %s, action: %s, account: %s, reward: [%llu, %u], onchain_timer_round: %llu",
                      task.contract.c_str(),
                      task.action.c_str(),
                      r.first.c_str(),
                      static_cast<uint64_t>(r.second / data::system_contract::REWARD_PRECISION),
                      static_cast<uint32_t>(r.second % data::system_contract::REWARD_PRECISION),
                      task.onchain_timer_round);
            }
        } else if (task.action == data::system_contract::XTRANSFER_ACTION) {
            std::map<std::string, uint64_t> issuances;
            base::xstream_t seo_stream(base::xcontext_t::instance(), (uint8_t *)task.params.c_str(), (uint32_t)task.params.size());
            seo_stream >> issuances;
            for (auto const & issue : issuances) {
                xinfo("[xzec_reward_contract::execute_task] transfer task, action: %s, contract account: %s, issuance: %llu, onchain_timer_round: %llu",
                      task.action.c_str(),
                      issue.first.c_str(),
                      issue.second,
                      task.onchain_timer_round);
                TRANSFER(issue.first, issue.second);
            }
        }

        if (task.action != data::system_contract::XTRANSFER_ACTION) {
            CALL(common::xaccount_address_t{task.contract}, task.action, task.params);
        }

        {
            MAP_REMOVE(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, it->first);
        }

        dispatch_tasks.erase(it);
    }
}

void xzec_reward_contract::print_tasks() {
#if defined(DEBUG)
    std::map<std::string, std::string> dispatch_tasks;
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_TASK_KEY, dispatch_tasks);

    data::system_contract::xreward_dispatch_task task;
    for (auto const & p : dispatch_tasks) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)p.second.c_str(), (uint32_t)p.second.size());
        task.serialize_from(stream);

        xdbg("[xzec_reward_contract::print_tasks] task id: %s, onchain_timer_round: %llu, contract: %s, action: %s",
             p.first.c_str(),
             task.onchain_timer_round,
             task.contract.c_str(),
             task.action.c_str());

        if (task.action == data::system_contract::XREWARD_CLAIMING_ADD_NODE_REWARD || task.action == data::system_contract::XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD) {
            base::xstream_t stream_params(base::xcontext_t::instance(), (uint8_t *)task.params.c_str(), (uint32_t)task.params.size());
            uint64_t onchain_timer_round;
            std::map<std::string, ::uint128_t> rewards;
            stream_params >> onchain_timer_round;
            stream_params >> rewards;
            for (auto const & r : rewards) {
                xdbg("[xzec_reward_contract::print_tasks] account: %s, reward: [%llu, %u]\n",
                     r.first.c_str(),
                     static_cast<uint64_t>(r.second / data::system_contract::REWARD_PRECISION),
                     static_cast<uint32_t>(r.second % data::system_contract::REWARD_PRECISION));
            }
        } else if (task.action == data::system_contract::XTRANSFER_ACTION) {
            std::map<std::string, uint64_t> issuances;
            base::xstream_t seo_stream(base::xcontext_t::instance(), (uint8_t *)task.params.c_str(), (uint32_t)task.params.size());
            seo_stream >> issuances;
            for (auto const & issue : issuances) {
                xdbg("[xzec_reward_contract::print_tasks] contract account: %s, issuance: %llu\n", issue.first.c_str(), issue.second);
            }
        }
    }
#endif
}

void xzec_reward_contract::update_accumulated_issuance(uint64_t const issuance, uint64_t const timer_round) {
    XMETRICS_COUNTER_INCREMENT(XREWARD_CONTRACT "update_accumulated_issuance_Called", 1);
    auto current_year = (timer_round - get_activated_time()) / data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR + 1;
    xinfo("[xzec_reward_contract::update_accumulated_issuance] time: %lu, current_year: %lu, this_time_issuance: %lu", timer_round, current_year, issuance);

    uint64_t cur_year_issuances{0};
    uint64_t total_issuances{0};
    std::string cur_year_issuances_str;
    std::string total_issuances_str;
    try {
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
    xinfo("[xzec_reward_contract::update_accumulated_issuance] cur_year_issuances %lu->%lu", cur_year_issuances, cur_year_issuances + issuance);
    cur_year_issuances += issuance;
    if (!total_issuances_str.empty()) {
        total_issuances = base::xstring_utl::touint64(total_issuances_str);
    }
    xinfo("[xzec_reward_contract::update_accumulated_issuance] total_issuances %lu->%lu", total_issuances, total_issuances + issuance);
    total_issuances += issuance;

    {
        MAP_SET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, base::xstring_utl::tostring(current_year), base::xstring_utl::tostring(cur_year_issuances));
        MAP_SET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, "total", base::xstring_utl::tostring(total_issuances));
        xinfo("[xzec_reward_contract::update_accumulated_issuance] set property: %s, year: , current_year: %lu -> cur_year_issuances: %lu, total -> total_issuances: %lu",
              data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE,
              current_year,
              cur_year_issuances,
              total_issuances);

        XMETRICS_PACKET_INFO(XREWARD_CONTRACT "issuance", "year", current_year, "issued", cur_year_issuances, "totalIssued", total_issuances);
        XMETRICS_COUNTER_INCREMENT(XREWARD_CONTRACT "update_accumulated_issuance_Executed", 1);
    }
}

uint64_t xzec_reward_contract::get_activated_time() const {
    data::system_contract::xactivation_record record;
    std::string value_str = STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        record.serialize_from(stream);
    }
    xinfo("[xzec_reward_contract::is_mainnet_activated] activated: %d, activation_time: %llu", record.activated, record.activation_time);
    return record.activation_time;
}

::uint128_t xzec_reward_contract::get_reserve_reward(::uint128_t issued_until_last_year_end, ::uint128_t minimum_issuance, uint32_t issuance_rate) {
    ::uint128_t reserve_reward = 0;
    ::uint128_t total_reserve = static_cast<::uint128_t>(data::system_contract::TOTAL_RESERVE) * data::system_contract::REWARD_PRECISION;
    if (total_reserve > issued_until_last_year_end) {
        reserve_reward = std::max((total_reserve - issued_until_last_year_end) * issuance_rate / 100, minimum_issuance);
    } else {
        reserve_reward = minimum_issuance;
    }
    return reserve_reward;
}

::uint128_t xzec_reward_contract::calc_issuance_internal(uint64_t const total_height,
                                                         uint64_t & last_issuance_time,
                                                         ::uint128_t const & minimum_issuance,
                                                         const uint32_t issuance_rate,
                                                         ::uint128_t & issued_until_last_year_end) {
    if (0 == total_height) {
        return 0;
    }

    ::uint128_t additional_issuance = 0;
    uint64_t issued_clocks = 0;  // from last issuance to last year end

    uint64_t call_duration_height = total_height - last_issuance_time;
    uint32_t current_year = total_height / data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR + 1;
    uint32_t last_issuance_year = last_issuance_time / data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR + 1;
    xdbg("[xzec_reward_contract::calc_issuance] last_issuance_time: %llu, current_year: %u, last_issuance_year:%u", last_issuance_time, current_year, last_issuance_year);
    while (last_issuance_year < current_year) {
        uint64_t remaining_clocks = data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR - last_issuance_time % data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR;
        if (remaining_clocks > 0) {
            auto reserve_reward = get_reserve_reward(issued_until_last_year_end, minimum_issuance, issuance_rate);
            additional_issuance += reserve_reward * remaining_clocks / data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR;
            xinfo(
                "[xzec_reward_contract::calc_issuance] cross year, last_issuance_year: %u, reserve_reward: [%llu, %u], remaining_clocks: %llu, issued_clocks: %u, "
                "additional_issuance: [%llu, %u], issued_until_last_year_end: [%llu, %u]",
                last_issuance_year,
                static_cast<uint64_t>(reserve_reward / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(reserve_reward % data::system_contract::REWARD_PRECISION),
                remaining_clocks,
                issued_clocks,
                static_cast<uint64_t>(additional_issuance / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(additional_issuance % data::system_contract::REWARD_PRECISION),
                static_cast<uint64_t>(issued_until_last_year_end / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(issued_until_last_year_end % data::system_contract::REWARD_PRECISION));
            issued_clocks += remaining_clocks;
            last_issuance_time += remaining_clocks;
            issued_until_last_year_end += reserve_reward;
        }
        last_issuance_year++;
    }

    ::uint128_t reserve_reward = 0;
    if (call_duration_height > issued_clocks) {
        reserve_reward = get_reserve_reward(issued_until_last_year_end, minimum_issuance, issuance_rate);
        additional_issuance += reserve_reward * (call_duration_height - issued_clocks) / data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR;
    }

    xinfo("[xzec_reward_contract::calc_issuance] additional_issuance: [%" PRIu64 ", %u], call_duration_height: %" PRId64 ", issued_clocks: %" PRId64 ", total_height: %" PRId64
          ", current_year: %" PRIu32 ", last_issuance_year: %" PRIu32
          ", reserve_reward: [%llu, %u], last_issuance_time: %llu, issued_until_last_year_end: [%llu, %u], TIMER_BLOCK_HEIGHT_PER_YEAR: %llu",
          static_cast<uint64_t>(additional_issuance / data::system_contract::REWARD_PRECISION),
          static_cast<uint32_t>(additional_issuance % data::system_contract::REWARD_PRECISION),
          call_duration_height,
          issued_clocks,
          total_height,
          current_year,
          last_issuance_year,
          static_cast<uint64_t>(reserve_reward / data::system_contract::REWARD_PRECISION),
          static_cast<uint32_t>(reserve_reward % data::system_contract::REWARD_PRECISION),
          last_issuance_time,
          static_cast<uint64_t>(issued_until_last_year_end / data::system_contract::REWARD_PRECISION),
          static_cast<uint32_t>(issued_until_last_year_end % data::system_contract::REWARD_PRECISION),
          data::system_contract::TIMER_BLOCK_HEIGHT_PER_YEAR);

    last_issuance_time = total_height;
    return additional_issuance;
}

int xzec_reward_contract::get_accumulated_record(data::system_contract::xaccumulated_reward_record & record) {
    std::string value_str = STRING_GET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY);
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
    record.serialize_from(stream);

    return 0;
}

void xzec_reward_contract::update_accumulated_record(const data::system_contract::xaccumulated_reward_record & record) {
    base::xstream_t stream(base::xcontext_t::instance());
    record.serialize_to(stream);
    auto value_str = std::string((char *)stream.data(), stream.size());
    STRING_SET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY, value_str);

    return;
}

int xzec_reward_contract::get_node_info(const std::map<std::string, data::system_contract::xreg_node_info> & map_nodes,
                                        const std::string & account,
                                        data::system_contract::xreg_node_info & node) {
    auto it = map_nodes.find(account);
    if (it == map_nodes.end()) {
        return -1;
    }
    node = it->second;
    return 0;
}

void xzec_reward_contract::on_receive_workload(std::string const & workload_str) {
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)workload_str.data(), workload_str.size());
    std::map<common::xgroup_address_t, data::system_contract::xgroup_workload_t> workload_info;

    MAP_OBJECT_DESERIALZE2(stream, workload_info);
    xinfo("[xzec_reward_contract::on_receive_workload] SOURCE_ADDRESS: %s, workload_info size: %zu\n", SOURCE_ADDRESS().c_str(), workload_info.size());
    for (auto const & workload : workload_info) {
        base::xstream_t stream(base::xcontext_t::instance());
        stream << workload.first;
        auto const & group_address_str = std::string((const char *)stream.data(), stream.size());
        auto const & workload_info = workload.second;
        if (common::has<common::xnode_type_t::consensus_auditor>(workload.first.type()) || common::has<common::xnode_type_t::evm_auditor>(workload.first.type())) {
            add_cluster_workload(true, group_address_str, workload_info.m_leader_count);
        } else if (common::has<common::xnode_type_t::consensus_validator>(workload.first.type()) || common::has<common::xnode_type_t::evm_validator>(workload.first.type())) {
            add_cluster_workload(false, group_address_str, workload_info.m_leader_count);
        } else {
            // invalid group
            xwarn("[xzec_workload_contract_v2::accumulate_workload] invalid group id: %d", workload.first.group_id().value());
            continue;
        }
    }

    XMETRICS_COUNTER_INCREMENT(XREWARD_CONTRACT "on_receive_workload_Executed", 1);
}

void xzec_reward_contract::add_cluster_workload(bool auditor, std::string const & group_address_str, std::map<std::string, uint32_t> const & leader_count) {
    const char * property;
    if (auditor) {
        property = data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY;
    } else {
        property = data::system_contract::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY;
    }
    common::xgroup_address_t group_address;
    {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)group_address_str.data(), group_address_str.size());
        stream >> group_address;
        xinfo("[xzec_reward_contract::add_cluster_workload] auditor: %d, group: %s, group size: %d", auditor, group_address.to_string().c_str(), leader_count.size());
    }

    data::system_contract::xgroup_workload_t workload;
    std::string value_str;
    int32_t ret;
    if (auditor) {
        ret = MAP_GET2(property, group_address_str, value_str);
    } else {
        ret = MAP_GET2(property, group_address_str, value_str);
    }

    if (ret) {
        workload.group_address_str = group_address_str;
    } else {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
        workload.serialize_from(stream);
    }

    for (auto const & leader_count_info : leader_count) {
        auto const & leader = leader_count_info.first;
        auto const & work = leader_count_info.second;

        workload.m_leader_count[leader] += work;
        workload.group_total_workload += work;
        xdbg("[xzec_reward_contract::add_cluster_workload] group: %s, leader: %s, work: %u, leader total workload: %u, group total workload: %d\n",
             group_address.to_string().c_str(),
             leader_count_info.first.c_str(),
             work,
             workload.m_leader_count[leader],
             workload.group_total_workload);
    }

    base::xstream_t stream(base::xcontext_t::instance());
    workload.serialize_to(stream);
    std::string value = std::string((const char *)stream.data(), stream.size());
    if (auditor) {
        MAP_SET(property, group_address_str, value);
    } else {
        MAP_SET(property, group_address_str, value);
    }
}

void xzec_reward_contract::clear_workload() {

    {
        CLEAR(enum_type_t::map, data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY);
    }
    {
        CLEAR(enum_type_t::map, data::system_contract::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY);
    }
}

void xzec_reward_contract::update_issuance_detail(data::system_contract::xissue_detail_v2 const & issue_detail) {
    xinfo(
        "[xzec_reward_contract::update_issuance_detail] onchain_timer_round: %llu, m_zec_vote_contract_height: %llu, "
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
    std::string issue_detail_str;
    issue_detail_str = issue_detail.to_string();
    try {
        STRING_SET(data::system_contract::XPROPERTY_REWARD_DETAIL, issue_detail_str);
    } catch (std::runtime_error & e) {
        xwarn("[xzec_reward_contract::update_issuance_detail] STRING_SET XPROPERTY_REWARD_DETAIL error:%s", e.what());
    }
}

void xzec_reward_contract::get_reward_param(const common::xlogic_time_t current_time,
                                            common::xlogic_time_t & activation_time,
                                            xreward_onchain_param_t & onchain_param,
                                            xreward_property_param_t & property_param,
                                            data::system_contract::xissue_detail_v2 & issue_detail) {
    // get time
    auto activation_str = STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
    XCONTRACT_ENSURE(!activation_str.empty(), "STRING GET XPORPERTY_CONTRACT_GENESIS_STAGE_KEY empty");

    data::system_contract::xactivation_record record;
    base::xstream_t stream(base::xcontext_t::instance(), reinterpret_cast<uint8_t *>(const_cast<char *>(activation_str.c_str())), static_cast<uint32_t>(activation_str.size()));
    record.serialize_from(stream);
    activation_time = record.activation_time;
    // get onchain param
    onchain_param.min_ratio_annual_total_reward = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_mining_annual_ratio);
    onchain_param.additional_issue_year_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(mining_annual_ratio_from_reserve_pool);
    onchain_param.edge_reward_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(edge_reward_ratio);
    onchain_param.archive_reward_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(archive_reward_ratio);
    onchain_param.validator_reward_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(validator_reward_ratio);
    onchain_param.auditor_reward_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_reward_ratio);
    onchain_param.vote_reward_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(vote_reward_ratio);
    onchain_param.governance_reward_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(governance_reward_ratio);
    onchain_param.auditor_group_zero_workload = XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_group_zero_workload);
    onchain_param.validator_group_zero_workload = XGET_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_zero_workload);

    onchain_param.evm_auditor_reward_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(evm_auditor_reward_ratio);
    onchain_param.evm_validator_reward_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(evm_validator_reward_ratio);
    onchain_param.evm_auditor_group_zero_workload = XGET_ONCHAIN_GOVERNANCE_PARAMETER(evm_auditor_group_zero_workload);
    onchain_param.evm_validator_group_zero_workload = XGET_ONCHAIN_GOVERNANCE_PARAMETER(evm_validator_group_zero_workload);

    auto total_ratio = onchain_param.edge_reward_ratio + onchain_param.archive_reward_ratio + onchain_param.validator_reward_ratio + onchain_param.auditor_reward_ratio +
                       onchain_param.vote_reward_ratio + onchain_param.governance_reward_ratio;
    xinfo(
        "[xzec_reward_contract::get_reward_param] onchain_timer_round: %u, min_ratio_annual_total_reward: %u, additional_issue_year_ratio: %u, edge_reward_ratio: %u, "
        "archive_reward_ratio: %u, validator_reward_ratio: %u, auditor_reward_ratio: %u, evm_validator_reward_ratio: %u, evm_auditor_reward_ratio: %u, vote_reward_ratio: %u, "
        "governance_reward_ratio: %u, auditor_group_zero_workload: %u, validator_group_zero_workload: %u, total_ratio: %u activation_time %" PRIu64,
        current_time,
        onchain_param.min_ratio_annual_total_reward,
        onchain_param.additional_issue_year_ratio,
        onchain_param.edge_reward_ratio,
        onchain_param.archive_reward_ratio,
        onchain_param.validator_reward_ratio,
        onchain_param.auditor_reward_ratio,
        onchain_param.evm_validator_reward_ratio,
        onchain_param.evm_auditor_reward_ratio,
        onchain_param.vote_reward_ratio,
        onchain_param.governance_reward_ratio,
        onchain_param.auditor_group_zero_workload,
        onchain_param.validator_group_zero_workload,
        total_ratio,
        static_cast<uint64_t>(activation_time));
    XCONTRACT_ENSURE(total_ratio == 100, "onchain reward total ratio not 100!");
    issue_detail.onchain_timer_round = current_time;
    issue_detail.m_zec_vote_contract_height = get_blockchain_height(sys_contract_zec_vote_addr);
    issue_detail.m_zec_workload_contract_height = get_blockchain_height(sys_contract_zec_workload_addr);
    issue_detail.m_zec_reward_contract_height = get_blockchain_height(sys_contract_zec_reward_addr);
    issue_detail.m_edge_reward_ratio = onchain_param.edge_reward_ratio;
    issue_detail.m_archive_reward_ratio = onchain_param.archive_reward_ratio;
    issue_detail.m_validator_reward_ratio = onchain_param.validator_reward_ratio;
    issue_detail.m_auditor_reward_ratio = onchain_param.auditor_reward_ratio;
    issue_detail.m_evm_validator_reward_ratio = onchain_param.evm_validator_reward_ratio;
    issue_detail.m_evm_auditor_reward_ratio = onchain_param.evm_auditor_reward_ratio;
    issue_detail.m_vote_reward_ratio = onchain_param.vote_reward_ratio;
    issue_detail.m_governance_reward_ratio = onchain_param.governance_reward_ratio;
    xinfo("[xzec_reward_contract::get_reward_param] zec_vote_contract_height: %u, zec_workload_contract_height: %u, zec_reward_contract_height: %u",
          issue_detail.m_zec_vote_contract_height,
          issue_detail.m_zec_workload_contract_height,
          issue_detail.m_zec_reward_contract_height);
    // get map nodes
    std::map<std::string, std::string> map_nodes;
    auto const last_read_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_LAST_READ_REC_REG_CONTRACT_BLOCK_HEIGHT)));
    GET_MAP_PROPERTY(data::system_contract::XPORPERTY_CONTRACT_REG_KEY, map_nodes, last_read_height, sys_contract_rec_registration_addr);
    XCONTRACT_ENSURE(map_nodes.size() != 0, "MAP GET PROPERTY XPORPERTY_CONTRACT_REG_KEY empty");
    xinfo("[xzec_reward_contract::get_reward_param] last_read_height: %llu, map_nodes size: %d", last_read_height, map_nodes.size());
    for (auto const & entity : map_nodes) {
        auto const & account = entity.first;
        auto const & value_str = entity.second;
        data::system_contract::xreg_node_info node;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
        node.serialize_from(stream);
        common::xaccount_address_t address{account};
        property_param.map_nodes[address] = node;
    }
    // get workload
    std::map<std::string, std::string> auditor_group_workloads;
    std::map<std::string, std::string> validator_group_workloads;
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_WORKLOAD_KEY, auditor_group_workloads);
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY, validator_group_workloads);
    clear_workload();
    for (auto it = auditor_group_workloads.begin(); it != auditor_group_workloads.end(); it++) {
        auto const & key_str = it->first;
        common::xgroup_address_t group_address;
        base::xstream_t key_stream(base::xcontext_t::instance(), (uint8_t *)key_str.data(), key_str.size());
        key_stream >> group_address;
        auto const & value_str = it->second;
        data::system_contract::xgroup_workload_t workload;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
        workload.serialize_from(stream);
        // if (common::has<common::xnode_type_t::consensus_auditor>(group_address.type())) {
        property_param.auditor_workloads_detail[group_address] = workload;
        // } else if (common::has<common::xnode_type_t::evm_auditor>(group_address.type())) {
        // property_param.evm_auditor_workloads_detail[group_address] = workload;
        // }
    }
    for (auto it = validator_group_workloads.begin(); it != validator_group_workloads.end(); it++) {
        auto const & key_str = it->first;
        common::xgroup_address_t group_address;
        base::xstream_t key_stream(base::xcontext_t::instance(), (uint8_t *)key_str.data(), key_str.size());
        key_stream >> group_address;
        auto const & value_str = it->second;
        data::system_contract::xgroup_workload_t workload;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
        workload.serialize_from(stream);
        // if (common::has<common::xnode_type_t::consensus_validator>(group_address.type())) {
        property_param.validator_workloads_detail[group_address] = workload;
        // } else if (common::has<common::xnode_type_t::evm_validator>(group_address.type())) {
        //     property_param.evm_validator_workloads_detail[group_address] = workload;
        // }
    }
    issue_detail.m_auditor_group_count = property_param.auditor_workloads_detail.size();
    issue_detail.m_validator_group_count = property_param.validator_workloads_detail.size();
    issue_detail.m_evm_auditor_group_count = property_param.evm_auditor_workloads_detail.size();
    issue_detail.m_evm_validator_group_count = property_param.evm_validator_workloads_detail.size();
    xinfo("[xzec_reward_contract::get_reward_param] auditor_group_count: %lu, validator_group_count: %lu, evm_auditor_group_count: %lu, evm_validator_group_count: %d",
          issue_detail.m_auditor_group_count,
          issue_detail.m_validator_group_count,
          issue_detail.m_evm_auditor_group_count,
          issue_detail.m_evm_validator_group_count);
    // get vote
    std::map<std::string, std::string> contract_auditor_votes;
    MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_TICKETS_KEY, contract_auditor_votes, sys_contract_zec_vote_addr);
    for (auto & contract_auditor_vote : contract_auditor_votes) {
        auto const & contract = contract_auditor_vote.first;
        auto const & auditor_votes_str = contract_auditor_vote.second;
        std::map<std::string, std::string> auditor_votes;
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)auditor_votes_str.data(), auditor_votes_str.size());
        stream >> auditor_votes;
        common::xaccount_address_t address{contract};
        std::map<common::xaccount_address_t, uint64_t> votes_detail;
        for (auto & votes : auditor_votes) {
            votes_detail.insert({common::xaccount_address_t{votes.first}, base::xstring_utl::touint64(votes.second)});
        }
        property_param.votes_detail[address] = votes_detail;
    }
    xinfo("[xzec_reward_contract::get_reward_param] votes_detail_count: %zu", property_param.votes_detail.size());
    // get accumulated reward
    std::string value_str = STRING_GET(data::system_contract::XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE_YEARLY);
    if (value_str.size() != 0) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        property_param.accumulated_reward_record.serialize_from(stream);
    }
    xinfo("[xzec_reward_contract::get_reward_param] accumulated_reward_record: %lu, [%lu, %u]",
          property_param.accumulated_reward_record.last_issuance_time,
          static_cast<uint64_t>(property_param.accumulated_reward_record.issued_until_last_year_end / data::system_contract::REWARD_PRECISION),
          static_cast<uint32_t>(property_param.accumulated_reward_record.issued_until_last_year_end % data::system_contract::REWARD_PRECISION));
}

/**
 * @brief calculate issuance
 *
 * @param issue_time_length currnet time - actived time
 * @param min_ratio_annual_total_reward onchain_parameter
 * @param additional_issue_year_ratio onchain_parameter
 * @param record accumulated reward record
 * @return total reward issuance
 */
::uint128_t xzec_reward_contract::calc_total_issuance(const common::xlogic_time_t issue_time_length,
                                                      const uint32_t min_ratio_annual_total_reward,
                                                      const uint32_t additional_issue_year_ratio,
                                                      data::system_contract::xaccumulated_reward_record & record) {
    auto const minimum_issuance = static_cast<::uint128_t>(TOTAL_ISSUANCE) * min_ratio_annual_total_reward / 100 * data::system_contract::REWARD_PRECISION;
    return calc_issuance_internal(issue_time_length, record.last_issuance_time, minimum_issuance, additional_issue_year_ratio, record.issued_until_last_year_end);
}

std::vector<std::vector<uint32_t>> xzec_reward_contract::calc_role_nums(std::map<common::xaccount_address_t, data::system_contract::xreg_node_info> const & map_nodes) {
    std::vector<std::vector<uint32_t>> role_nums;
    // vector init
    role_nums.resize(role_type_idx_num);
    for (uint i = 0; i < role_nums.size(); i++) {
        role_nums[i].resize(num_type_idx_num);
        for (uint j = 0; j < role_nums[i].size(); j++) {
            role_nums[i][j] = 0;
        }
    }

    // calc nums
    for (auto const & entity : map_nodes) {
        // auto const & account = entity.first;
        auto const & node = entity.second;

        // now statistical method
        if (node.could_be_edge()) {
            // total edger nums
            role_nums[edger_idx][total_idx]++;
            // valid edger nums
            if (VALID_EDGER(node)) {
                role_nums[edger_idx][valid_idx]++;
            }
            // deposit zero edger nums
            if (node.deposit() == 0) {
                role_nums[edger_idx][deposit_zero_num]++;
            }
        }
        if (node.could_be_auditor()) {
            // total auditor nums
            role_nums[auditor_idx][total_idx]++;
            // valid auditor nums
            if (VALID_AUDITOR(node)) {
                role_nums[auditor_idx][valid_idx]++;
            }
            // deposit zero auditor nums
            if (node.deposit() == 0) {
                role_nums[auditor_idx][deposit_zero_num]++;
            }
        }
        if (node.could_be_validator()) {
            // total validator nums
            role_nums[validator_idx][total_idx]++;
            // valid validator nums
            if (VALID_VALIDATOR(node)) {
                role_nums[validator_idx][valid_idx]++;
            }
            // deposit zero validator nums
            if (node.deposit() == 0) {
                role_nums[validator_idx][deposit_zero_num]++;
            }
        }
        if (node.could_be_evm_auditor()) {
            // total evm auditor nums
            role_nums[evm_auditor_idx][total_idx]++;
            // valid evm auditor nums
            if (node.deposit() > 0 && node.can_be_evm_auditor()) {
                role_nums[evm_auditor_idx][valid_idx]++;
            } else if (node.deposit() == 0) {
                role_nums[evm_auditor_idx][deposit_zero_num]++;
            }
        }
        if (node.could_be_evm_validator()) {
            // total evm validator nums
            role_nums[evm_validator_idx][total_idx]++;
            // valid evm validator nums
            if (node.deposit() > 0) {
                role_nums[evm_validator_idx][valid_idx]++;
            } else if (node.deposit() == 0) {
                role_nums[evm_validator_idx][deposit_zero_num]++;
            }
        }
    }

    return role_nums;
}

uint64_t xzec_reward_contract::calc_votes(std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, uint64_t>> const & votes_detail,
                                          std::map<common::xaccount_address_t, data::system_contract::xreg_node_info> & map_nodes,
                                          std::map<common::xaccount_address_t, uint64_t> & account_votes) {
    for (auto & entity : map_nodes) {
        auto const & account = entity.first;
        auto & node = entity.second;
        uint64_t node_total_votes = 0;
        for (auto const & vote_detail : votes_detail) {
            // auto const & contract = vote_detail.first;
            auto const & vote = vote_detail.second;
            auto iter = vote.find(account);
            if (iter != vote.end()) {
                account_votes[account] += iter->second;
                node_total_votes += iter->second;
            }
        }
        node.m_vote_amount = node_total_votes;
        xdbg("[xzec_reward_contract::calc_votes] map_nodes: account: %s, deposit: %llu, node_type: %s, votes: %llu",
             node.m_account.to_string().c_str(),
             node.deposit(),
             node.genesis() ? "advance,validator,edge" : common::to_string(node.miner_type()).c_str(),
             node.m_vote_amount);
    }
    // valid auditor only
    uint64_t total_votes = 0;
    for (auto const & entity : votes_detail) {
        auto const & vote = entity.second;

        for (auto const & entity2 : vote) {
            data::system_contract::xreg_node_info node;
            auto it = map_nodes.find(common::xaccount_address_t{entity2.first});
            if (it == map_nodes.end()) {
                xwarn("[xzec_reward_contract::calc_votes] account %s not in map_nodes", entity2.first.to_string().c_str());
                continue;
            } else {
                node = it->second;
            }

            if (node.deposit() > 0 && node.can_be_auditor()) {
                total_votes += entity2.second;
            }
        }
    }

    return total_votes;
}

std::map<common::xaccount_address_t, uint64_t> xzec_reward_contract::calc_votes(
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, uint64_t>> const & votes_detail,
    std::map<common::xaccount_address_t, data::system_contract::xreg_node_info> const & map_nodes) {
    std::map<common::xaccount_address_t, uint64_t> account_votes;
    for (auto & entity : map_nodes) {
        auto const & account = entity.first;
        // auto & node = entity.second;
        for (auto const & vote_detail : votes_detail) {
            // auto const & contract = vote_detail.first;
            auto const & vote = vote_detail.second;
            auto iter = vote.find(account);
            if (iter != vote.end()) {
                account_votes[account] += iter->second;
            }
        }
    }

    return account_votes;
}

::uint128_t xzec_reward_contract::calc_zero_workload_reward(std::map<common::xgroup_address_t, data::system_contract::xgroup_workload_t> & workloads_detail,
                                                            const uint32_t zero_workload,
                                                            const ::uint128_t group_reward,
                                                            std::vector<string> & zero_workload_account) {
    ::uint128_t zero_workload_rewards = 0;

    for (auto it = workloads_detail.begin(); it != workloads_detail.end();) {
        if (it->second.group_total_workload <= zero_workload) {
            xinfo(
                "[xzec_reward_contract::calc_zero_workload_reward] group id: %s, group size: %lu, group_total_workload: %u <= zero_workload_val %u and will "
                "be ignored\n",
                it->first.to_string().c_str(),
                it->second.m_leader_count.size(),
                it->second.group_total_workload,
                zero_workload);
            for (auto it2 = it->second.m_leader_count.begin(); it2 != it->second.m_leader_count.end(); it2++) {
                zero_workload_account.push_back(it2->first);
            }
            workloads_detail.erase(it++);
            zero_workload_rewards += group_reward;
        } else {
            it++;
        }
    }

    return zero_workload_rewards;
}

::uint128_t xzec_reward_contract::calc_invalid_workload_group_reward(bool is_auditor,
                                                                     std::map<common::xaccount_address_t, data::system_contract::xreg_node_info> const & map_nodes,
                                                                     const ::uint128_t group_reward,
                                                                     std::map<common::xgroup_address_t, data::system_contract::xgroup_workload_t> & workloads_detail) {
    ::uint128_t invalid_group_reward = 0;

    for (auto it = workloads_detail.begin(); it != workloads_detail.end();) {
        for (auto it2 = it->second.m_leader_count.begin(); it2 != it->second.m_leader_count.end();) {
            data::system_contract::xreg_node_info node;
            auto it3 = map_nodes.find(common::xaccount_address_t{it2->first});
            if (it3 == map_nodes.end()) {
                xinfo("[xzec_reward_contract::calc_invalid_workload_group_reward] account: %s not in map nodes", it2->first.c_str());
                it->second.group_total_workload -= it2->second;
                it->second.m_leader_count.erase(it2++);
                continue;
            } else {
                node = it3->second;
            }
            if (is_auditor) {
                if (node.deposit() == 0 || !node.can_be_auditor()) {
                    xinfo("[xzec_reward_contract::calc_invalid_workload_group_reward] account: %s is not a valid auditor, deposit: %llu, votes: %llu",
                          it2->first.c_str(),
                          node.deposit(),
                          node.m_vote_amount);
                    it->second.group_total_workload -= it2->second;
                    it->second.m_leader_count.erase(it2++);
                } else {
                    it2++;
                }
            } else {
                if (node.deposit() == 0 || !node.could_be_validator()) {
                    xinfo("[xzec_reward_contract::calc_invalid_workload_group_reward] account: %s is not a valid validator, deposit: %lu", it2->first.c_str(), node.deposit());
                    it->second.group_total_workload -= it2->second;
                    it->second.m_leader_count.erase(it2++);
                } else {
                    it2++;
                }
            }
        }
        if (it->second.m_leader_count.size() == 0) {
            xinfo("[xzec_reward_contract::calc_invalid_workload_group_reward] is auditor %d, group: %s, all node invalid, will be ignored",
                  is_auditor,
                  it->first.to_string().c_str());
            workloads_detail.erase(it++);
            invalid_group_reward += group_reward;
        } else {
            it++;
        }
    }

    return invalid_group_reward;
}

::uint128_t xzec_reward_contract::calc_evm_invalid_workload_group_reward(bool is_auditor,
                                                                         std::map<common::xaccount_address_t, data::system_contract::xreg_node_info> const & map_nodes,
                                                                         const ::uint128_t group_reward,
                                                                         std::map<common::xgroup_address_t, data::system_contract::xgroup_workload_t> & workloads_detail) {
    ::uint128_t invalid_group_reward = 0;

    for (auto it = workloads_detail.begin(); it != workloads_detail.end();) {
        for (auto it2 = it->second.m_leader_count.begin(); it2 != it->second.m_leader_count.end();) {
            data::system_contract::xreg_node_info node;
            auto it3 = map_nodes.find(common::xaccount_address_t{it2->first});
            if (it3 == map_nodes.end()) {
                xinfo("[xzec_reward_contract::calc_evm_invalid_workload_group_reward] account: %s not in map nodes", it2->first.c_str());
                it->second.group_total_workload -= it2->second;
                it->second.m_leader_count.erase(it2++);
                continue;
            } else {
                node = it3->second;
            }
            if (is_auditor) {
                if (node.deposit() == 0 || !node.can_be_evm_auditor()) {
                    xinfo("[xzec_reward_contract::calc_evm_invalid_workload_group_reward] account: %s is not a valid evm auditor, deposit: %llu, votes: %llu",
                          it2->first.c_str(),
                          node.deposit(),
                          node.m_vote_amount);
                    it->second.group_total_workload -= it2->second;
                    it->second.m_leader_count.erase(it2++);
                } else {
                    it2++;
                }
            } else {
                if (node.deposit() == 0 || !node.could_be_evm_validator()) {
                    xinfo("[xzec_reward_contract::calc_evm_invalid_workload_group_reward] account: %s is not a valid evm validator, deposit: %lu",
                          it2->first.c_str(),
                          node.deposit());
                    it->second.group_total_workload -= it2->second;
                    it->second.m_leader_count.erase(it2++);
                } else {
                    it2++;
                }
            }
        }
        if (it->second.m_leader_count.size() == 0) {
            xinfo("[xzec_reward_contract::calc_evm_invalid_workload_group_reward] eth group: %s, all node invalid, will be ignored", it->first.to_string().c_str());
            workloads_detail.erase(it++);
            invalid_group_reward += group_reward;
        } else {
            it++;
        }
    }

    return invalid_group_reward;
}

void xzec_reward_contract::calc_edge_workload_rewards(data::system_contract::xreg_node_info const & node,
                                                      std::vector<uint32_t> const & edge_num,
                                                      const ::uint128_t edge_workload_rewards,
                                                      ::uint128_t & reward_to_self) {
    XCONTRACT_ENSURE(edge_num.size() == num_type_idx_num, "edge_num not 3");
    auto divide_num = edge_num[valid_idx];
    reward_to_self = 0;
    if (0 == divide_num) {
        return;
    }
    if (!VALID_EDGER(node)) {
        return;
    }
    reward_to_self = edge_workload_rewards / divide_num;
    xdbg("[xzec_reward_contract::calc_edge_worklaod_rewards] account: %s, edge reward: [%llu, %u]",
         node.m_account.to_string().c_str(),
         static_cast<uint64_t>(reward_to_self / data::system_contract::REWARD_PRECISION),
         static_cast<uint32_t>(reward_to_self % data::system_contract::REWARD_PRECISION));

    return;
}

void xzec_reward_contract::calc_auditor_workload_rewards(data::system_contract::xreg_node_info const & node,
                                                         std::vector<uint32_t> const & auditor_num,
                                                         std::map<common::xgroup_address_t, data::system_contract::xgroup_workload_t> const & auditor_workloads_detail,
                                                         const ::uint128_t auditor_group_workload_rewards,
                                                         ::uint128_t & reward_to_self) {
    xdbg("[xzec_reward_contract::calc_auditor_worklaod_rewards] account: %s, %d group report workloads, group_total_rewards: [%llu, %u]\n",
         node.m_account.to_string().c_str(),
         auditor_workloads_detail.size(),
         static_cast<uint64_t>(auditor_group_workload_rewards / data::system_contract::REWARD_PRECISION),
         static_cast<uint32_t>(auditor_group_workload_rewards % data::system_contract::REWARD_PRECISION));
    XCONTRACT_ENSURE(auditor_num.size() == num_type_idx_num, "auditor_num array not 3");
    auto divide_num = auditor_num[valid_idx];
    reward_to_self = 0;
    if (0 == divide_num) {
        return;
    }
    if (!VALID_AUDITOR(node)) {
        return;
    }
    for (auto & workload : auditor_workloads_detail) {
        xdbg("[xzec_reward_contract::calc_auditor_worklaod_rewards] account: %s, group: %s, group size: %d, group_total_workload: %u\n",
             node.m_account.to_string().c_str(),
             workload.first.to_string().c_str(),
             workload.second.m_leader_count.size(),
             workload.second.group_total_workload);
        auto it = workload.second.m_leader_count.find(node.m_account.to_string());
        if (it != workload.second.m_leader_count.end()) {
            auto const & work = it->second;
            reward_to_self += auditor_group_workload_rewards * work / workload.second.group_total_workload;
            xdbg(
                "[xzec_reward_contract::calc_auditor_worklaod_rewards] account: %s, group: %s, work: %d, total_workload: %u, group_total_rewards: [%lu, %u], reward: [%lu, "
                "%u]",
                node.m_account.to_string().c_str(),
                workload.first.to_string().c_str(),
                work,
                workload.second.group_total_workload,
                static_cast<uint64_t>(auditor_group_workload_rewards / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(auditor_group_workload_rewards % data::system_contract::REWARD_PRECISION),
                static_cast<uint64_t>(reward_to_self / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(reward_to_self % data::system_contract::REWARD_PRECISION));
        }
    }

    return;
}

void xzec_reward_contract::calc_validator_workload_rewards(data::system_contract::xreg_node_info const & node,
                                                           std::vector<uint32_t> const & validator_num,
                                                           std::map<common::xgroup_address_t, data::system_contract::xgroup_workload_t> const & validator_workloads_detail,
                                                           const ::uint128_t validator_group_workload_rewards,
                                                           ::uint128_t & reward_to_self) {
    xdbg("[xzec_reward_contract::calc_validator_worklaod_rewards] account: %s, %d group report workloads, group_total_rewards: [%llu, %u]\n",
         node.m_account.to_string().c_str(),
         validator_workloads_detail.size(),
         static_cast<uint64_t>(validator_group_workload_rewards / data::system_contract::REWARD_PRECISION),
         static_cast<uint32_t>(validator_group_workload_rewards % data::system_contract::REWARD_PRECISION));
    XCONTRACT_ENSURE(validator_num.size() == num_type_idx_num, "validator_num array not 3");
    auto divide_num = validator_num[valid_idx];
    reward_to_self = 0;
    if (0 == divide_num) {
        return;
    }
    if (!VALID_VALIDATOR(node)) {
        return;
    }
    for (auto & workload : validator_workloads_detail) {
        xdbg("[xzec_reward_contract::calc_validator_worklaod_rewards] account: %s, group: %s, group size: %d, group_total_workload: %u\n",
             node.m_account.to_string().c_str(),
             workload.first.to_string().c_str(),
             workload.second.m_leader_count.size(),
             workload.second.group_total_workload);
        auto it = workload.second.m_leader_count.find(node.m_account.to_string());
        if (it != workload.second.m_leader_count.end()) {
            auto const & work = it->second;
            reward_to_self += validator_group_workload_rewards * work / workload.second.group_total_workload;
            xdbg(
                "[xzec_reward_contract::calc_validator_worklaod_rewards] account: %s, group: %s, work: %d, total_workload: %d, group_total_rewards: [%llu, %u], reward: "
                "[%llu, %u]\n",
                node.m_account.to_string().c_str(),
                workload.first.to_string().c_str(),
                work,
                workload.second.group_total_workload,
                static_cast<uint64_t>(validator_group_workload_rewards / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(validator_group_workload_rewards % data::system_contract::REWARD_PRECISION),
                static_cast<uint64_t>(reward_to_self / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(reward_to_self % data::system_contract::REWARD_PRECISION));
        }
    }

    return;
}

void xzec_reward_contract::calc_eth_auditor_workload_rewards(data::system_contract::xreg_node_info const & node,
                                                             std::vector<uint32_t> const & eth_num,
                                                             std::map<common::xgroup_address_t, data::system_contract::xgroup_workload_t> const & eth_workloads_detail,
                                                             const ::uint128_t eth_group_workload_rewards,
                                                             ::uint128_t & reward_to_self) {
    xdbg("[xzec_reward_contract::calc_eth_auditor_workload_rewards] account: %s, %d group report workloads, group_total_rewards: [%llu, %u]\n",
         node.m_account.to_string().c_str(),
         eth_workloads_detail.size(),
         static_cast<uint64_t>(eth_group_workload_rewards / data::system_contract::REWARD_PRECISION),
         static_cast<uint32_t>(eth_group_workload_rewards % data::system_contract::REWARD_PRECISION));
    XCONTRACT_ENSURE(eth_num.size() == num_type_idx_num, "array not 3");
    auto divide_num = eth_num[valid_idx];
    reward_to_self = 0;
    if (0 == divide_num) {
        return;
    }
    if (!node.can_be_evm_auditor()) {
        return;
    }
    for (auto & workload : eth_workloads_detail) {
        xdbg("[xzec_reward_contract::calc_eth_auditor_workload_rewards] account: %s, group id: %s, group size: %d, group_total_workload: %u\n",
             node.m_account.to_string().c_str(),
             workload.first.to_string().c_str(),
             workload.second.m_leader_count.size(),
             workload.second.group_total_workload);
        auto it = workload.second.m_leader_count.find(node.m_account.to_string());
        if (it != workload.second.m_leader_count.end()) {
            auto const & work = it->second;
            reward_to_self += eth_group_workload_rewards * work / workload.second.group_total_workload;
            xdbg(
                "[xzec_reward_contract::calc_eth_auditor_workload_rewards] account: %s, group id: %s, work: %d, total_workload: %d, group_total_workload: [%llu, %u], reward: "
                "[%llu, %u]\n",
                node.m_account.to_string().c_str(),
                workload.first.to_string().c_str(),
                work,
                workload.second.group_total_workload,
                static_cast<uint64_t>(eth_group_workload_rewards / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(eth_group_workload_rewards % data::system_contract::REWARD_PRECISION),
                static_cast<uint64_t>(reward_to_self / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(reward_to_self % data::system_contract::REWARD_PRECISION));
        }
    }

    return;
}

void xzec_reward_contract::calc_eth_validator_workload_rewards(data::system_contract::xreg_node_info const & node,
                                                               std::vector<uint32_t> const & eth_num,
                                                               std::map<common::xgroup_address_t, data::system_contract::xgroup_workload_t> const & eth_workloads_detail,
                                                               const ::uint128_t eth_group_workload_rewards,
                                                               ::uint128_t & reward_to_self) {
    xdbg("[xzec_reward_contract::calc_eth_validator_workload_rewards] account: %s, %d group report workloads, group_total_rewards: [%llu, %u]\n",
         node.m_account.to_string().c_str(),
         eth_workloads_detail.size(),
         static_cast<uint64_t>(eth_group_workload_rewards / data::system_contract::REWARD_PRECISION),
         static_cast<uint32_t>(eth_group_workload_rewards % data::system_contract::REWARD_PRECISION));
    XCONTRACT_ENSURE(eth_num.size() == num_type_idx_num, "array not 3");
    auto divide_num = eth_num[valid_idx];
    reward_to_self = 0;
    if (0 == divide_num) {
        return;
    }
    if (!node.can_be_evm_validator()) {
        return;
    }
    for (auto & workload : eth_workloads_detail) {
        xdbg("[xzec_reward_contract::calc_eth_validator_workload_rewards] account: %s, group id: %s, group size: %d, group_total_workload: %u\n",
             node.m_account.to_string().c_str(),
             workload.first.to_string().c_str(),
             workload.second.m_leader_count.size(),
             workload.second.group_total_workload);
        auto it = workload.second.m_leader_count.find(node.m_account.to_string());
        if (it != workload.second.m_leader_count.end()) {
            auto const & work = it->second;
            reward_to_self += eth_group_workload_rewards * work / workload.second.group_total_workload;
            xdbg(
                "[xzec_reward_contract::calc_eth_validator_workload_rewards] account: %s, group id: %s, work: %d, total_workload: %d, group_total_workload: [%llu, %u], reward: "
                "[%llu, %u]\n",
                node.m_account.to_string().c_str(),
                workload.first.to_string().c_str(),
                work,
                workload.second.group_total_workload,
                static_cast<uint64_t>(eth_group_workload_rewards / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(eth_group_workload_rewards % data::system_contract::REWARD_PRECISION),
                static_cast<uint64_t>(reward_to_self / data::system_contract::REWARD_PRECISION),
                static_cast<uint32_t>(reward_to_self % data::system_contract::REWARD_PRECISION));
        }
    }

    return;
}

void xzec_reward_contract::calc_vote_reward(data::system_contract::xreg_node_info const & node,
                                            const uint64_t auditor_total_votes,
                                            const ::uint128_t vote_rewards,
                                            ::uint128_t & reward_to_self) {
    reward_to_self = 0;
    if (node.can_be_auditor() && node.deposit() > 0) {
        XCONTRACT_ENSURE(auditor_total_votes > 0, "total_votes = 0 while valid auditor num > 0");
        reward_to_self = vote_rewards * node.m_vote_amount / auditor_total_votes;
        xdbg("[xzec_reward_contract::calc_nodes_rewards_v4] account: %s, node_vote_reward: [%llu, %u], node deposit: %llu, auditor_total_votes: %llu, node_votes: %llu",
             node.m_account.to_string().c_str(),
             static_cast<uint64_t>(reward_to_self / data::system_contract::REWARD_PRECISION),
             static_cast<uint32_t>(reward_to_self % data::system_contract::REWARD_PRECISION),
             node.deposit(),
             auditor_total_votes,
             node.m_vote_amount);
    }
    return;
}

void xzec_reward_contract::calc_table_node_dividend_detail(common::xaccount_address_t const & table_address,
                                                           common::xaccount_address_t const & account,
                                                           ::uint128_t const & reward,
                                                           uint64_t node_total_votes,
                                                           std::map<common::xaccount_address_t, uint64_t> const & vote_detail,
                                                           std::map<common::xaccount_address_t, ::uint128_t> & table_total_rewards,
                                                           std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>> & table_node_dividend_detail) {
    auto iter = vote_detail.find(account);
    if (iter != vote_detail.end()) {
        auto reward_to_voter = reward * iter->second / node_total_votes;
        xdbg(
            "[calc_table_node_dividend_detail] account: %s, contract: %s, table votes: %llu, adv_total_votes: %llu, adv_reward_to_voters: [%llu, %u], adv_reward_to_contract: "
            "[%llu, %u]\n",
            account.to_string().c_str(),
            iter->first.to_string().c_str(),
            iter->second,
            node_total_votes,
            static_cast<uint64_t>(reward / data::system_contract::REWARD_PRECISION),
            static_cast<uint32_t>(reward % data::system_contract::REWARD_PRECISION),
            static_cast<uint64_t>(reward_to_voter / data::system_contract::REWARD_PRECISION),
            static_cast<uint32_t>(reward_to_voter % data::system_contract::REWARD_PRECISION));
        if (reward_to_voter > 0) {
            table_total_rewards[table_address] += reward_to_voter;
            table_node_dividend_detail[table_address][account] += reward_to_voter;
        }
    }
}

void xzec_reward_contract::calc_table_node_reward_detail(common::xaccount_address_t const & table_address,
                                                         common::xaccount_address_t const & account,
                                                         ::uint128_t node_reward,
                                                         std::map<common::xaccount_address_t, ::uint128_t> & table_total_rewards,
                                                         std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>> & table_node_reward_detail) {
    table_total_rewards[table_address] += node_reward;
    table_node_reward_detail[table_address][account] = node_reward;
    xdbg("[xzec_reward_contract::calc_table_node_reward_detail] node reward, reward_contract: %s, account: %s, reward: [%llu, %u]\n",
         table_address.to_string().c_str(),
         account.to_string().c_str(),
         static_cast<uint64_t>(node_reward / data::system_contract::REWARD_PRECISION),
         static_cast<uint32_t>(node_reward % data::system_contract::REWARD_PRECISION));
}

common::xaccount_address_t xzec_reward_contract::calc_table_contract_address(common::xaccount_address_t const & account) {
    uint32_t table_id = 0;
    if (!EXTRACT_TABLE_ID(account, table_id)) {
        xwarn("[xzec_reward_contract::calc_table_contract_address] EXTRACT_TABLE_ID failed, node reward pid: %d, account: %s\n", getpid(), account.to_string().c_str());
        return {};
    }
    auto const & table_address = CALC_CONTRACT_ADDRESS(sys_contract_sharding_reward_claiming_addr, table_id);
    return common::xaccount_address_t{table_address};
}

void xzec_reward_contract::calc_nodes_rewards_v5(common::xlogic_time_t const current_time,
                                                 common::xlogic_time_t const issue_time_length,
                                                 xreward_onchain_param_t const & onchain_param,
                                                 xreward_property_param_t & property_param,
                                                 data::system_contract::xissue_detail_v2 & issue_detail,
                                                 std::map<common::xaccount_address_t, ::uint128_t> & node_reward_detail,
                                                 std::map<common::xaccount_address_t, ::uint128_t> & node_dividend_detail,
                                                 ::uint128_t & community_reward) {
    // step 1: calculate issuance
    auto const total_issuance =
        calc_total_issuance(issue_time_length, onchain_param.min_ratio_annual_total_reward, onchain_param.additional_issue_year_ratio, property_param.accumulated_reward_record);
    auto const limit_issuance =
        static_cast<::uint128_t>(data::system_contract::TOTAL_RESERVE) * data::system_contract::REWARD_PRECISION * onchain_param.additional_issue_year_ratio / 100;
    XCONTRACT_ENSURE(total_issuance > 0, "total issuance = 0");
    XCONTRACT_ENSURE(total_issuance < limit_issuance, "total over limit");
    auto edge_workload_rewards = total_issuance * onchain_param.edge_reward_ratio / 100;
    auto archive_workload_rewards = total_issuance * onchain_param.archive_reward_ratio / 100;
    auto total_auditor_total_workload_rewards = total_issuance * onchain_param.auditor_reward_ratio / 100;
    auto total_validator_total_workload_rewards = total_issuance * onchain_param.validator_reward_ratio / 100;
    // auto evm_auditor_total_workload_rewards = total_auditor_total_workload_rewards * onchain_param.evm_auditor_reward_ratio / 100;
    // auto evm_validator_total_workload_rewards = total_validator_total_workload_rewards * onchain_param.evm_validator_reward_ratio / 100;
    // auto auditor_total_workload_rewards = total_auditor_total_workload_rewards - evm_auditor_total_workload_rewards;
    // auto validator_total_workload_rewards = total_validator_total_workload_rewards - evm_validator_total_workload_rewards;
    auto auditor_total_workload_rewards = total_auditor_total_workload_rewards;
    auto validator_total_workload_rewards = total_validator_total_workload_rewards;
    auto vote_rewards = total_issuance * onchain_param.vote_reward_ratio / 100;
    auto governance_rewards = total_issuance * onchain_param.governance_reward_ratio / 100;
    community_reward = governance_rewards;
    auto auditor_group_count = property_param.auditor_workloads_detail.size();
    auto validator_group_count = property_param.validator_workloads_detail.size();
    // auto evm_auditor_group_count = property_param.evm_auditor_workloads_detail.size();
    // auto evm_validator_group_count = property_param.evm_validator_workloads_detail.size();
    ::uint128_t auditor_group_workload_rewards = 0;
    ::uint128_t validator_group_workload_rewards = 0;
    // ::uint128_t evm_auditor_group_workload_rewards = 0;
    // ::uint128_t evm_validator_group_workload_rewards = 0;
    if (auditor_group_count != 0) {
        auditor_group_workload_rewards = auditor_total_workload_rewards / auditor_group_count;
    }
    if (validator_group_count != 0) {
        validator_group_workload_rewards = validator_total_workload_rewards / validator_group_count;
    }
    // if (evm_auditor_group_count != 0) {
    //     evm_auditor_group_workload_rewards = evm_auditor_total_workload_rewards / evm_auditor_group_count;
    // }
    // if (evm_validator_group_count != 0) {
    //     evm_validator_group_workload_rewards = evm_validator_total_workload_rewards / evm_validator_group_count;
    // }

    // step 2: calculate different votes and role nums
    std::map<common::xaccount_address_t, uint64_t> account_votes;
    auto auditor_total_votes = calc_votes(property_param.votes_detail, property_param.map_nodes, account_votes);
    std::vector<std::vector<uint32_t>> role_nums = calc_role_nums(property_param.map_nodes);

    xinfo(
        "[xzec_reward_contract::calc_nodes_rewards] issue_time_length: %llu, "
        "total issuance: [%llu, %u], "
        "edge workload rewards: [%llu, %u], total edge num: %d, valid edge num: %d, "
        "archive workload rewards: [%llu, %u], total archive num: %d, valid archive num: %d, "
        "auditor workload rewards: [%llu, %u], auditor workload group num: %d, auditor group workload rewards: [%llu, %u], total auditor num: %d, valid auditor num: %d, "
        "validator workload rewards: [%llu, %u], validator workload group num: %d, validator group workload rewards: [%llu, %u], total validator num: %d, valid validator num: %d, "
        // "evm auditor workload rewards: [%llu, %u], evm auditor workload group num: %d, evm auditor group workload rewards: [%llu, %u], total evm auditor num: %d, valid evm
        // auditor num: %d,  " "evm validator workload rewards: [%llu, %u], evm validator workload group num: %d, evm validator group workload rewards: [%llu, %u], total evm
        // validator num: %d, valid evm validator num: %d,  "
        "vote rewards: [%llu, %u], "
        "governance rewards: [%llu, %u]",
        issue_time_length,
        static_cast<uint64_t>(total_issuance / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(total_issuance % data::system_contract::REWARD_PRECISION),
        static_cast<uint64_t>(edge_workload_rewards / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(edge_workload_rewards % data::system_contract::REWARD_PRECISION),
        role_nums[edger_idx][total_idx],
        role_nums[edger_idx][valid_idx],
        static_cast<uint64_t>(archive_workload_rewards / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(archive_workload_rewards % data::system_contract::REWARD_PRECISION),
        role_nums[archiver_idx][total_idx],
        role_nums[archiver_idx][valid_idx],
        static_cast<uint64_t>(auditor_total_workload_rewards / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(auditor_total_workload_rewards % data::system_contract::REWARD_PRECISION),
        auditor_group_count,
        static_cast<uint64_t>(auditor_group_workload_rewards / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(auditor_group_workload_rewards % data::system_contract::REWARD_PRECISION),
        role_nums[auditor_idx][total_idx],
        role_nums[auditor_idx][valid_idx],
        static_cast<uint64_t>(validator_total_workload_rewards / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(validator_total_workload_rewards % data::system_contract::REWARD_PRECISION),
        validator_group_count,
        static_cast<uint64_t>(validator_group_workload_rewards / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(validator_group_workload_rewards % data::system_contract::REWARD_PRECISION),
        role_nums[validator_idx][total_idx],
        role_nums[validator_idx][valid_idx],
        // static_cast<uint64_t>(evm_auditor_total_workload_rewards / data::system_contract::REWARD_PRECISION),
        // static_cast<uint32_t>(evm_auditor_total_workload_rewards % data::system_contract::REWARD_PRECISION),
        // evm_auditor_group_count,
        // static_cast<uint64_t>(evm_auditor_group_workload_rewards / data::system_contract::REWARD_PRECISION),
        // static_cast<uint32_t>(evm_auditor_group_workload_rewards % data::system_contract::REWARD_PRECISION),
        // role_nums[evm_auditor_idx][total_idx],
        // role_nums[evm_auditor_idx][valid_idx],
        // static_cast<uint64_t>(evm_validator_total_workload_rewards / data::system_contract::REWARD_PRECISION),
        // static_cast<uint32_t>(evm_validator_total_workload_rewards % data::system_contract::REWARD_PRECISION),
        // evm_validator_group_count,
        // static_cast<uint64_t>(evm_validator_group_workload_rewards / data::system_contract::REWARD_PRECISION),
        // static_cast<uint32_t>(evm_validator_group_workload_rewards % data::system_contract::REWARD_PRECISION),
        // role_nums[evm_validator_idx][total_idx],
        // role_nums[evm_validator_idx][valid_idx],
        static_cast<uint64_t>(vote_rewards / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(vote_rewards % data::system_contract::REWARD_PRECISION),
        static_cast<uint64_t>(governance_rewards / data::system_contract::REWARD_PRECISION),
        static_cast<uint32_t>(governance_rewards % data::system_contract::REWARD_PRECISION));
    // step 3: calculate reward
    if (0 == role_nums[edger_idx][valid_idx]) {
        community_reward += edge_workload_rewards;
    }
    community_reward += archive_workload_rewards;
    if (0 == role_nums[auditor_idx][valid_idx]) {
        community_reward += vote_rewards;
    }
    // 3.1 zero workload
    std::vector<string> zero_workload_account;
    // node which deposit = 0 whose rewards do not given to community yet
    if (auditor_group_workload_rewards != 0) {
        community_reward += calc_invalid_workload_group_reward(true, property_param.map_nodes, auditor_group_workload_rewards, property_param.auditor_workloads_detail);
        community_reward +=
            calc_zero_workload_reward(property_param.auditor_workloads_detail, onchain_param.auditor_group_zero_workload, auditor_group_workload_rewards, zero_workload_account);
    }
    if (validator_group_workload_rewards != 0) {
        community_reward += calc_invalid_workload_group_reward(false, property_param.map_nodes, validator_group_workload_rewards, property_param.validator_workloads_detail);
        community_reward += calc_zero_workload_reward(
            property_param.validator_workloads_detail, onchain_param.validator_group_zero_workload, validator_group_workload_rewards, zero_workload_account);
    }
    // if (evm_auditor_group_workload_rewards != 0) {
    //     community_reward += calc_evm_invalid_workload_group_reward(true, property_param.map_nodes, evm_auditor_group_workload_rewards,
    //     property_param.evm_auditor_workloads_detail); community_reward += calc_zero_workload_reward(
    //         property_param.evm_auditor_workloads_detail, onchain_param.evm_auditor_group_zero_workload, evm_auditor_group_workload_rewards, zero_workload_account);
    // }
    // if (evm_validator_group_workload_rewards != 0) {
    //     community_reward +=
    //         calc_evm_invalid_workload_group_reward(false, property_param.map_nodes, evm_validator_group_workload_rewards, property_param.evm_validator_workloads_detail);
    //     community_reward += calc_zero_workload_reward(
    //         property_param.evm_validator_workloads_detail, onchain_param.evm_validator_group_zero_workload, evm_validator_group_workload_rewards, zero_workload_account);
    // }

    // TODO: voter to zero workload account has no workload reward
    for (auto const & entity : property_param.map_nodes) {
        auto const & account = entity.first;
        auto const & node = entity.second;

        ::uint128_t self_reward = 0;
        ::uint128_t dividend_reward = 0;

        // 3.2 workload reward
        if (node.could_be_edge()) {
            ::uint128_t reward_to_self = 0;
            calc_edge_workload_rewards(node, role_nums[edger_idx], edge_workload_rewards, reward_to_self);
            if (reward_to_self != 0) {
                issue_detail.m_node_rewards[account.to_string()].m_edge_reward = reward_to_self;
                self_reward += reward_to_self;
            }
        }

        if (node.could_be_auditor()) {
            ::uint128_t reward_to_self = 0;
            calc_auditor_workload_rewards(node, role_nums[auditor_idx], property_param.auditor_workloads_detail, auditor_group_workload_rewards, reward_to_self);
            if (reward_to_self != 0) {
                issue_detail.m_node_rewards[account.to_string()].m_auditor_reward = reward_to_self;
                self_reward += reward_to_self;
            }
        }
        if (node.could_be_validator()) {
            ::uint128_t reward_to_self = 0;
            calc_validator_workload_rewards(node, role_nums[validator_idx], property_param.validator_workloads_detail, validator_group_workload_rewards, reward_to_self);
            if (reward_to_self != 0) {
                issue_detail.m_node_rewards[account.to_string()].m_validator_reward = reward_to_self;
                self_reward += reward_to_self;
            }
        }
        // if (node.can_be_evm_auditor()) {
        //     ::uint128_t reward_to_self = 0;
        //     calc_eth_auditor_workload_rewards(node, role_nums[evm_auditor_idx], property_param.evm_auditor_workloads_detail, evm_auditor_group_workload_rewards, reward_to_self);
        //     if (reward_to_self != 0) {
        //         issue_detail.m_node_rewards[account.to_string()].m_evm_auditor_reward = reward_to_self;
        //         self_reward += reward_to_self;
        //     }
        // }
        // if (node.can_be_evm_validator()) {
        //     ::uint128_t reward_to_self = 0;
        //     calc_eth_validator_workload_rewards(node, role_nums[evm_validator_idx], property_param.evm_validator_workloads_detail, evm_validator_group_workload_rewards,
        //     reward_to_self); if (reward_to_self != 0) {
        //         issue_detail.m_node_rewards[account.to_string()].m_evm_validator_reward = reward_to_self;
        //         self_reward += reward_to_self;
        //     }
        // }
        // 3.3 vote reward
        if (node.can_be_auditor() && node.deposit() > 0 && auditor_total_votes > 0) {
            ::uint128_t reward_to_self = 0;
            calc_vote_reward(node, auditor_total_votes, vote_rewards, reward_to_self);
            if (reward_to_self != 0) {
                issue_detail.m_node_rewards[account.to_string()].m_vote_reward = reward_to_self;
                self_reward += reward_to_self;
            }
        }
        // 3.4 dividend reward = (workload reward + vote reward) * ratio
        if (node.m_support_ratio_numerator > 0 && account_votes[account] > 0) {
            dividend_reward = self_reward * node.m_support_ratio_numerator / node.m_support_ratio_denominator;
            self_reward -= dividend_reward;
        }
        issue_detail.m_node_rewards[account.to_string()].m_self_reward = self_reward;
        // 3.5 calc table reward
        if (self_reward > 0) {
            node_reward_detail[account] = self_reward;
        }
        if (dividend_reward > 0) {
            node_dividend_detail[account] = dividend_reward;
        }
    }
    XMETRICS_COUNTER_INCREMENT(XREWARD_CONTRACT "calc_nodes_rewards_Executed", 1);
}

void xzec_reward_contract::calc_table_rewards(xreward_property_param_t & property_param,
                                              std::map<common::xaccount_address_t, ::uint128_t> const & node_reward_detail,
                                              std::map<common::xaccount_address_t, ::uint128_t> const & node_dividend_detail,
                                              std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>> & table_node_reward_detail,
                                              std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>> & table_node_dividend_detail,
                                              std::map<common::xaccount_address_t, ::uint128_t> & table_total_rewards) {
    std::map<common::xaccount_address_t, uint64_t> account_votes;
    calc_votes(property_param.votes_detail, property_param.map_nodes, account_votes);
    for (auto reward : node_reward_detail) {
        xinfo("[xzec_reward_contract::calc_table_rewards] acocunt: %s", reward.first.to_string().c_str());
        common::xaccount_address_t table_address = calc_table_contract_address(common::xaccount_address_t{reward.first});
        if (table_address.empty()) {
            continue;
        }
        calc_table_node_reward_detail(table_address, reward.first, reward.second, table_total_rewards, table_node_reward_detail);
    }
    for (auto reward : node_dividend_detail) {
        for (auto & vote_detail : property_param.votes_detail) {
            auto const & voter = vote_detail.first;
            auto const & votes = vote_detail.second;
            common::xaccount_address_t table_address = calc_table_contract_address(common::xaccount_address_t{voter});
            if (table_address.empty()) {
                continue;
            }
            calc_table_node_dividend_detail(table_address, reward.first, reward.second, account_votes[reward.first], votes, table_total_rewards, table_node_dividend_detail);
        }
    }
}

void xzec_reward_contract::dispatch_all_reward_v3(const common::xlogic_time_t current_time,
                                                  std::map<common::xaccount_address_t, ::uint128_t> & table_total_rewards,
                                                  std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>> & table_node_reward_detail,
                                                  std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, ::uint128_t>> & table_node_dividend_detail,
                                                  ::uint128_t & community_reward,
                                                  uint64_t & actual_issuance) {
    xinfo("[xzec_reward_contract::dispatch_all_reward_v3] dispatch_all_reward");
    // dispatch table reward
    uint64_t issuance = 0;
    uint32_t task_id = get_task_id();
    for (auto const & entity : table_total_rewards) {
        auto const & contract = entity.first;
        auto const & total_award = entity.second;

        uint64_t reward = static_cast<uint64_t>(total_award / data::system_contract::REWARD_PRECISION);
        if (total_award % data::system_contract::REWARD_PRECISION != 0) {
            reward += 1;
        }
        issuance += reward;
        std::map<std::string, uint64_t> issuances;
        issuances.emplace(contract.to_string(), reward);
        base::xstream_t seo_stream(base::xcontext_t::instance());
        seo_stream << issuances;

        add_task(task_id, current_time, "", data::system_contract::XTRANSFER_ACTION, std::string((char *)seo_stream.data(), seo_stream.size()));
        xinfo("[xzec_reward_contract::dispatch_all_reward] add transfer task to %s, %lu", contract.to_string().c_str(), reward);
        task_id++;
    }
    xinfo("[xzec_reward_contract::dispatch_all_reward] actual issuance: %lu", issuance);
    // dispatch community reward
    uint64_t common_funds = static_cast<uint64_t>(community_reward / data::system_contract::REWARD_PRECISION);
    if (common_funds > 0) {
        task_id = get_task_id();
        issuance += common_funds;
        std::map<std::string, uint64_t> issuances;
        issuances.emplace(sys_contract_rec_tcc_addr, common_funds);
        base::xstream_t seo_stream(base::xcontext_t::instance());
        seo_stream << issuances;

        add_task(task_id, current_time, "", data::system_contract::XTRANSFER_ACTION, std::string((char *)seo_stream.data(), seo_stream.size()));
        task_id++;
        xinfo("[xzec_reward_contract::dispatch_all_reward] common_funds: %lu", common_funds);
    }
    // generate tasks
    const int task_limit = 1000;
    xinfo("[xzec_reward_contract::dispatch_all_reward] table_node_reward_detail size: %zu", table_node_reward_detail.size());
    for (auto & entity : table_node_reward_detail) {
        auto const & contract = entity.first;
        auto const & account_awards = entity.second;

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
                         std::string((char *)reward_stream.data(), reward_stream.size()));
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
                     std::string((char *)reward_stream.data(), reward_stream.size()));
            task_id++;
        }
        xinfo("[xzec_reward_contract::dispatch_all_reward] add table_node_reward_detail task to %s", contract.to_string().c_str());
    }
    xinfo("[xzec_reward_contract::dispatch_all_reward] table_node_dividend_detail size: %zu", table_node_dividend_detail.size());
    for (auto const & entity : table_node_dividend_detail) {
        auto const & contract = entity.first;
        auto const & auditor_vote_rewards = entity.second;

        int count = 0;
        std::map<std::string, ::uint128_t> auditor_vote_rewards2;
        for (auto it = auditor_vote_rewards.begin(); it != auditor_vote_rewards.end(); it++) {
            auditor_vote_rewards2[it->first.to_string()] = it->second;
            if (++count % task_limit == 0) {
                base::xstream_t reward_stream(base::xcontext_t::instance());
                reward_stream << current_time;
                reward_stream << auditor_vote_rewards2;
                add_task(task_id,
                         current_time,
                         contract.to_string(),
                         data::system_contract::XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD,
                         std::string((char *)reward_stream.data(), reward_stream.size()));
                task_id++;

                auditor_vote_rewards2.clear();
            }
        }
        if (auditor_vote_rewards2.size() > 0) {
            base::xstream_t reward_stream(base::xcontext_t::instance());
            reward_stream << current_time;
            reward_stream << auditor_vote_rewards2;
            add_task(task_id,
                     current_time,
                     contract.to_string(),
                     data::system_contract::XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD,
                     std::string((char *)reward_stream.data(), reward_stream.size()));
            task_id++;
        }
        xinfo("[xzec_reward_contract::dispatch_all_reward] add table_node_dividend_detail task to %s", contract.to_string().c_str());
    }

    actual_issuance = issuance;
    print_tasks();

    XMETRICS_COUNTER_INCREMENT(XREWARD_CONTRACT "dispatch_all_reward_Executed", 1);
    return;
}

void xzec_reward_contract::update_property(const uint64_t current_time,
                                           const uint64_t actual_issuance,
                                           data::system_contract::xaccumulated_reward_record const & record,
                                           data::system_contract::xissue_detail_v2 const & issue_detail) {
    xinfo("[xzec_reward_contract::update_property] actual_issuance: %lu, current_time: %lu", actual_issuance, current_time);
    xinfo("[xzec_reward_contract::update_property] accumulated_reward_record: %lu, current_time: %lu, [%lu, %u]",
          record.last_issuance_time,
          static_cast<uint64_t>(record.issued_until_last_year_end / data::system_contract::REWARD_PRECISION),
          static_cast<uint32_t>(record.issued_until_last_year_end % data::system_contract::REWARD_PRECISION));
    update_accumulated_issuance(actual_issuance, current_time);
    update_accumulated_record(record);
    update_issuance_detail(issue_detail);
}

NS_END2

#undef XREWARD_CONTRACT
#undef XCONTRACT_PREFIX
#undef XZEC_MODULE
