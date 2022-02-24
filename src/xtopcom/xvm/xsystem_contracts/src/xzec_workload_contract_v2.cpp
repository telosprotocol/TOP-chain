// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xworkload/xzec_workload_contract_v2.h"

#include "xconfig/xpredefined_configurations.h"
#include "xdata/xblock_statistics_data.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xnative_contract_address.h"
#include "xstake/xstake_algorithm.h"
#include "xvm/manager/xcontract_manager.h"

using top::base::xcontext_t;
using top::base::xstream_t;
using top::base::xstring_utl;
using top::base::xtime_utl;
using namespace top::xstake;
using namespace top::config;

#if !defined(XZEC_MODULE)
#    define XZEC_MODULE "sysContract_"
#endif

#define XCONTRACT_PREFIX "workload_v2_"

#define XWORKLOAD_CONTRACT XZEC_MODULE XCONTRACT_PREFIX

NS_BEG3(top, xvm, system_contracts)

xzec_workload_contract_v2::xzec_workload_contract_v2(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {
}

void xzec_workload_contract_v2::setup() {
    // key: common::xaccount_address_t(table), value: uint64(height)
    MAP_CREATE(XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY);
    MAP_CREATE(XPORPERTY_CONTRACT_WORKLOAD_KEY);
    MAP_CREATE(XPORPERTY_CONTRACT_VALIDATOR_WORKLOAD_KEY);
    STRING_CREATE(XPORPERTY_CONTRACT_TGAS_KEY);
}

bool xzec_workload_contract_v2::is_mainnet_activated() const {
    xactivation_record record;

    std::string value_str = STRING_GET2(xstake::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
    if (!value_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value_str.c_str(), (uint32_t)value_str.size());
        record.serialize_from(stream);
    }
    xdbg("[xzec_workload_contract_v2::is_mainnet_activated] activated: %d\n", record.activated);
    return static_cast<bool>(record.activated);
};


void xzec_workload_contract_v2::on_receive_workload(std::string const & table_info_str) {
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "on_receive_workload");
    XMETRICS_COUNTER_INCREMENT(XWORKLOAD_CONTRACT "on_receive_workload", 1);
    XCONTRACT_ENSURE(!table_info_str.empty(), "workload_str empty");


    auto const & source_address = SOURCE_ADDRESS();
    std::string base_addr;
    uint32_t table_id;
    if (!data::xdatautil::extract_parts(source_address, base_addr, table_id) || sys_contract_sharding_statistic_info_addr != base_addr) {
        xwarn("[xzec_workload_contract_v2::on_receive_workload] invalid call from %s", source_address.c_str());
        return;
    }
    xdbg("[xzec_workload_contract_v2::on_receive_workload] on_receive_workload call from %s, pid: %d, ", source_address.c_str(), getpid());

    std::string activation_str;
    std::map<std::string, std::string> workload_str;
    std::string tgas_str;
    std::string height_str;
    std::map<std::string, std::string> workload_str_new;
    std::string tgas_str_new;
    {
        XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "on_receive_workload_map_get");
        activation_str = STRING_GET2(XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, sys_contract_rec_registration_addr);
        MAP_COPY_GET(XPORPERTY_CONTRACT_WORKLOAD_KEY, workload_str);
        tgas_str = STRING_GET2(XPORPERTY_CONTRACT_TGAS_KEY);
        MAP_GET2(XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY, std::to_string(table_id), height_str);
    }

    handle_workload_str(activation_str, table_info_str, workload_str, tgas_str, height_str, workload_str_new, tgas_str_new);

    {
        XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "on_receive_workload_map_set");
        for (auto it = workload_str_new.cbegin(); it != workload_str_new.end(); it++) {
            MAP_SET(XPORPERTY_CONTRACT_WORKLOAD_KEY, it->first, it->second);
        }
        if (!tgas_str_new.empty() && tgas_str != tgas_str_new) {
            STRING_SET(XPORPERTY_CONTRACT_TGAS_KEY, tgas_str_new);
        }
    }
}

