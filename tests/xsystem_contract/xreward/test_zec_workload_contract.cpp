// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <gtest/gtest.h>

#include <string>

#define private public
#include "xbase/xobject_ptr.h"
#include "xblockstore/xblockstore_face.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xgenesis_data.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/xstore_face.h"
#include "xdata/xblocktool.h"
#include "xstore/xstore_face.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract.h"
#include "xvm/xsystem_contracts/xreward/xtable_vote_contract.h"
#include "xvm/xsystem_contracts/xreward/xzec_vote_contract.h"
#include "xvm/xsystem_contracts/xworkload/xzec_workload_contract_v2.h"
#include "xvm/xsystem_contracts/xreward/xzec_reward_contract.h"
#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"
#include "xchain_upgrade/xchain_upgrade_center.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"
#include <fstream>

using namespace top;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::xstake;
using namespace top::contract;

#define DB_PATH "db_test"

NS_BEG3(top, xvm, system_contracts)

class xtop_hash_t : public top::base::xhashplugin_t {
public:
    xtop_hash_t()
      : base::xhashplugin_t(-1)  //-1 = support every hash types
    {
    }

private:
    xtop_hash_t(const xtop_hash_t &) = delete;
    xtop_hash_t & operator=(const xtop_hash_t &) = delete;

public:
    virtual ~xtop_hash_t(){};
    virtual const std::string hash(const std::string & input, enum_xhash_type type) override {
        xassert(type == enum_xhash_type_sha2_256);
        auto hash = utl::xsha2_256_t::digest(input);
        return std::string(reinterpret_cast<char *>(hash.data()), hash.size());
    }
};

