// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>

#include <string>

#define private public
#include "xbase/xobject_ptr.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xgenesis_data.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/xstore_face.h"
#include "xdata/xblocktool.h"
#include "xdata/xworkload_info.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract.h"
//#include "xvm/xrec/xelect/xbeacon_timer_contract.h"
#include "xvm/xsystem_contracts/xreward/xtable_vote_contract.h"
#include "xvm/xsystem_contracts/xreward/xzec_vote_contract.h"
#include "xvm/xsystem_contracts/xreward/xzec_workload_contract.h"
#include "xvm/xsystem_contracts/xworkload/xzec_workload_contract_v2.h"
#include "xvm/xsystem_contracts/xreward/xzec_reward_contract.h"
#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"
#include "xchain_upgrade/xchain_upgrade_center.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"

using namespace top;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::xstake;
using namespace top::contract;

NS_BEG3(top, xvm, system_contracts)

class xtop_test_workload_contract_v2 : public xzec_workload_contract_v2, public testing::Test {
    using xbase_t = xzec_reward_contract;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_workload_contract_v2);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_workload_contract_v2); 

    xtop_test_workload_contract_v2() : xzec_workload_contract_v2(common::xnetwork_id_t{0}) {};

    xcontract_base * clone() override { return {}; }

    void exec(top::xvm::xvm_context * vm_ctx) { return; }

    xstatistics_data_t data;

    void construct_data();
    void tableblock_statistics_handle(const xvip2_t leader_xip, const uint32_t txs_count, std::vector<base::xvoter> const & voter_info, xstatistics_data_t & data);

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
};
using xtest_workload_contract_v2_t = xtop_test_workload_contract_v2;

void xtest_workload_contract_v2_t::tableblock_statistics_handle(const xvip2_t leader_xip, const uint32_t txs_count, std::vector<base::xvoter> const & voter_info, xstatistics_data_t & data){
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
    common::xslot_id_t slot_id = common::xslot_id_t{slot_idx};
    if(it_group->second.account_statistics_data.size() < size_t(slot_idx+1)){
        it_group->second.account_statistics_data.resize(slot_idx+1);
    }
    // workload
    it_group->second.account_statistics_data[slot_idx].block_data.block_count++;
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

void xtest_workload_contract_v2_t::construct_data(){
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
        tableblock_statistics_handle(leader_xip_test[i], tx_count_test[i], voter_info_test[i], data);
    }
}

TEST_F(xtest_workload_contract_v2_t, test_construct_data){
    construct_data();

    common::xgroup_address_t group_addr7(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{7});
    common::xgroup_address_t group_addr6(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{6});
    common::xgroup_address_t group_addr3(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{3});
    EXPECT_EQ(data.detail.size(), 2);
    EXPECT_EQ(data.detail[20].group_statistics_data.size(), 2);
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data.size(), 9);
    for(int i = 0; i < 8; i++){
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[i].vote_data.vote_count, 0);
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[i].block_data.block_count, 0);
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[i].block_data.transaction_count, 0);
    }
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[8].vote_data.vote_count, 2);
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[8].block_data.block_count, 1);
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr7].account_statistics_data[8].block_data.transaction_count, 110);

    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data.size(), 9);
    for(int i = 0; i < 8; i++){
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[i].vote_data.vote_count, 0);
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[i].block_data.block_count, 0);
        EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[i].block_data.transaction_count, 0);
    }
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[8].vote_data.vote_count, 3);
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[8].block_data.block_count, 1);
    EXPECT_EQ(data.detail[20].group_statistics_data[group_addr6].account_statistics_data[8].block_data.transaction_count, 120);

    EXPECT_EQ(data.detail[10].group_statistics_data.size(), 1);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data.size(), 7);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[0].vote_data.vote_count, 12);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[0].block_data.block_count, 2);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[0].block_data.transaction_count, 300);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[1].vote_data.vote_count, 5);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[1].block_data.block_count, 1);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[1].block_data.transaction_count, 140);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[2].vote_data.vote_count, 6);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[2].block_data.block_count, 1);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[2].block_data.transaction_count, 150);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[3].vote_data.vote_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[3].block_data.block_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[3].block_data.transaction_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[4].vote_data.vote_count, 1);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[4].block_data.block_count, 1);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[4].block_data.transaction_count, 100);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[5].vote_data.vote_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[5].block_data.block_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[5].block_data.transaction_count, 0);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[6].vote_data.vote_count, 7);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[6].block_data.block_count, 1);
    EXPECT_EQ(data.detail[10].group_statistics_data[group_addr3].account_statistics_data[6].block_data.transaction_count, 160);
} 

TEST_F(xtest_workload_contract_v2_t, test_accumulate_data){
    construct_data();
    std::map<common::xgroup_address_t, xauditor_workload_info_t> bookload_auditor_group_workload_info;
    std::map<common::xgroup_address_t, xvalidator_workload_info_t> bookload_validator_group_workload_info;
    // use node service in function, cannot test function now
    //accumulate_workload(data, bookload_auditor_group_workload_info, bookload_validator_group_workload_info);    
}

NS_END3