void xzec_workload_contract_v2::handle_workload_str(const std::string & activation_record_str,
                                                    const std::string & table_info_str,
                                                    const std::map<std::string, std::string> & workload_str,
                                                    const std::string & tgas_str,
                                                    const std::string & height_str,
                                                    std::map<std::string, std::string> & workload_str_new,
                                                    std::string & tgas_str_new) {
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "handle_workload_str");
    std::map<common::xgroup_address_t, xgroup_workload_t> group_workload;
    int64_t table_pledge_balance_change_tgas = 0;
    uint64_t height = 0;
    {
        xstream_t stream(xcontext_t::instance(), (uint8_t *)table_info_str.data(), table_info_str.size());
        MAP_OBJECT_DESERIALZE2(stream, group_workload);
        stream >> table_pledge_balance_change_tgas;
        stream >> height;
    }
    xinfo("[xzec_workload_contract::handle_workload_str] group_workload size: %zu, table_pledge_balance_change_tgas: %lld, height: %lu, last height: %lu\n",
          group_workload.size(),
          table_pledge_balance_change_tgas,
          height,
          xstring_utl::touint64(height_str));

    XCONTRACT_ENSURE(xstring_utl::touint64(height_str) < height, "zec_last_read_height >= table_previous_height");

    // update system total tgas
    int64_t tgas = 0;
    if (!tgas_str.empty()) {
        tgas = xstring_utl::toint64(tgas_str);
    }
    tgas_str_new = xstring_utl::tostring(tgas += table_pledge_balance_change_tgas);

    xactivation_record record;
    if (!activation_record_str.empty()) {
        base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)activation_record_str.c_str(), (uint32_t)activation_record_str.size());
        record.serialize_from(stream);
    }

    xdbg("[xzec_workload_contract_v2::is_mainnet_activated] activated: %d\n", record.activated);
    if (!record.activated) {
        return;
    }
    update_workload(group_workload, workload_str, workload_str_new);
}

std::vector<xobject_ptr_t<data::xblock_t>> xzec_workload_contract_v2::get_fullblock(common::xlogic_time_t const timestamp, const uint32_t table_id) {
    uint64_t last_read_height = get_table_height(table_id);
    uint64_t cur_read_height = last_read_height;
    // calc table address
    auto const & table_owner = common::xaccount_address_t{xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, table_id)};
    // get block
    std::vector<xobject_ptr_t<data::xblock_t>> res;
    uint64_t time_interval = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes{10}).count() / XGLOBAL_TIMER_INTERVAL_IN_SECONDS;
    while (true) {
        base::xauto_ptr<data::xblock_t> next_block = get_next_fullblock(table_owner.value(), cur_read_height);
        if (nullptr == next_block) {
            xdbg("[xzec_workload_contract_v2::get_fullblock] table %s next full block is empty, last read height: %lu", table_owner.value().c_str(), last_read_height);
            break;
        }
        auto block_height = next_block->get_height();
        XCONTRACT_ENSURE(next_block->is_fulltable(), "[xzec_workload_contract_v2::get_fullblock] next full tableblock is not full tableblock");
        // XCONTRACT_ENSURE(timestamp > next_block->get_clock(), "[xzec_workload_contract_v2::get_fullblock] time order error");
        if (timestamp < next_block->get_clock() || timestamp < (next_block->get_clock() + time_interval)) {
            xdbg("[xzec_workload_contract_v2::get_fullblock] clock interval not statisfy, timestamp: %lu, table: %s, full block height: %" PRIu64 ", block clock: %" PRIu64,
                 timestamp,
                 table_owner.value().c_str(),
                 block_height,
                 next_block->get_clock());
            break;
        } else {
            xdbg("[xzec_workload_contract_v2::get_fullblock] table: %s, block height: %" PRIu64 " push in", table_owner.value().c_str(), block_height);
            cur_read_height = block_height;
            res.push_back(std::move(next_block));
        }
    }
    // update table address height
    if (cur_read_height > last_read_height) {
        update_table_height(table_id, cur_read_height);
    }
    return res;
}