class xtop_test_workload_contract_v2 : public xzec_workload_contract_v2, public testing::Test {
    using xbase_t = xzec_reward_contract;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_workload_contract_v2);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_workload_contract_v2); 

    xtop_test_workload_contract_v2() : xzec_workload_contract_v2(common::xnetwork_id_t{0}) {
        if (access(DB_PATH, 0) == 0) {
            printf("db dir not exist\n");
            m_store = top::store::xstore_factory::create_store_with_kvdb(DB_PATH);
            base::xvchain_t::instance().set_xdbstore(m_store.get());
            m_blockstore.attach(store::get_vblockstore());
        }
    };

    xcontract_base * clone() override { return {}; }

    void exec(top::xvm::xvm_context * vm_ctx) { return; }

    xstatistics_data_t data;
    top::xobject_ptr_t<top::store::xstore_face_t> m_store;
    top::xobject_ptr_t<top::base::xvblockstore_t> m_blockstore;

    void tableblock_statistics_handle(const xvip2_t leader_xip, const uint32_t txs_count, const uint32_t blk_count, std::vector<base::xvoter> const & voter_info, xstatistics_data_t & data){
        // height
        uint64_t block_height = get_network_height_from_xip2(leader_xip);
        auto it_height = data.detail.find(block_height);
        if (it_height == data.detail.end()) {
            xelection_related_statistics_data_t election_related_data;
            std::pair<std::map<uint64_t, xelection_related_statistics_data_t>::iterator, bool> ret = data.detail.insert(std::make_pair(block_height, election_related_data));
            it_height = ret.first;
        }
        // gid
        uint8_t group_idx = uint8_t(get_group_id_from_xip2(leader_xip));
        common::xgroup_id_t group_id = common::xgroup_id_t{group_idx};
        common::xcluster_id_t cluster_id = common::xcluster_id_t{0};
        common::xzone_id_t zone_id = common::xzone_id_t{0};
        common::xnetwork_id_t network_id = common::xnetwork_id_t{0};
        common::xgroup_address_t group_addr(network_id, zone_id, cluster_id, group_id);
        auto it_group = it_height->second.group_statistics_data.find(group_addr);
        if (it_group == it_height->second.group_statistics_data.end()) {
            xgroup_related_statistics_data_t group_related_data;
            auto ret = it_height->second.group_statistics_data.insert(std::make_pair(group_addr, group_related_data));
            it_group = ret.first;
        }
        // nid
        uint16_t slot_idx = uint16_t(get_node_id_from_xip2(leader_xip));
        // common::xslot_id_t slot_id = common::xslot_id_t{slot_idx};
        if(it_group->second.account_statistics_data.size() < size_t(slot_idx+1)){
            it_group->second.account_statistics_data.resize(slot_idx+1);
        }
        // workload
        it_group->second.account_statistics_data[slot_idx].block_data.block_count += blk_count;
        it_group->second.account_statistics_data[slot_idx].block_data.transaction_count += txs_count;
        // vote
        uint32_t vote_count = 0;
        for (auto const & item : voter_info) {
            if (item.is_voted) {
                it_group->second.account_statistics_data[slot_idx].vote_data.vote_count++;
                vote_count++;
            }
        }
        printf("[tableblock_statistics] xip: [%lu, %lu], block_height: %lu, group_id: %u, slot_id: %u, "
            "work add block_count: %u, block_count: %u, add txs_count %u, transaction_count: %u, add vote count: %u, vote_count: %u\n",
                leader_xip.high_addr,
                leader_xip.low_addr,
                block_height,
                group_idx,
                slot_idx,
                1,
                it_group->second.account_statistics_data[slot_idx].block_data.block_count,
                txs_count, 
                it_group->second.account_statistics_data[slot_idx].block_data.transaction_count,
                vote_count,
                it_group->second.account_statistics_data[slot_idx].vote_data.vote_count);
    }

    void construct_data(){
        xvip2_t leader_xip_test[8];
        leader_xip_test[0] = {0x2080c04, 10}; // group id 3, node id 4, time 10
        leader_xip_test[1] = {0xa181c08, 20}; // group id 7, node id 8, time 10
        leader_xip_test[2] = {0xa181808, 20}; // group id 6, node id 8, time 20
        leader_xip_test[3] = {0x2080c00, 10}; // group id 3, node id 0, time 10
        leader_xip_test[4] = {0x2080c01, 10}; // group id 3, node id 1, time 10
        leader_xip_test[5] = {0x2080c02, 10}; // group id 3, node id 2, time 10
        leader_xip_test[6] = {0x2080c06, 10}; // group id 3, node id 6, time 10
        leader_xip_test[7] = {0x2080c00, 10}; // group id 3, node id 0, time 10

        uint32_t tx_count_test[8] = {100, 110, 120, 130, 140, 150, 160, 170};
        uint32_t blk_count_test[8] = {10, 11, 12, 13, 14, 15, 16, 17};

        std::vector<std::vector<base::xvoter>> voter_info_test;
        std::vector<base::xvoter> temp;
        for(int i = 0; i < 8; i++){
            base::xvoter vote;
            vote.is_voted = 0;
            temp.push_back(vote);
            vote.is_voted = 1;
            temp.push_back(vote);
            voter_info_test.push_back(temp);
        }

        for(size_t i = 0; i < 8; i++){
            tableblock_statistics_handle(leader_xip_test[i], tx_count_test[i], blk_count_test[i], voter_info_test[i], data);
        }
    }
    base::xauto_ptr<xblock_t> get_block_by_height(const std::string & owner, uint64_t height) const {
        // TODO(jimmy)
        base::xvaccount_t _vaddr(owner);
        base::xauto_ptr<base::xvblock_t> _block = m_blockstore->load_block_object(_vaddr, height, base::enum_xvblock_flag_committed, true);
        if (_block != nullptr) {
            _block->add_ref();
            return base::xauto_ptr<xblock_t>(dynamic_cast<data::xblock_t*>(_block.get()));
        }
        return base::xauto_ptr<xblock_t>(nullptr);
    }

    std::vector<xobject_ptr_t<data::xblock_t>> test_get_fullblock(common::xaccount_address_t const & table_owner, common::xlogic_time_t const timestamp) {
        // calc table address height
        uint64_t cur_read_height = 0;
        // uint64_t last_read_height = get_table_height(table_owner);
        uint64_t last_read_height = 0;
        // get block
        std::vector<xobject_ptr_t<data::xblock_t>> res;
        auto latest_block = m_blockstore->get_latest_committed_block(table_owner.value());
        data::xblock_t * block = dynamic_cast<data::xblock_t *>(latest_block.get());
        if (latest_block == nullptr) {
            std::cout << "account " << table_owner.value() << "latest committed block is null" << std::endl;
            return res;
        }
        auto cur_height = latest_block->get_height();
        // std::cout << table_owner.value() << " height " << cur_height << std::endl;
        auto time_interval = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes{5}).count() / XGLOBAL_TIMER_INTERVAL_IN_SECONDS;
        xobject_ptr_t<data::xblock_t> cur_tableblock = get_block_by_height(table_owner.value(), cur_height);
        while (cur_tableblock == nullptr) {
            if (cur_height > 0) {
                cur_tableblock = get_block_by_height(table_owner.value(), --cur_height);
            } else {
                xwarn("[xzec_workload_contract_v2::get_fullblock] no_block_is_available. table %s at time %" PRIu64, table_owner.c_str(), timestamp);
                return res;
            }
        }
        auto last_full_block_height = cur_tableblock->get_last_full_block_height();
        while (last_full_block_height != 0 && last_read_height < last_full_block_height) {
            xdbg("[xzec_workload_contract_v2::get_fullblock] last_full_block_height %lu", last_full_block_height);
            // get full block, assume that all full table blocks are in time order
            xobject_ptr_t<data::xblock_t> last_full_block = get_block_by_height(table_owner.value(), last_full_block_height);
            if (last_full_block == nullptr) {
                xwarn("[xzec_workload_contract_v2::get_fullblock] full block empty");
                break;
            }
            // XCONTRACT_ENSURE(last_full_block->is_fulltable(), "[xzec_workload_contract_v2::get_fullblock] full block check error");
            // check time interval
            // if (last_full_block_height + time_interval > 0) {
            if (0) {
                if (cur_read_height != 0) {
                    xwarn("[xzec_workload_contract_v2::get_fullblock] full table block may not in order. table %s at time %, " PRIu64 "front height %lu, rear height %lu",
                        table_owner.c_str(),
                        timestamp,
                        last_full_block_height,
                        cur_read_height);
                }
            } else {
                if (cur_read_height == 0) {
                    // cur read height first set
                    cur_read_height = last_full_block_height;
                }
                res.push_back(last_full_block);
                if (res.size() >= timestamp) {
                    // std::cout << "table " << ", res size: " <<  res.size() << std::endl;
                    return res;
                }
            }
            last_full_block_height = last_full_block->get_last_full_block_height();
            xdbg("[xzec_workload_contract_v2::get_fullblock] table %s last block height in cycle : " PRIu64, table_owner.c_str(), last_full_block_height);
        }

        // update table address height
        // if (cur_read_height > last_read_height) {
        //     update_table_height(table_owner, cur_read_height);
        // }
        xinfo("[xzec_workload_contract_v2::get_fullblock] table table_owner address: %s, last height: %lu, cur height: %lu\n", table_owner.c_str(), last_read_height, cur_read_height);

        return res;
    }

    void test_accumulate_workload(xstatistics_data_t const & stat_data, std::map<common::xgroup_address_t, xgroup_workload_t> & group_workload) {
        // auto node_service = contract::xcontract_manager_t::instance().get_node_service();
        auto workload_per_tableblock = 2;
        auto workload_per_tx = 1;
        for (auto const static_item: stat_data.detail) {
            auto elect_statistic = static_item.second;
            for (auto const group_item: elect_statistic.group_statistics_data) {
                common::xgroup_address_t const & group_addr = group_item.first;
                xgroup_related_statistics_data_t const & group_account_data = group_item.second;
                xvip2_t const &group_xvip2 = top::common::xip2_t{
                    group_addr.network_id(),
                    group_addr.zone_id(),
                    group_addr.cluster_id(),
                    group_addr.group_id(),
                    (uint16_t)group_account_data.account_statistics_data.size(),
                    static_item.first};
                printf("[xzec_workload_contract_v2::accumulate_workload] group xvip2: %lu, %lu\n",
                       group_xvip2.high_addr,
                       group_xvip2.low_addr);
                for (size_t slotid = 0; slotid < group_account_data.account_statistics_data.size(); ++slotid) {
                    auto account_str = group_addr.to_string() + "_" +std::to_string(slotid);
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
                }
            }
        }
    }

    void test_accumulate_workload_with_fullblock(common::xlogic_time_t const timestamp, std::map<common::xgroup_address_t, xgroup_workload_t> & group_workload) {
        // XMETRICS_TIME_RECORD(XWORKLOAD_CONTRACT "accumulate_total_time");
        // xinfo("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] enum_vbucket_has_tables_count %d, timestamp: %llu", enum_vledger_const::enum_vbucket_has_tables_count, timestamp);
        int64_t table_pledge_balance_change_tgas = 0;
        for (auto i = 0; i < enum_vledger_const::enum_vbucket_has_tables_count; i++) {
        // for (auto i = 0; i < 10; i++) {
            // calc table address
            auto table_owner = common::xaccount_address_t{xdatautil::serialize_owner_str(sys_contract_sharding_table_block_addr, i)};
            {
                std::string value_str;
                m_store->map_get(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY, std::to_string(i), value_str);
                if (!value_str.empty()) {
                    uint32_t last_read_height = base::xstring_utl::touint64(value_str);
                }
            }
            // get table block
            auto full_blocks = test_get_fullblock(table_owner, timestamp);
            uint32_t total_table_block_count = 0;
            // accumulate workload
            for (std::size_t block_index = 0; block_index < full_blocks.size(); block_index++) {
                xfull_tableblock_t *full_tableblock = dynamic_cast<xfull_tableblock_t *>(full_blocks[block_index].get());
                assert(full_tableblock != nullptr);
                auto const & stat_data = full_tableblock->get_table_statistics();
                test_accumulate_workload(stat_data, group_workload);
                // m_table_pledge_balance_change_tgas
                table_pledge_balance_change_tgas += full_tableblock->get_pledge_balance_change_tgas();
                total_table_block_count++;
                // xinfo("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] block_index: %u, total_table_block_count: %" PRIu32 "", block_index, total_table_block_count);
            }
            {
                std::string value_str;
                m_store->map_get(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY, std::to_string(i), value_str);
                if (!value_str.empty()) {
                    uint32_t last_read_height = base::xstring_utl::touint64(value_str);
                }
            }
            {
                std::string value_str;
                m_store->map_get(sys_contract_zec_workload_addr, XPORPERTY_CONTRACT_TABLEBLOCK_HEIGHT_KEY, std::to_string(i), value_str);
                if (!value_str.empty()) {
                    uint32_t last_read_height = base::xstring_utl::touint64(value_str);
                }
            }

            // std::cout << "table: " << i << ", fullblock size: " <<  full_blocks.size() << std::endl;

            if (full_blocks.size() >  0) {
                xinfo(
                    "[xzec_workload_contract_v2::accumulate_workload_with_fullblock] bucket index: %d, timer round: %" PRIu64 ", pid: %d, total_table_block_count: %" PRIu32 ", table_pledge_balance_change_tgas: %lld, "
                    "this: %p\n",
                    i,
                    timestamp,
                    getpid(),
                    total_table_block_count,
                    table_pledge_balance_change_tgas,
                    this);
    #if 0
                for (auto & entity : auditor_group_workload) {
                    auto const & group = entity.first;
                    for (auto const & wl : entity.second.m_leader_count) {
                        printf("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] workload auditor group: %s, leader: %s, workload: %u", group.to_string().c_str(), wl.first.c_str(), wl.second);
                    }
                    printf("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] workload auditor group: %s, group id: %u, ends", group.to_string().c_str(), group.group_id().value());
                }

                for (auto & entity : validator_group_workload) {
                    auto const & group = entity.first;
                    for (auto const & wl : entity.second.m_leader_count) {
                        printf("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] workload validator group: %s, leader: %s, workload: %u", group.to_string().c_str(), wl.first.c_str(), wl.second);
                    }
                    printf("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] workload validator group: %s, group id: %u, ends", group.to_string().c_str(), group.group_id().value());
                }
    #endif
            }
        }
        //update_tgas(table_pledge_balance_change_tgas);
    }
    static void SetUpTestCase() {
        auto hash_plugin = new xtop_hash_t();
        top::config::config_register.get_instance().set(config::xmin_free_gas_balance_onchain_goverance_parameter_t::name, std::to_string(ASSET_TOP(100)));
        top::config::config_register.get_instance().set(config::xfree_gas_onchain_goverance_parameter_t::name, std::to_string(25000));
        top::config::config_register.get_instance().set(config::xmax_validator_stake_onchain_goverance_parameter_t::name, std::to_string(5000));
        top::config::config_register.get_instance().set(config::xchain_name_configuration_t::name, std::string{top::config::chain_name_testnet});
        data::xrootblock_para_t para;
        data::xrootblock_t::init(para);
    }
    static void TearDownTestCase() {}
};
using xtest_workload_contract_v2_t = xtop_test_workload_contract_v2;