uint64_t xzec_workload_contract_v2::get_table_height(const uint32_t table_id) const {
    uint64_t last_read_height = 0;
    std::string value_str;
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "get_property_fulltableblock_height");

    std::string key = std::to_string(table_id);
    if (MAP_FIELD_EXIST(XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY, key)) {
        value_str = MAP_GET(XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY, key);
        XCONTRACT_ENSURE(!value_str.empty(), "read full tableblock height empty");
    }
    if (!value_str.empty()) {
        last_read_height = xstring_utl::touint64(value_str);
    }

    return last_read_height;
}

void xzec_workload_contract_v2::update_table_height(const uint32_t table_id, const uint64_t cur_read_height) {
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "set_property_contract_fulltableblock_height");
    MAP_SET(XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY, std::to_string(table_id), xstring_utl::tostring(cur_read_height));
}

void xzec_workload_contract_v2::update_tgas(int64_t table_pledge_balance_change_tgas) {
    std::string pledge_tgas_str;
    {
        XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "XPORPERTY_CONTRACT_TGAS_KEY_GetExecutionTime");
        pledge_tgas_str = STRING_GET2(XPORPERTY_CONTRACT_TGAS_KEY);
    }
    int64_t tgas = 0;
    if (!pledge_tgas_str.empty()) {
        tgas = xstring_utl::toint64(pledge_tgas_str);
    }
    tgas += table_pledge_balance_change_tgas;

    {
        XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "XPORPERTY_CONTRACT_TGAS_KEY_SetExecutionTime");
        STRING_SET(XPORPERTY_CONTRACT_TGAS_KEY, xstring_utl::tostring(tgas));
    }
}

void xzec_workload_contract_v2::accumulate_workload(xstatistics_data_t const & stat_data, std::map<common::xgroup_address_t, xgroup_workload_t> & group_workload) {
    auto node_service = contract::xcontract_manager_t::instance().get_node_service();
    auto workload_per_tableblock = XGET_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tableblock);
    auto workload_per_tx = XGET_ONCHAIN_GOVERNANCE_PARAMETER(workload_per_tx);
    for (auto const & static_item : stat_data.detail) {
        auto elect_statistic = static_item.second;
        for (auto const & group_item : elect_statistic.group_statistics_data) {
            common::xgroup_address_t const & group_addr = group_item.first;
            xgroup_related_statistics_data_t const & group_account_data = group_item.second;
            xvip2_t const & group_xvip2 = top::common::xip2_t{group_addr.network_id(),
                                                              group_addr.zone_id(),
                                                              group_addr.cluster_id(),
                                                              group_addr.group_id(),
                                                              (uint16_t)group_account_data.account_statistics_data.size(),
                                                              static_item.first};
            xdbg("[xzec_workload_contract_v2::accumulate_workload] group xvip2: %llu, %llu", group_xvip2.high_addr, group_xvip2.low_addr);
            for (size_t slotid = 0; slotid < group_account_data.account_statistics_data.size(); ++slotid) {
                auto account_str = node_service->get_group(group_xvip2)->get_node(slotid)->get_account();
                uint32_t block_count = group_account_data.account_statistics_data[slotid].block_data.block_count;
                uint32_t tx_count = group_account_data.account_statistics_data[slotid].block_data.transaction_count;
                uint32_t workload = block_count * workload_per_tableblock + tx_count * workload_per_tx;
                if (workload > 0) {
                    auto it2 = group_workload.find(group_addr);
                    if (it2 == group_workload.end()) {
                        xgroup_workload_t group_workload_info;
                        auto ret = group_workload.insert(std::make_pair(group_addr, group_workload_info));
                        XCONTRACT_ENSURE(ret.second, "insert workload failed");
                        it2 = ret.first;
                    }

                    it2->second.m_leader_count[account_str] += workload;
                }

                xdbg(
                    "[xzec_workload_contract_v2::accumulate_workload] group_addr: [%s, network_id: %u, zone_id: %u, cluster_id: %u, group_id: %u], leader: %s, workload: %u, "
                    "block_count: %u, tx_count: %u, workload_per_tableblock: %u, workload_per_tx: %u",
                    group_addr.to_string().c_str(),
                    group_addr.network_id().value(),
                    group_addr.zone_id().value(),
                    group_addr.cluster_id().value(),
                    group_addr.group_id().value(),
                    account_str.c_str(),
                    workload,
                    block_count,
                    tx_count,
                    workload_per_tableblock,
                    workload_per_tx);
            }
        }
    }
}

void xzec_workload_contract_v2::accumulate_workload_with_fullblock(common::xlogic_time_t const timestamp,
                                                                   const uint32_t start_table,
                                                                   const uint32_t end_table,
                                                                   std::map<common::xgroup_address_t, xgroup_workload_t> & group_workload) {
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "accumulate_total_time");
    XMETRICS_CPU_TIME_RECORD(XWORKLOAD_CONTRACT "accumulate_total_cpu_time");
    int64_t table_pledge_balance_change_tgas = 0;
    int64_t total_table_fullblock_num = 0;
    int64_t total_get_fullblock_time = 0;
    int64_t total_accumulate_workload_time = 0;

    for (auto i = start_table; i <= end_table; i++) {
        int64_t t1 = 0;
        int64_t t2 = 0;
        int64_t t3 = 0;
        t1 = xtime_utl::time_now_ms();
        // get table block
        auto const & full_blocks = get_fullblock(timestamp, i);
        t2 = xtime_utl::time_now_ms();
        // accumulate workload
        for (std::size_t block_index = 0; block_index < full_blocks.size(); block_index++) {
            xdbg("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] block_index: %u", block_index);
            xfull_tableblock_t * full_tableblock = dynamic_cast<xfull_tableblock_t *>(full_blocks[block_index].get());
            assert(full_tableblock != nullptr);
            auto const & stat_data = full_tableblock->get_table_statistics();
            accumulate_workload(stat_data, group_workload);
            // m_table_pledge_balance_change_tgas
            table_pledge_balance_change_tgas += full_tableblock->get_pledge_balance_change_tgas();
        }
        t3 = xtime_utl::time_now_ms();
        total_get_fullblock_time += t2 - t1;
        total_accumulate_workload_time += t3 - t2;
        total_table_fullblock_num += full_blocks.size();
        if (full_blocks.size() > 0) {
            xinfo("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] timer round: %" PRIu64 ", bucket index: %d, total_table_block_count: %" PRIu32 "\n",
                  timestamp,
                  i,
                  full_blocks.size());
        }
    }
    xinfo("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] timer round: %" PRIu64
          ", total_block_nums: %u, total_time: %ums, get_fullblock_time: %ums, accumulate_workload_time: %ums",
          timestamp,
          total_table_fullblock_num,
          total_get_fullblock_time + total_accumulate_workload_time,
          total_get_fullblock_time,
          total_accumulate_workload_time);
#if defined DEBUG
    for (auto & entity : group_workload) {
        auto const & group = entity.first;
        for (auto const & wl : entity.second.m_leader_count) {
            xdbg("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] workload group: %s, leader: %s, workload: %u",
                 group.to_string().c_str(),
                 wl.first.c_str(),
                 wl.second);
        }
        xdbg("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] workload group: %s, group id: %u, ends", group.to_string().c_str(), group.group_id().value());
    }
#endif
    update_tgas(table_pledge_balance_change_tgas);
    XMETRICS_COUNTER_SET(XWORKLOAD_CONTRACT "accumulate_total_time_counter", total_get_fullblock_time + total_accumulate_workload_time);
}