TEST_F(xtest_workload_contract_v2_t, test_construct_data){
    construct_data();

    common::xgroup_address_t group_addr7(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{7});
    common::xgroup_address_t group_addr6(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{6});
    common::xgroup_address_t group_addr3(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{3});
    EXPECT_EQ(data.detail.size(), 2);
    EXPECT_EQ(data.detail[20].group_statistics_data.size(), 2);
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data.size(), 9);
    for(int i = 0; i < 8; i++){
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[i].block_data.block_count, 0);
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[i].block_data.transaction_count, 0);
    }
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[8].block_data.block_count, 11);
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[8].block_data.transaction_count, 110);

    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data.size(), 9);
    for(int i = 0; i < 8; i++){
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[i].block_data.block_count, 0);
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[i].block_data.transaction_count, 0);
    }
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[8].block_data.block_count, 12);
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[8].block_data.transaction_count, 120);

    EXPECT_EQ(data.detail[10].group_statistics_data.size(), 1);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data.size(), 7);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[0].block_data.block_count, 30);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[0].block_data.transaction_count, 300);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[1].block_data.block_count, 14);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[1].block_data.transaction_count, 140);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[2].block_data.block_count, 15);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[2].block_data.transaction_count, 150);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[3].block_data.block_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[3].block_data.transaction_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[4].block_data.block_count, 10);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[4].block_data.transaction_count, 100);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[5].block_data.block_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[5].block_data.transaction_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[6].block_data.block_count, 16);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[6].block_data.transaction_count, 160);
} 

TEST_F(xtest_workload_contract_v2_t, test_accumulate_workload){
    construct_data();
    auto const stat_data = data;
    std::map<common::xgroup_address_t, xgroup_workload_t> group_workload;
    test_accumulate_workload(stat_data, group_workload);
    EXPECT_EQ(group_workload.size(), 3);
    common::xgroup_address_t group_addr7(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{7});
    common::xgroup_address_t group_addr6(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{6});
    common::xgroup_address_t group_addr3(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{3});


    EXPECT_EQ(group_workload[group_addr3].m_leader_count.size(), 5);
    EXPECT_EQ(group_workload[group_addr3].m_leader_count[group_addr3.to_string() + "_" + std::to_string(0)], (130+170)+2*30);
    EXPECT_EQ(group_workload[group_addr3].m_leader_count[group_addr3.to_string() + "_" + std::to_string(1)], 140+2*14);
    EXPECT_EQ(group_workload[group_addr3].m_leader_count[group_addr3.to_string() + "_" + std::to_string(2)], 150+2*15);
    EXPECT_EQ(group_workload[group_addr3].m_leader_count[group_addr3.to_string() + "_" + std::to_string(3)], 0);
    EXPECT_EQ(group_workload[group_addr3].m_leader_count[group_addr3.to_string() + "_" + std::to_string(4)], 100+2*10);
    EXPECT_EQ(group_workload[group_addr3].m_leader_count[group_addr3.to_string() + "_" + std::to_string(5)], 0);
    EXPECT_EQ(group_workload[group_addr3].m_leader_count[group_addr3.to_string() + "_" + std::to_string(6)], 160+2*16);

    EXPECT_EQ(group_workload[group_addr7].m_leader_count.size(), 1);
    for(size_t slotid = 0; slotid < 9; ++slotid) {
        if(slotid == 8) {
            EXPECT_EQ(group_workload[group_addr7].m_leader_count[group_addr7.to_string() + "_" + std::to_string(slotid)], 110+2*11);
        } else {
            EXPECT_EQ(group_workload[group_addr7].m_leader_count[group_addr7.to_string() + "_" + std::to_string(slotid)], 0);
        }
    }
    EXPECT_EQ(group_workload[group_addr6].m_leader_count.size(), 1);  
    for(size_t slotid = 0; slotid < 9; ++slotid) {
        if(slotid == 8) {
            EXPECT_EQ(group_workload[group_addr6].m_leader_count[group_addr6.to_string() + "_" + std::to_string(slotid)], 120+2*12);
        } else {
            EXPECT_EQ(group_workload[group_addr6].m_leader_count[group_addr6.to_string() + "_" + std::to_string(slotid)], 0);
        }
    }  
}