xgroup_workload_t xzec_workload_contract_v2::get_workload(common::xgroup_address_t const & group_address) {
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "get_workload_time");
    XMETRICS_CPU_TIME_RECORD(XWORKLOAD_CONTRACT "get_workload_cpu_time");
        std::string group_address_str;
        xstream_t stream(xcontext_t::instance());
        stream << group_address;
        group_address_str = std::string((const char *)stream.data(), stream.size());
        xgroup_workload_t total_workload;
        {
            int32_t ret = 0;
            std::string value_str;
            XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "XPORPERTY_CONTRACT_WORKLOAD_KEY_GetExecutionTime");
            ret = MAP_GET2(XPORPERTY_CONTRACT_WORKLOAD_KEY, group_address_str, value_str);
            if (ret) {
                xdbg("[xzec_workload_contract_v2::update_workload] group not exist: %s", group_address.to_string().c_str());
                total_workload.cluster_id = group_address_str;
            } else {
                xstream_t stream(xcontext_t::instance(), (uint8_t *)value_str.data(), value_str.size());
                total_workload.serialize_from(stream);
            }
        }
    return total_workload;
}

void xzec_workload_contract_v2::set_workload(common::xgroup_address_t const & group_address, xgroup_workload_t const & group_workload) {
    xstream_t key_stream(xcontext_t::instance());
    key_stream << group_address;
    std::string group_address_str = std::string((const char *)key_stream.data(), key_stream.size());
    xstream_t stream(xcontext_t::instance());
    group_workload.serialize_to(stream);
    std::string value_str = std::string((const char *)stream.data(), stream.size());
    MAP_SET(XPORPERTY_CONTRACT_WORKLOAD_KEY, group_address_str, value_str);
}

void xzec_workload_contract_v2::update_workload(const std::map<common::xgroup_address_t, xgroup_workload_t> & group_workload,
                                                const std::map<std::string, std::string> & workload_str,
                                                std::map<std::string, std::string> & workload_new) {
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "update_workload");
    for (auto const & one_group_workload : group_workload) {
        auto const & group_address = one_group_workload.first;
        auto const & workload = one_group_workload.second;
        // get
        std::string group_address_str;
        {
            xstream_t stream(xcontext_t::instance());
            stream << group_address;
            group_address_str = std::string((const char *)stream.data(), stream.size());
        }
        xgroup_workload_t total_workload;
        {
            auto it = workload_str.find(group_address_str);
            if (it == workload_str.end()) {
                xdbg("[xzec_workload_contract_v2::update_workload] group not exist: %s", group_address.to_string().c_str());
            } else {
                xstream_t stream(xcontext_t::instance(), (uint8_t *)it->second.data(), it->second.size());
                total_workload.serialize_from(stream);
            }
        }
        // update
        for (auto const & leader_workload : workload.m_leader_count) {
            auto const & leader = leader_workload.first;
            auto const & count = leader_workload.second;
            total_workload.m_leader_count[leader] += count;
            total_workload.cluster_total_workload += count;
            xdbg("[xzec_workload_contract_v2::update_workload] group: %u, leader: %s, count: %d, total_count: %d, total_workload: %d",
                 group_address.group_id().value(),
                 leader.c_str(),
                 count,
                 total_workload.m_leader_count[leader],
                 total_workload.cluster_total_workload);
        }
        // set
        {
            xstream_t stream(xcontext_t::instance());
            total_workload.serialize_to(stream);
            workload_new.insert(std::make_pair(group_address_str, std::string((const char *)stream.data(), stream.size())));
        }
    }
}

void xzec_workload_contract_v2::update_workload(std::map<common::xgroup_address_t, xgroup_workload_t> const & group_workload) {
    // auto t1 = xtime_utl::time_now_ms();
    for (auto const & one_group_workload : group_workload) {
        auto const & group_address = one_group_workload.first;
        auto const & workload = one_group_workload.second;
        // get
        auto total_workload = get_workload(group_address);
        // update
        for (auto const & leader_workload : workload.m_leader_count) {
            auto const & leader = leader_workload.first;
            auto const & count = leader_workload.second;
            total_workload.m_leader_count[leader] += count;
            total_workload.cluster_total_workload += count;
            xdbg("[xzec_workload_contract_v2::update_workload] group: %u, leader: %s, count: %d, total_count: %d, total_workload: %d",
                 group_address.group_id().value(),
                 leader.c_str(),
                 count,
                 total_workload.m_leader_count[leader],
                 total_workload.cluster_total_workload);
        }
        // set
        set_workload(group_address, total_workload);
    }
    // auto t2 = xtime_utl::time_now_ms();
    // XMETRICS_COUNTER_SET(XWORKLOAD_CONTRACT "update_workload_counter", t2 - t1);
}

void xzec_workload_contract_v2::upload_workload(common::xlogic_time_t const timestamp) {
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "upload_workload_time");
    XMETRICS_CPU_TIME_RECORD(XWORKLOAD_CONTRACT "upload_workload_cpu_time");
    std::string call_contract_str{};
    upload_workload_internal(timestamp, call_contract_str);
    if (!call_contract_str.empty()) {
        CALL(common::xaccount_address_t{sys_contract_zec_reward_addr}, "calculate_reward", call_contract_str);
    }
}

void xzec_workload_contract_v2::upload_workload_internal(common::xlogic_time_t const timestamp, std::string & call_contract_str) {
    std::map<std::string, std::string> group_workload_str;
    std::map<common::xgroup_address_t, xgroup_workload_t> group_workload_upload;

    MAP_COPY_GET(XPORPERTY_CONTRACT_WORKLOAD_KEY, group_workload_str);
    for (auto it = group_workload_str.begin(); it != group_workload_str.end(); it++) {
        xstream_t key_stream(xcontext_t::instance(), (uint8_t *)it->first.data(), it->first.size());
        common::xgroup_address_t group_address;
        key_stream >> group_address;
        xstream_t stream(xcontext_t::instance(), (uint8_t *)it->second.data(), it->second.size());
        xgroup_workload_t group_workload;
        group_workload.serialize_from(stream);

        for (auto const & leader_workload : group_workload.m_leader_count) {
            auto const & leader = leader_workload.first;
            auto const & workload = leader_workload.second;

            auto it2 = group_workload_upload.find(group_address);
            if (it2 == group_workload_upload.end()) {
                xgroup_workload_t empty_workload;
                auto ret = group_workload_upload.insert(std::make_pair(group_address, empty_workload));
                it2 = ret.first;
            }
            it2->second.m_leader_count[leader] += workload;
        }
    }
    if (group_workload_upload.size() > 0) {
        std::string group_workload_upload_str;
        {
            xstream_t stream(xcontext_t::instance());
            MAP_OBJECT_SERIALIZE2(stream, group_workload_upload);
            group_workload_upload_str = std::string((char *)stream.data(), stream.size());

            xinfo("[xzec_workload_contract_v2::upload_workload] report workload to zec reward, group_workload_upload size: %d, timer round: %" PRIu64,
                  group_workload_upload.size(),
                  timestamp);
        }
        {
            xstream_t stream(xcontext_t::instance());
            stream << timestamp;
            stream << group_workload_upload_str;
            call_contract_str = std::string((char *)stream.data(), stream.size());
            group_workload_upload.clear();
        }
    }
}

void xzec_workload_contract_v2::clear_workload() {
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "XPORPERTY_CONTRACT_WORKLOAD_KEY_SetExecutionTime");
    MAP_CLEAR(XPORPERTY_CONTRACT_WORKLOAD_KEY);
}

void xzec_workload_contract_v2::on_timer(common::xlogic_time_t const timestamp) {
    XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "on_timer");
    XMETRICS_CPU_TIME_RECORD(XWORKLOAD_CONTRACT "on_timer_cpu_time");
    upload_workload(timestamp);
    clear_workload();
}

NS_END3

#undef XWORKLOAD_CONTRACT
#undef XCONTRACT_PREFIX
#undef XZEC_MODULE