TEST_F(xtest_workload_contract_v2_t, test_accumulate_workload_with_fullblock) {
    if (access(DB_PATH, 0) != 0) {
        return;
    }
    clock_t start, end;
    {
        start = clock();
        std::map<common::xgroup_address_t, xgroup_workload_t> group_workload;
        test_accumulate_workload_with_fullblock(20, group_workload);
        end = clock();
        clock_t interval = end - start;
        std::cout << "table num: 20" << ", start: " << start << ", end: " << end << "interval: " << interval << std::endl;
        // for (auto & entity : group_workload) {
        //     auto const & group = entity.first;
        //     for (auto const & wl : entity.second.m_leader_count) {
        //         printf("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] workload auditor group: %s, leader: %s, workload: %u\n", group.to_string().c_str(), wl.first.c_str(), wl.second);
        //     }
        //     printf("[xzec_workload_contract_v2::accumulate_workload_with_fullblock] workload auditor group: %s, group id: %u, ends\n\n", group.to_string().c_str(), group.group_id().value());
        // }

    }
    {
        start = clock();
        std::map<common::xgroup_address_t, xgroup_workload_t> group_workload;
        test_accumulate_workload_with_fullblock(50, group_workload);
        end = clock();
        clock_t interval = end - start;
        std::cout << "table num: 50" << ", start: " << start << ", end: " << end << "interval: " << interval << std::endl;
    }
    {
        start = clock();
        std::map<common::xgroup_address_t, xgroup_workload_t> group_workload;
        test_accumulate_workload_with_fullblock(100, group_workload);
        end = clock();
        clock_t interval = end - start;
        std::cout << "table num: 100" << ", start: " << start << ", end: " << end << ", interval: " << interval << std::endl;
    }
}

NS_END3