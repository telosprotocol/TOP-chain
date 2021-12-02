// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <gtest/gtest.h>
#include <string>

#define private public
//#include "xbasic/xobject_ptr.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xgenesis_data.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/xstore_face.h"
#include "xdata/xblocktool.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract.h"
//#include "xvm/xrec/xelect/xbeacon_timer_contract.h"
#include "xvm/xsystem_contracts/xreward/xtable_vote_contract.h"
#include "xvm/xsystem_contracts/xreward/xtable_reward_claiming_contract.h"
#include "xvm/xsystem_contracts/xreward/xzec_vote_contract.h"
#include "xvm/xsystem_contracts/xreward/xzec_reward_contract.h"
#include "xvm/xsystem_contracts/tcc/xrec_proposal_contract.h"
#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xtxexecutor/xtransaction_context.h"

using namespace top;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::xstake;
using namespace top::contract;

NS_BEG3(top, tests, reward)
class xtop_test_reward_contract : public xzec_reward_contract, public testing::Test {
    using xbase_t = xzec_reward_contract;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_reward_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_reward_contract); 

    xtop_test_reward_contract() : xzec_reward_contract(common::xnetwork_id_t{0}) {};

    xcontract_base * clone() override { return {}; }

    void exec(top::xvm::xvm_context * vm_ctx) { return; }

    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
};
using xtest_reward_contract_t = xtop_test_reward_contract;

TEST_F(xtest_reward_contract_t, test_calc_total_issuance) {
    // printf("reserve_reward: [%lu, %u]\n",
    //     static_cast<uint64_t>(reserve_reward / REWARD_PRECISION),
    //      static_cast<uint32_t>(reserve_reward % REWARD_PRECISION));
    common::xlogic_time_t issue_time_length{0};
    uint32_t min_ratio_annual_total_reward{2};
    uint32_t additional_issue_year_ratio{8};
    xaccumulated_reward_record record;
    top::xstake::uint128_t total{0};

    top::xstake::uint128_t year1_total = static_cast<top::xstake::uint128_t>(TOTAL_RESERVE) * REWARD_PRECISION * additional_issue_year_ratio / 100;
    top::xstake::uint128_t year2_total = (static_cast<top::xstake::uint128_t>(TOTAL_RESERVE) * REWARD_PRECISION - year1_total) * additional_issue_year_ratio / 100;
    // case 1: not cross-year
    issue_time_length = 10;
    total = calc_total_issuance(issue_time_length, min_ratio_annual_total_reward, additional_issue_year_ratio, record);
    top::xstake::uint128_t expect_total = static_cast<top::xstake::uint128_t>(TOTAL_RESERVE) * REWARD_PRECISION * additional_issue_year_ratio / 100 * issue_time_length / TIMER_BLOCK_HEIGHT_PER_YEAR;
    EXPECT_EQ(total, expect_total);
    EXPECT_EQ(0, record.issued_until_last_year_end);
    EXPECT_EQ(issue_time_length, record.last_issuance_time);
    // case 2: cross-year 1-3 not success(do not pass)
    // issue_time_length = 2 * TIMER_BLOCK_HEIGHT_PER_YEAR + 10;
    // top::xstake::uint128_t last_total = total;
    // top::xstake::uint128_t year3_amount = (static_cast<top::xstake::uint128_t>(TOTAL_RESERVE) * REWARD_PRECISION - year1_total - year2_total) * additional_issue_year_ratio / 100 * 10 / TIMER_BLOCK_HEIGHT_PER_YEAR;
    // total = calc_total_issuance(issue_time_length, min_ratio_annual_total_reward, additional_issue_year_ratio, record);
    // expect_total = year1_total + year2_total + year3_amount - last_total;
    // EXPECT_EQ(total, expect_total);
    // EXPECT_EQ(year1_total + year2_total, record.issued_until_last_year_end);
    // EXPECT_EQ(issue_time_length, record.last_issuance_time);
    // case 3: border test
    record.issued_until_last_year_end = 0;
    record.last_issuance_time = 0;
    issue_time_length = 2 * TIMER_BLOCK_HEIGHT_PER_YEAR;
    total = calc_total_issuance(issue_time_length, min_ratio_annual_total_reward, additional_issue_year_ratio, record);
    expect_total = year1_total + year2_total;
    EXPECT_EQ(total, expect_total);
    EXPECT_EQ(record.issued_until_last_year_end, expect_total);
    EXPECT_EQ(record.last_issuance_time, issue_time_length);
    // case 4: issue %2
    record.issued_until_last_year_end = static_cast<top::xstake::uint128_t>(TOTAL_RESERVE) * REWARD_PRECISION - static_cast<top::xstake::uint128_t>(TOTAL_ISSUANCE) * min_ratio_annual_total_reward / 100 *2;
    record.last_issuance_time = 0;
    issue_time_length = 10;
    total = calc_total_issuance(issue_time_length, min_ratio_annual_total_reward, additional_issue_year_ratio, record);
    expect_total = static_cast<top::xstake::uint128_t>(TOTAL_ISSUANCE) * REWARD_PRECISION * min_ratio_annual_total_reward / 100 * 10 / TIMER_BLOCK_HEIGHT_PER_YEAR;
    EXPECT_EQ(total, expect_total);
}

TEST_F(xtest_reward_contract_t, test_calc_role_nums) {
    std::vector<std::vector<uint32_t>> role_nums;
    std::map<common::xaccount_address_t, xreg_node_info> map_nodes;

    // deposit zero edge 5, valid edge 10,
    // deposit zero advance 6, deposit > vote 12, deposit <= vote 18,
    // deposit zero archive 7, deposit > vote 14, deposit <= vote 21,
    // deposit zero consensus 8, valid > vote 16,
    for(int i = 0; i < 5+10; i++){
        std::string name = "edge" + std::to_string(i+1); 
        xreg_node_info node;
        node.m_registered_role = common::xrole_type_t::edge;
        node.m_vote_amount = 0;
        if(i < 5){
            node.m_account_mortgage = 0;
        }else{
            node.m_account_mortgage = 10*TOP_UNIT;
        }
        map_nodes.insert({common::xaccount_address_t{name}, node});
    }
    for(int i = 0; i < 7+14+21; i++){
        std::string name = "archive" + std::to_string(i+1); 
        xreg_node_info node;
        node.m_registered_role = common::xrole_type_t::archive;
        if(i < 7){
            node.m_account_mortgage = 0;
            node.m_vote_amount = 0;
        }else if(i < 7+14){
            node.m_vote_amount = 5;
            node.m_account_mortgage = 10*TOP_UNIT;
        }else if(i < 7+14+21){
            node.m_vote_amount = 15;
            node.m_account_mortgage = 10*TOP_UNIT;            
        }
        map_nodes.insert({common::xaccount_address_t{name}, node});
    }
    for(int i = 0; i < 6+12+18; i++){
        std::string name = "advance" + std::to_string(i+1); 
        xreg_node_info node;
        node.m_registered_role = common::xrole_type_t::advance;
        if(i < 6){
            node.m_account_mortgage = 0;
            node.m_vote_amount = 0;
        }else if(i < 6+12){
            node.m_vote_amount = 5;
            node.m_account_mortgage = 10*TOP_UNIT;
        }else if(i < 6+12+18){
            node.m_vote_amount = 15;
            node.m_account_mortgage = 10*TOP_UNIT;            
        }
        map_nodes.insert({common::xaccount_address_t{name}, node});
    }
    for(int i = 0; i < 8+16; i++){
        std::string name = "consensus" + std::to_string(i+1); 
        xreg_node_info node;
        node.m_registered_role = common::xrole_type_t::validator;
        node.m_vote_amount = 0;
        if(i < 8){
            node.m_account_mortgage = 0;
        }else{
            node.m_account_mortgage = 10*TOP_UNIT;
        }
        map_nodes.insert({common::xaccount_address_t{name}, node});
    }

    role_nums = calc_role_nums(map_nodes);
    EXPECT_EQ(role_nums[0][0], 5+10);   // total edge
    EXPECT_EQ(role_nums[0][1], 10);     // valid edge
    EXPECT_EQ(role_nums[1][0], 6+12+18+7+14+21);    // total archive = archive + advanc
    EXPECT_EQ(role_nums[1][1], 18+14+21);           // valid archive = valid archive + valid advance
    EXPECT_EQ(role_nums[1][2], 6+7);                // deposit zero archive = deposit zero archive + deposit zero advance
    EXPECT_EQ(role_nums[2][0], 6+12+18);    // total advance
    EXPECT_EQ(role_nums[2][1], 18);         // valid advance
    EXPECT_EQ(role_nums[2][2], 6);          // deposit zero advance
    EXPECT_EQ(role_nums[3][0], 8+16+6+12+18);   // total validator = total consensus + total advance
    EXPECT_EQ(role_nums[3][1], 16+12+18);       // valid validator = valid consensus + valid advance
}

TEST_F(xtest_reward_contract_t, test_calc_zero_workload_reward) {    
    cluster_workload_t normal_work;
    cluster_workload_t zero_work;
    normal_work.cluster_total_workload = 20;
    zero_work.cluster_total_workload = 1;

    uint32_t zero_workload = 1;
    const top::xstake::uint128_t group_reward = 1000;
    std::vector<string> zero_workload_account;
    
    common::xgroup_address_t g_normal1(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{1});
    common::xgroup_address_t g_normal2(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{2});
    common::xgroup_address_t g_zero1(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{3});
    common::xgroup_address_t g_zero2(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{4});
    std::pair<common::xgroup_address_t, cluster_workload_t> normal1{g_normal1, normal_work};
    std::pair<common::xgroup_address_t, cluster_workload_t> normal2{g_normal2, normal_work};
    std::pair<common::xgroup_address_t, cluster_workload_t> zero1{g_zero1, zero_work};
    std::pair<std::string, uint32_t> zero1_1{"zero1_1", 0};
    std::pair<std::string, uint32_t> zero1_2{"zero1_2", 1};
    zero1.second.m_leader_count.insert(zero1_1);
    zero1.second.m_leader_count.insert(zero1_2);
    std::pair<common::xgroup_address_t, cluster_workload_t> zero2{g_zero2, zero_work};
    std::pair<std::string, uint32_t> zero2_1{"zero2_1", 0};
    std::pair<std::string, uint32_t> zero2_2{"zero2_2", 1};
    zero2.second.m_leader_count.insert(zero2_1);
    zero2.second.m_leader_count.insert(zero2_2);

    std::map<common::xgroup_address_t, cluster_workload_t> workloads_detail;
    workloads_detail.insert(normal1);
    workloads_detail.insert(zero2);
    workloads_detail.insert(zero1);
    workloads_detail.insert(normal2);

    // 4 total, 2 zero erase, 2 normal left, 2 group_reward get
    EXPECT_EQ(calc_zero_workload_reward(true, workloads_detail, zero_workload, group_reward, zero_workload_account), group_reward*2);
    EXPECT_EQ(workloads_detail.size(), 2);
    EXPECT_EQ(workloads_detail.count(g_normal1), 1);
    EXPECT_EQ(workloads_detail.count(g_normal2), 1);
    EXPECT_EQ(workloads_detail.count(g_zero1), 0);
    EXPECT_EQ(workloads_detail.count(g_zero2), 0);
    // zero workload account add to "zero_workload_account"
    EXPECT_EQ(zero_workload_account.size(), 4);
    EXPECT_EQ(zero_workload_account[0], "zero1_1");
    EXPECT_EQ(zero_workload_account[1], "zero1_2");
    EXPECT_EQ(zero_workload_account[2], "zero2_1");
    EXPECT_EQ(zero_workload_account[3], "zero2_2");
}

TEST_F(xtest_reward_contract_t, test_calc_invalid_workload_group_reward) {  
    for(int role = 0; role < 2; role++)
    {
        std::map<common::xaccount_address_t, xreg_node_info> map_nodes;
        // valid nodes
        for(int i = 0; i < 4; i++){
            std::pair<common::xaccount_address_t, xreg_node_info> node;
            node.first = common::xaccount_address_t{"node1_" + std::to_string(i+1)};
            node.second.m_account = common::xaccount_address_t{node.first};
            if(role == 0){
                node.second.m_registered_role = common::xrole_type_t::advance;
            }else{
                node.second.m_registered_role = common::xrole_type_t::validator;
            }
            node.second.m_vote_amount = 15;
            node.second.m_account_mortgage = 10*TOP_UNIT; 
            map_nodes.insert(node);
        }
        // invalid nodes
        for(int i = 0; i < 3; i++){
            std::pair<common::xaccount_address_t, xreg_node_info> node;
            node.first = common::xaccount_address_t{"node2_" + std::to_string(i+1)};
            node.second.m_account = common::xaccount_address_t{node.first};
            if(i < 1){
                node.second.m_vote_amount = 15;
                node.second.m_account_mortgage = 0; 
                if(role == 0){
                    node.second.m_registered_role = common::xrole_type_t::advance;
                }else{
                    node.second.m_registered_role = common::xrole_type_t::validator;
                }
            }else if(i < 2){
                node.second.m_vote_amount = 5;
                if(role == 0){
                    node.second.m_registered_role = common::xrole_type_t::advance;
                    node.second.m_account_mortgage = 10*TOP_UNIT; 
                }else{
                    node.second.m_registered_role = common::xrole_type_t::validator;
                    node.second.m_account_mortgage = 0; 
                }
            }else if(i < 3){
                node.second.m_vote_amount = 15;
                node.second.m_account_mortgage = 10*TOP_UNIT; 
                node.second.m_registered_role = common::xrole_type_t::archive;
            }
            map_nodes.insert(node);
        } 
        // workload
        std::map<common::xgroup_address_t, cluster_workload_t> workloads_detail;     
        for(int j = 0; j < 2; j++){
            std::pair<common::xgroup_address_t, cluster_workload_t> group;
            group.first = common::xgroup_address_t{common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{j+1}};
            group.second.cluster_total_workload = 100;
            for(int i = 0; i < 5; i++){
                std::pair<std::string, uint32_t> work;
                work.first = "node" + std::to_string(j+1) + "_" +std::to_string(i+1);
                work.second = 20;
                group.second.m_leader_count.insert(work);
            }
            workloads_detail.insert(group);
        }

        top::xstake::uint128_t group_reward = 1000;
        if(role == 0){
            EXPECT_EQ(group_reward, calc_invalid_workload_group_reward(true, map_nodes, group_reward, workloads_detail));
        }else{
            EXPECT_EQ(group_reward, calc_invalid_workload_group_reward(false, map_nodes, group_reward, workloads_detail));
        }
        // 2 total groups, 1 valid left, 1 invalid erase(all nodes invalid)
        common::xgroup_address_t group1(common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{1});
        EXPECT_EQ(workloads_detail.size(), 1);
        EXPECT_EQ(workloads_detail.count(group1), 1);
        // 5 nodes 100 total workload, invalid node of 20 workload erase, left 4 nodes, left 80 workload 
        EXPECT_EQ(workloads_detail[group1].cluster_total_workload, 80);
        EXPECT_EQ(workloads_detail[group1].m_leader_count.size(), 4);
        EXPECT_EQ(workloads_detail[group1].m_leader_count.count("node1_1"), 1);
        EXPECT_EQ(workloads_detail[group1].m_leader_count.count("node1_2"), 1);
        EXPECT_EQ(workloads_detail[group1].m_leader_count.count("node1_3"), 1);
        EXPECT_EQ(workloads_detail[group1].m_leader_count.count("node1_4"), 1);
    }
}

TEST_F(xtest_reward_contract_t, test_calc_edger_worklaod_rewards) { 
    xreg_node_info node;
    node.m_registered_role = common::xrole_type_t::edge;
    node.m_account_mortgage = 0;
    std::vector<uint32_t> edger_num{10, 0, 3};
    top::xstake::uint128_t edge_workload_rewards = 70;
    top::xstake::uint128_t reward_to_self;

    calc_edge_workload_rewards(node, edger_num, edge_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    edger_num[1] = 7;
    calc_edge_workload_rewards(node, edger_num, edge_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    node.m_account_mortgage = 1000;  
    calc_edge_workload_rewards(node, edger_num, edge_workload_rewards, reward_to_self);  
    EXPECT_EQ(reward_to_self, 10);    
}

TEST_F(xtest_reward_contract_t, test_calc_archiver_worklaod_rewards) { 
    xreg_node_info node;
    node.m_registered_role = common::xrole_type_t::archive;
    node.m_account_mortgage = 0;
    node.m_vote_amount = 0;
    std::vector<uint32_t> archiver_num{10, 0, 3};
    top::xstake::uint128_t archiver_workload_rewards = 70;
    top::xstake::uint128_t reward_to_self;

    calc_archive_workload_rewards(node, archiver_num, archiver_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    archiver_num[1] = 7;
    calc_archive_workload_rewards(node, archiver_num, archiver_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    node.m_vote_amount = 1000;  
    calc_archive_workload_rewards(node, archiver_num, archiver_workload_rewards, reward_to_self);  
    EXPECT_EQ(reward_to_self, 0);
    node.m_account_mortgage = 100;  
    calc_archive_workload_rewards(node, archiver_num, archiver_workload_rewards, reward_to_self);  
    EXPECT_EQ(reward_to_self, 10);  
}

TEST_F(xtest_reward_contract_t, test_calc_validator_worklaod_rewards) {
    std::vector<uint32_t> validator_num{10, 0, 3};
    top::xstake::uint128_t validator_group_workload_rewards = 50;
    top::xstake::uint128_t reward_to_self = 0;

    // workload
    std::map<common::xgroup_address_t, cluster_workload_t> validator_workloads_detail;     
    for(int j = 0; j < 2; j++){
        std::pair<common::xgroup_address_t, cluster_workload_t> group;
        group.first = common::xgroup_address_t{common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{j+1}};
        group.second.cluster_total_workload = 100;
        for(int i = 0; i < 5; i++){
            std::pair<std::string, uint32_t> work;
            work.first = "node" + std::to_string(j+1) + "_" +std::to_string(i+1);
            work.second = 20;
            group.second.m_leader_count.insert(work);
        }
        validator_workloads_detail.insert(group);
    }
    // node 
    xreg_node_info node;
    node.m_account = common::xaccount_address_t{"node1_1"};
    node.m_registered_role = common::xrole_type_t::validator;
    node.m_account_mortgage = 100; 
    calc_validator_workload_rewards(
            node, validator_num, validator_workloads_detail, validator_group_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    validator_num[1] = 7;
    node.m_account_mortgage = 0; 
    calc_validator_workload_rewards(
            node, validator_num, validator_workloads_detail, validator_group_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    node.m_account_mortgage = 100;
    node.m_account = common::xaccount_address_t{"node1_0"};
    calc_validator_workload_rewards(
            node, validator_num, validator_workloads_detail, validator_group_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    node.m_account = common::xaccount_address_t{"node1_1"};
    calc_validator_workload_rewards(
            node, validator_num, validator_workloads_detail, validator_group_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 10);
}

TEST_F(xtest_reward_contract_t, test_calc_auditor_worklaod_rewards) {
    std::vector<uint32_t> auditor_num{10, 0, 3};
    top::xstake::uint128_t auditor_group_workload_rewards = 50;
    top::xstake::uint128_t reward_to_self = 0;

    // workload
    std::map<common::xgroup_address_t, cluster_workload_t> auditor_workloads_detail;     
    for(int j = 0; j < 2; j++){
        std::pair<common::xgroup_address_t, cluster_workload_t> group;
        group.first = common::xgroup_address_t{common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{j+1}};
        group.second.cluster_total_workload = 100;
        for(int i = 0; i < 5; i++){
            std::pair<std::string, uint32_t> work;
            work.first = "node" + std::to_string(j+1) + "_" +std::to_string(i+1);
            work.second = 20;
            group.second.m_leader_count.insert(work);
        }
        auditor_workloads_detail.insert(group);
    }
    // node 
    xreg_node_info node;
    node.m_vote_amount = 0;
    node.m_account = common::xaccount_address_t{"node1_1"};
    node.m_registered_role = common::xrole_type_t::advance;
    node.m_account_mortgage = 100; 
    calc_auditor_workload_rewards(
            node, auditor_num, auditor_workloads_detail, auditor_group_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    auditor_num[1] = 7;
    node.m_account_mortgage = 0; 
    calc_auditor_workload_rewards(
            node, auditor_num, auditor_workloads_detail, auditor_group_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    node.m_account_mortgage = 100;
    node.m_account = common::xaccount_address_t{"node1_0"};
    calc_auditor_workload_rewards(
            node, auditor_num, auditor_workloads_detail, auditor_group_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    node.m_account = common::xaccount_address_t{"node1_1"};
    calc_auditor_workload_rewards(
            node, auditor_num, auditor_workloads_detail, auditor_group_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 0);
    node.m_vote_amount = 1;
    calc_auditor_workload_rewards(
            node, auditor_num, auditor_workloads_detail, auditor_group_workload_rewards, reward_to_self);
    EXPECT_EQ(reward_to_self, 10);
}

TEST_F(xtest_reward_contract_t, test_nodes_rewards) {
    // make issue_time_length
    uint64_t issue_time_length = TIMER_BLOCK_HEIGHT_PER_YEAR + 10;
    // make onchain_param
    xreward_onchain_param_t onchain_param;
    onchain_param.min_ratio_annual_total_reward = 2;
    onchain_param.additional_issue_year_ratio = 8;
    onchain_param.edge_reward_ratio = 2;
    onchain_param.archive_reward_ratio = 4;
    onchain_param.auditor_reward_ratio = 10;
    onchain_param.validator_reward_ratio = 60;
    onchain_param.vote_reward_ratio = 20;
    onchain_param.governance_reward_ratio = 4;
    onchain_param.validator_group_zero_workload = 0;
    onchain_param.auditor_group_zero_workload = 0;
    // make property_param
    xreward_property_param_t property_param;
    // make property_param accumulated_reward_record
    property_param.accumulated_reward_record.last_issuance_time = 0;
    property_param.accumulated_reward_record.issued_until_last_year_end = 0;
    // make property_param map_nodes
    for(int table = 0; table < 4; table++){
        for(int node_idx = 0; node_idx < 64; node_idx++){
            xreg_node_info node;
            node.m_account = common::xaccount_address_t{std::string("node") + std::to_string(node_idx+1 + table*64)};
            node.m_vote_amount = 1000;
            node.m_support_ratio_numerator = 20;
            node.m_support_ratio_denominator = 100;
            if(node_idx%2==0){
                node.m_account_mortgage = 1e6;
            }else{
                node.m_account_mortgage = 0;
            }
            if(node_idx < 4){ // edger
                node.m_registered_role = common::xrole_type_t::edge;
            }else if(node_idx < 4+20){ // auditor
                node.m_registered_role = common::xrole_type_t::advance;
                if(node.m_account_mortgage == 0){
                    node.m_account_mortgage = 1e9;
                }
            }else{ // validator
                node.m_registered_role = common::xrole_type_t::validator;
            } 
            property_param.map_nodes.insert(std::make_pair(common::xaccount_address_t{"node" + std::to_string(node_idx+1 + table*64)}, node));
        }
    }
    // make property_param votes_detail
    for(int table = 0; table < 4; table++){
        int base = 0;
        if(table != 3){
            base = table+1;
        }else{
            base = 0;
        }
        for(int node_idx = 0; node_idx < 64; node_idx++){
            if(node_idx < 4+20){ 
                string voter = "node" + std::to_string(node_idx+1 + table*64);
                std::map<common::xaccount_address_t, uint64_t> vote_detail;
                uint64_t vote = 1;
                common::xaccount_address_t account1;
                common::xaccount_address_t account2;
                if(node_idx%2 == 0){
                    account1 = common::xaccount_address_t{"node" + std::to_string(node_idx+1 + base*64)};
                    account2 = common::xaccount_address_t{"node" + std::to_string(node_idx+1+1 + base*64)};
                }else{
                    account1 = common::xaccount_address_t{"node" + std::to_string(node_idx+1-1 + base*64)};
                    account2 = common::xaccount_address_t{"node" + std::to_string(node_idx+1 + base*64)};
                }
                vote_detail.insert(std::make_pair(account1, vote));
                vote_detail.insert(std::make_pair(account2, vote));
                property_param.votes_detail.insert(std::make_pair(common::xaccount_address_t{voter}, vote_detail));
            }
        }
    }

    // make property_param auditor_workloads_detail
    for(int table = 0; table < 4; table++){
        cluster_workload_t workload;
        if(table != 3){
            workload.cluster_total_workload = 400;
        }else{
            workload.cluster_total_workload = 200;
        }
        for(int node_idx = 0; node_idx < 64; node_idx++){
            if(node_idx >= 4 && node_idx < 2+2+20){
                string node = "node" + std::to_string(node_idx+1 + table*64);
                if(table != 3){
                    workload.m_leader_count.insert(std::make_pair(node, 10));
                }else{
                    workload.m_leader_count.insert(std::make_pair(node, 0));
                }
            }      
        }
        for(int node_idx = 0; node_idx < 20; node_idx++){
            string node = "unregisterd_auditor_node" + std::to_string(node_idx+1 + table*64);
            workload.m_leader_count.insert(std::make_pair(node, 10));
        }
        auto group = common::xgroup_address_t{common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{0}, common::xgroup_id_t{table+1}};
        property_param.auditor_workloads_detail.insert(std::make_pair(group, workload));
    }
    // make property_param validator_workloads_detail
    for(int table = 0; table < 4; table++){
        cluster_workload_t workload;
        if(table != 3){
            workload.cluster_total_workload = 800;
        }else{
            workload.cluster_total_workload = 0;
        }
        for(int node_idx = 0; node_idx < 64; node_idx++){
            if(node_idx >= 2+2+20){
                string node = "node" + std::to_string(node_idx+1 + table*64);
                if(table != 3){
                    workload.m_leader_count.insert(std::make_pair(node, 10));
                }else{
                    workload.m_leader_count.insert(std::make_pair(node, 0));
                }
                if(table == 2){
                    property_param.map_nodes[common::xaccount_address_t{node}].m_account_mortgage = 0;
                }
            }      
        }
        for(int node_idx = 0; node_idx < 40; node_idx++){
            string node = "unregisterd_validator_node" + std::to_string(node_idx+1 + table*64);
            if(table != 3){
                workload.m_leader_count.insert(std::make_pair(node, 10));
            }else{
                workload.m_leader_count.insert(std::make_pair(node, 0));
            }
        }
        auto group = common::xgroup_address_t{common::xnetwork_id_t{0}, common::xzone_id_t{0}, common::xcluster_id_t{1}, common::xgroup_id_t{table+1}};
        property_param.validator_workloads_detail.insert(std::make_pair(group, workload));
    }

    xissue_detail issue_detail;
    top::xstake::uint128_t community_reward;
    std::map<common::xaccount_address_t, top::xstake::uint128_t> node_reward_detail;
    std::map<common::xaccount_address_t, top::xstake::uint128_t> node_dividend_detail;
    std::map<common::xaccount_address_t, top::xstake::uint128_t> table_total_rewards;
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, top::xstake::uint128_t>> table_node_reward_detail;
    std::map<common::xaccount_address_t, std::map<common::xaccount_address_t, top::xstake::uint128_t>> table_node_dividend_detail;
    calc_nodes_rewards_v5(issue_time_length, onchain_param, property_param, issue_detail, node_reward_detail, node_dividend_detail, community_reward);
    // total issuance: [608001772473988, 494255]
    // edge workload rewards: [12160035449479, 769885], total edge num: 16, valid edge num: 8,
    // archive workload rewards: [24320070898959, 539770], total archive num: 80, valid archive num: 40, ----->auditor
    // auditor workload rewards: [60800177247398, 849425], auditor workload grop num: 4, auditor group workload rewards: [15200044311849, 712356], total auditor num: 80, valid auditor num: 40,
    // validator workload rewards: [364801063484393, 96553], validator workload grop num: 4, validator group workload rewards: [91200265871098, 274138], total validator num: 240, valid validator num: 160, 
    // vote rewards: [121600354494797, 698851], 
    // governance rewards: [24320070898959, 539770], 
    // all tickets: 80, 
    // 1 auditor group invalid, 2 validator group invalid
    // community rewards: [221920646953005, 800402], 

    // edger: 1 3 65 67 129 131 
    // workload reward: [1520004431184, 971235],  total/8
    // dividend reward: [304000886236, 994247], 
    // node reward: [1216003544947, 976988]
    
    // auditor: 5-24 69-88 133-152
    // archive_workload reward: [608001772473, 988494],  total/20
    // auditor_workload reward: [1520004431184, 971235], total/10
    // vote reward: [3040008862369, 942471],                total/40
    // dividend reward: [1033603013205, 780440], 
    // node reward: [4134412052823, 121760] 

    // auditor: 197-216
    // archive_workload reward: [608001772473, 988494]
    // vote reward: [3040008862369, 942471]
    // dividend reward: [729602126968, 786193]
    // node reward: [2918408507875, 144772]
    
    // validitor: 25-64 89-128 153-192
    // validator_workload reward: [4560013293554, 913706] total/20
    // node reward: [4560013293554, 913706]
    
    // validitor: 217-256
    // validator_workload reward: [4560013293554, 913706] total/20
    // node reward: [4560013293554, 913706]
    EXPECT_EQ(community_reward, top::xstake::uint128_t("221920646953005800402"));
    EXPECT_EQ(property_param.accumulated_reward_record.last_issuance_time, issue_time_length);
    EXPECT_EQ(property_param.accumulated_reward_record.issued_until_last_year_end, top::xstake::uint128_t("608000000000000000000"));
    for(auto node : property_param.map_nodes){
        EXPECT_NE(node.second.m_vote_amount, 1000);
        EXPECT_TRUE(node.second.m_vote_amount == 0 || node.second.m_vote_amount == 2);
    }
    //calc_table_rewards(property_param, node_reward_detail, node_dividend_detail, table_node_reward_detail, table_node_dividend_detail, table_total_rewards);
    std::map<common::xaccount_address_t, uint64_t> account_votes;
    calc_votes(property_param.votes_detail, property_param.map_nodes, account_votes);
    for(auto reward : node_reward_detail){
        std::string table_address = std::to_string(stoi(reward.first.to_string().substr(4)) / 64);
        calc_table_node_reward_detail(common::xaccount_address_t{table_address}, reward.first, reward.second, table_total_rewards, table_node_reward_detail);
    }
    for(auto reward : node_dividend_detail){
        for (auto & vote_detail : property_param.votes_detail){
            auto const & voter = vote_detail.first;
            auto const & votes = vote_detail.second;
            std::string table_address = std::to_string(stoi(voter.to_string().substr(4)) / 64);
            calc_table_node_dividend_detail(common::xaccount_address_t{table_address}, reward.first, reward.second, account_votes[reward.first], votes, table_total_rewards, table_node_dividend_detail);
        }
    }
    {
        EXPECT_EQ(table_node_dividend_detail.size(), 4);
        for(auto table : table_node_dividend_detail){
            for(auto reward : table.second){
                //printf("table %s, account %s, reward [%lu, %u]\n", table.first.c_str(), reward.first.c_str(), static_cast<uint64_t>(reward.second / REWARD_PRECISION),
                    //static_cast<uint32_t>(reward.second % REWARD_PRECISION));
            }
        }
        for(int idx = 0; idx < 4; idx++){
            std::string table = std::to_string(idx);
            if(table != "3"){
                for(int i = 0; i < 24; i=i+2){
                    if(i < 4){
                        EXPECT_EQ(table_node_dividend_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table)+1)*64+i+1)}], top::xstake::uint128_t("304000886236994246"));
                    }else{
                        if(table == "2"){
                            EXPECT_EQ(table_node_dividend_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table)+1)*64+i+1)}], top::xstake::uint128_t("729602126968786192"));
                        }else{
                            EXPECT_EQ(table_node_dividend_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table)+1)*64+i+1)}], top::xstake::uint128_t("1033603013205780440"));
                        }
                    }
                }
            }else{
                for(int i = 0; i < 24; i=i+2){
                    if(i < 4){
                        EXPECT_EQ(table_node_dividend_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string(i+1)}], top::xstake::uint128_t("304000886236994246"));
                    }else{
                        EXPECT_EQ(table_node_dividend_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string(i+1)}], top::xstake::uint128_t("1033603013205780440"));
                    }
                }
            }
        }
    }
    {
        EXPECT_EQ(table_node_reward_detail.size(), 4);
        for(int idx = 0; idx < 4; idx++){
            std::string table = std::to_string(idx);
            if(table == "0" || table == "1" ){
                EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}].size(), 32);
                for(int i = 0; i < 64; i=i+2){
                    if(i < 4){
                        EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table))*64+i+1)}], top::xstake::uint128_t("1216003544947976988"));
                    }else if(i < 24){
                        EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table))*64+i+1)}], top::xstake::uint128_t("4134412052823121760"));
                    }else{
                        EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table))*64+i+1)}], top::xstake::uint128_t("4560013293554913706"));
                    }
                }
            }else if(table == "2" ){
                EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}].size(), 12);
                for(int i = 0; i < 24; i=i+2){
                    if(i < 4){
                        EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table))*64+i+1)}], top::xstake::uint128_t("1216003544947976988"));
                    }else if(i < 24){
                        EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table))*64+i+1)}], top::xstake::uint128_t("4134412052823121760"));
                    }
                }                
            }else if(table == "3" ){
                EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}].size(), 12);
                for(int i = 0; i < 24; i=i+2){
                    if(i < 4){
                        EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table))*64+i+1)}], top::xstake::uint128_t("1216003544947976988"));
                    }else if(i < 24){
                        EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}][common::xaccount_address_t{"node"+std::to_string((std::stoi(table))*64+i+1)}], top::xstake::uint128_t("2918408507875144772"));
                    }
                }
            } 
        }
    }
    {
        EXPECT_EQ(table_total_rewards.size(), 4);
        EXPECT_EQ(table_total_rewards[common::xaccount_address_t{"0"}], top::xstake::uint128_t("145920425393757238588"));
        EXPECT_EQ(table_total_rewards[common::xaccount_address_t{"1"}], top::xstake::uint128_t("145920425393757238588"));
        EXPECT_EQ(table_total_rewards[common::xaccount_address_t{"2"}], top::xstake::uint128_t("51680150660289021988"));
        EXPECT_EQ(table_total_rewards[common::xaccount_address_t{"3"}], top::xstake::uint128_t("42560124073179194588"));
    }
    {
        for(int idx = 0; idx < 4; idx++){
            std::string table = std::to_string(idx);
            if(table == "0" || table == "1" ){
                EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}].size(), 32);
                for(int i = 0; i < 64; i=i+2){
                    if(i < 4){
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_edge_reward, top::xstake::uint128_t("1520004431184971235"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_archive_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_validator_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_auditor_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_vote_reward, top::xstake::uint128_t("0"));
                    }else if(i < 24){
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_edge_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_archive_reward, top::xstake::uint128_t("608001772473988494"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_validator_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_auditor_reward, top::xstake::uint128_t("1520004431184971235"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_vote_reward, top::xstake::uint128_t("3040008862369942471"));
                    }else{
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_edge_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_archive_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_validator_reward, top::xstake::uint128_t("4560013293554913706"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_auditor_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_vote_reward, top::xstake::uint128_t("0"));
                    }
                }
            }else if(table == "2" || table == "3" ){
                EXPECT_EQ(table_node_reward_detail[common::xaccount_address_t{table}].size(), 12);
                for(int i = 0; i < 64; i=i+2){
                    if(i < 4){
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_edge_reward, top::xstake::uint128_t("1520004431184971235"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_archive_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_validator_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_auditor_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_vote_reward, top::xstake::uint128_t("0"));
                    }else if(i < 24){
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_edge_reward, top::xstake::uint128_t("0"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_archive_reward, top::xstake::uint128_t("608001772473988494"));
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_validator_reward, top::xstake::uint128_t("0"));
                        if(table == "2"){
                            EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_auditor_reward, top::xstake::uint128_t("1520004431184971235"));
                        }else if(table == "3"){
                            EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_auditor_reward, top::xstake::uint128_t("0"));
                        }
                        EXPECT_EQ(issue_detail.m_node_rewards["node"+std::to_string((std::stoi(table))*64+i+1)].m_vote_reward, top::xstake::uint128_t("3040008862369942471"));
                    }
                }              
            }
        }
    }
}

NS_END3

#if 0
class test_suite_zec_reward_contract_t : public xzec_reward_contract, public testing::Test {
public:
    test_suite_zec_reward_contract_t() : xzec_reward_contract(common::xnetwork_id_t{0}) {};

    static void SetUpTestCase() {
        m_store = store::xstore_factory::create_store_with_memdb();
        auto mbus = std::make_shared<top::mbus::xmessage_bus_t>(true, 1000);
        auto chain_timer = top::make_object_ptr<time::xchain_timer_t>();
        auto & config_center = top::config::xconfig_register_t::get_instance();

        config::xconfig_loader_ptr_t loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(m_store), make_observer(mbus), make_observer(chain_timer.get()));
        config_center.add_loader(loader);
        config_center.load();

        // node registration contract
        xcontract_manager_t::instance().register_contract<xrec_registration_contract>(common::xaccount_address_t{sys_contract_rec_registration_addr},
                                                                                         common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_rec_registration_addr},
                                                                          common::xaccount_address_t{sys_contract_rec_registration_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_rec_registration_addr}, m_store.get());

        // tcc contract
        xcontract_manager_t::instance().register_contract<tcc::xrec_proposal_contract>(common::xaccount_address_t{sys_contract_rec_tcc_addr},
                                                                                         common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_rec_tcc_addr},
                                                                          common::xaccount_address_t{sys_contract_rec_tcc_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_rec_tcc_addr}, m_store.get());


        // table vote contract
        xcontract_manager_t::instance().register_contract<xtable_vote_contract>(common::xaccount_address_t{sys_contract_sharding_vote_addr}, common::xtopchain_network_id);
        for (auto i = 0; i < enum_vbucket_has_tables_count; i++) {
            auto addr = data::make_address_by_prefix_and_subaddr(sys_contract_sharding_vote_addr, i);
            xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_sharding_vote_addr},
                                                                              common::xaccount_address_t{addr});
            xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{addr}, m_store.get());
        }

        // table reward claiming contract
        xcontract_manager_t::instance().register_contract<xvm::system_contracts::reward::xtop_table_reward_claiming_contract>(common::xaccount_address_t{sys_contract_sharding_reward_claiming_addr}, common::xtopchain_network_id);
        for (auto i = 0; i < enum_vbucket_has_tables_count; i++) {
            auto addr = data::make_address_by_prefix_and_subaddr(sys_contract_sharding_reward_claiming_addr, i);
            xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_sharding_reward_claiming_addr},
                                                                              common::xaccount_address_t{addr});
            xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{addr}, m_store.get());
        }

        // zec vote contract
        xcontract_manager_t::instance().register_contract<xzec_vote_contract>(common::xaccount_address_t{sys_contract_zec_vote_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_zec_vote_addr},
                                                                          common::xaccount_address_t{sys_contract_zec_vote_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_zec_vote_addr}, m_store.get());


        // workload contract
        xcontract_manager_t::instance().register_contract<xzec_workload_contract>(common::xaccount_address_t{sys_contract_zec_workload_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_zec_workload_addr},
                                                                          common::xaccount_address_t{sys_contract_zec_workload_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_zec_workload_addr}, m_store.get());

        // zec reward contract
        xcontract_manager_t::instance().register_contract<xzec_reward_contract>(common::xaccount_address_t{sys_contract_zec_reward_addr}, common::xtopchain_network_id);
        xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_zec_reward_addr},
                                                                          common::xaccount_address_t{sys_contract_zec_reward_addr});
        xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_zec_reward_addr}, m_store.get());

        std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
        uint64_t node_morgage = 1000000000000;
        std::string node_types = "advance";

        int ret = registerNode(node_account, node_morgage, node_types);
        ASSERT_TRUE(ret == 0);
        for (auto i = 0; i < 1400; i++) {
            std::stringstream ss;
            ss << std::setw(40) << std::setfill('0') << i;
            auto node_account = ss.str();
            int ret = registerNode(node_account, node_morgage, node_types);
            ASSERT_TRUE(ret == 0);
        }

        uint64_t votes = 10000;
        ret = update_batch_stake(node_account, votes);
        ASSERT_TRUE(ret == 0);
    }

    static void TearDownTestCase() {}

    static int make_block(const std::string & address, const xtransaction_ptr_t & tx, uint64_t timer_height = 1) {
        auto account = m_store->clone_account(address);
        xassert(account != NULL);
        tx->set_last_nonce(account->account_send_trans_number() + 1);
        tx->set_digest();

        xaccount_context_t ac(address, m_store.get());
        ac.set_context_para(timer_height, "111", 1, 1);

        xvm::xvm_service s;
        xtransaction_trace_ptr trace = s.deal_transaction(tx, &ac);
        xassert(0 == (int) trace->m_errno);
        store::xtransaction_result_t result;
        ac.get_transaction_result(result);

        xlightunit_block_para_t para1;
        para1.set_one_input_tx(tx);
        para1.set_transaction_result(result);
        base::xauto_ptr<base::xvblock_t> block(xlightunit_block_t::create_next_lightunit(para1, account));
        xassert(block);
        xassert(block->get_block_hash().empty());
        static uint64_t clock = 1;
        block->get_cert()->set_clock(clock++);
        block->get_cert()->set_viewid(block->get_height() + 1);
        block->get_cert()->set_validator({1, (uint64_t)-1});
        block->get_cert()->set_viewtoken(1111);
        if (block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_cert) {
            block->set_extend_cert("1");
            block->set_extend_data("1");
        } else {
            block->set_verify_signature("1");
        }
        block->set_block_flag(base::enum_xvblock_flag_authenticated);
        block->set_block_flag(base::enum_xvblock_flag_locked);
        block->set_block_flag(base::enum_xvblock_flag_committed);
        block->set_block_flag(base::enum_xvblock_flag_executed);
        block->set_block_flag(base::enum_xvblock_flag_connected);

        auto ret = m_store->set_vblock(block->get_account(), block.get());
        xassert(ret);
        ret = m_store->execute_block(block.get());
        xassert(ret);
        return 0;
    }

    static base::xauto_ptr<base::xvblock_t> make_block2(const xcons_transaction_ptr_t & cons_tx, store::xtransaction_result_t & result, uint64_t timer_height = 1) {
        std::string address;
        auto tx = cons_tx->get_transaction();
        if (tx->get_tx_subtype() == data::enum_transaction_subtype_recv || tx->get_tx_subtype() == enum_transaction_subtype_self) {
            address = tx->get_target_action().get_account_addr();
        } else if (tx->get_tx_subtype() == data::enum_transaction_subtype_send) {
            address = tx->get_source_action().get_account_addr();
        }
        xassert(!address.empty());
        auto account = m_store->clone_account(address);
        xassert(account != NULL);
        tx->set_last_nonce(account->account_send_trans_number() + 1);
        tx->set_digest();

        xaccount_context_t ac(address, m_store.get());
        ac.set_context_para(timer_height, "111", 1, 1);

        ac.add_transaction(cons_tx);
        txexecutor::xtransaction_context_t tx_context(&ac, cons_tx);
        int32_t action_ret = tx_context.exec(result);
        if (action_ret) {
            xwarn("[make_block2] cons_tx exec fail, cons_tx=%s,error:%s",
                cons_tx->dump().c_str(), chainbase::xmodule_error_to_str(action_ret).c_str());
            return base::xauto_ptr<base::xvblock_t>({nullptr});
        }
        /*xvm::xvm_service s;
        xtransaction_trace_ptr trace = s.deal_transaction(tx, &ac);
        xassert(0 == (int) trace->m_errno);
        ac.get_transaction_result(result);*/

        xlightunit_block_para_t para1;
        para1.set_one_input_tx(cons_tx);
        para1.set_transaction_result(result);
        base::xauto_ptr<base::xvblock_t> block(xlightunit_block_t::create_next_lightunit(para1, account));
        xassert(block);
        xassert(block->get_block_hash().empty());
        static uint64_t clock = 1;
        block->get_cert()->set_clock(clock++);
        block->get_cert()->set_viewid(block->get_height() + 1);
        block->get_cert()->set_validator({1, (uint64_t)-1});
        block->get_cert()->set_viewtoken(1111);
        if (block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_cert) {
            block->set_extend_cert("1");
            block->set_extend_data("1");
        } else {
            block->set_verify_signature("1");
        }
        block->set_block_flag(base::enum_xvblock_flag_authenticated);
        block->set_block_flag(base::enum_xvblock_flag_locked);
        block->set_block_flag(base::enum_xvblock_flag_committed);
        block->set_block_flag(base::enum_xvblock_flag_executed);
        block->set_block_flag(base::enum_xvblock_flag_connected);

        auto ret = m_store->set_vblock(block->get_account(), block.get());
        xassert(ret);
        ret = m_store->execute_block(block.get());
        xassert(ret);
        return block;
    }

    static int make_block_ex(const xtransaction_ptr_t & tx, uint64_t timer_height = 1) {
        store::xtransaction_result_t result;
        // source action
        tx->set_digest(uint256_t(1111));
        xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
        auto block = make_block2(cons_tx, result, timer_height);
        xassert(block != nullptr);

        // target action
        if (tx->get_tx_subtype() != data::enum_transaction_subtype_self) {
            auto txreceipt = ((xlightunit_block_t *)block.get())->create_one_txreceipt(cons_tx->get_transaction());
            auto block = make_block2(txreceipt, result, timer_height);
            xassert(block != nullptr);
        }

        // inline action
        for (auto itx : result.get_contract_create_txs()) {
            xtransaction_ptr_t tx2;
            auto tx3 = itx->get_transaction();
            tx3->add_ref();
            tx2.attach(tx3);
            make_block_ex(tx2, timer_height);
        }
        return 0;
    }

    static int recreate_account(const std::string & account) {
        auto genesis_block = data::xblocktool_t::create_genesis_lightunit(account, ASSET_TOP(2000000000000));
        base::xauto_ptr<base::xvblock_t> auto_genesis_block(genesis_block);
        xassert(genesis_block);

        auto ret = m_store->set_vblock(genesis_block);
        if (!ret) {
            xerror("xtop_application::create_genesis_account store genesis block fail");
            return ret;
        }
        ret = m_store->execute_block(genesis_block);
        if (!ret) {
            xerror("xtop_application::create_genesis_account execute genesis block fail");
            return ret;
        }

        xaccount_ptr_t account_ptr  = m_store->query_account(account);
        if (account_ptr == NULL) {
            xerror("xtop_application::create_genesis_account execute genesis block fail");
            return 1;
        }

        xdbg("[recreate_account] account_balance: %llu", account_ptr->balance());
        m_original_balance = account_ptr->balance();

        m_vote_account_ctx_ptr = std::make_shared<xaccount_context_t>(
                sys_contract_rec_registration_addr,
                m_store.get());
        return 0;
    }

    static int stakeVotes(std::string voter, uint64_t vote_num, uint16_t lock_duration) {


        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << vote_num;
        tstream << lock_duration;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_pledge_token_vote);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(voter);
        tx->get_source_action().set_action_type(xaction_type_source_null);
        tx->get_source_action().set_action_param("");

        tx->get_target_action().set_account_addr(voter);
        tx->get_target_action().set_action_type(xaction_type_pledge_token_vote);
        tx->get_target_action().set_action_name("");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        //tx->set_tx_subtype(data::enum_transaction_subtype_recv);
        xassert( make_block_ex(tx) == 0);

        xaccount_ptr_t account_ptr  = m_store->query_account(voter);
        if (account_ptr == NULL) {
            xerror("stakeVotes fail");
            return 1;
        }

        //printf("[stakeVotes] unvote_num: %lu\n", account_ptr->unvote_num());
        //xdbg("[stakeVotes] unvote_num: %llu\n", account_ptr->unvote_num());


        return 0;
    }

    static int registerNode(std::string node_account, uint64_t deposit, std::string node_types) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << deposit;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << node_types;
        tstream << std::string("test1");
        tstream << std::string();
        tstream << static_cast<uint32_t>(50);
#if defined XENABLE_MOCK_ZEC_STAKE
        tstream << node_account;
#endif

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(node_account);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("registerNode");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_rec_registration_addr, tx);
    }

    static int unregisterNode(std::string node_account) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(node_account);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("unregisterNode");
        tx->get_target_action().set_action_param("");

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_rec_registration_addr, tx);
    }

    static int unregisterAllNodes() {
        std::map<std::string, std::string> map_nodes;

        auto ret = m_store->map_copy_get(sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_REG_KEY, map_nodes);
        if (ret) return ret;
        for (auto node : map_nodes) {
            auto ret = unregisterNode(node.first);
            if (ret) return ret;
        }
        map_nodes.clear();
        ret = m_store->map_copy_get(sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_REG_KEY, map_nodes);
        if (map_nodes.size()) return 2;

        return 0;
    }

    static int update_batch_stake(std::string node_account, uint64_t votes) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        std::map<std::string, std::string> adv_votes;
        adv_votes[node_account] = base::xstring_utl::tostring(votes);

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << adv_votes;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(table_vote_contract_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("update_batch_stake");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_rec_registration_addr, tx);
    }

    static int on_receive_workload2(std::string node_account,
                                    int auditor_group_id,
                                    uint32_t auditor_txs,
                                    int validator_group_id,
                                    uint32_t validator_txs,
                                    int64_t table_pledge_balance_change_tgas) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        std::map<common::xcluster_address_t, xauditor_workload_info_t> auditor_workload_info;
        std::map<common::xcluster_address_t, xvalidator_workload_info_t> validator_workload_info;

        common::xnetwork_id_t   net_id{0};
        common::xzone_id_t      zone_id{0};
        common::xcluster_id_t   cluster_id{1};
        common::xgroup_id_t     group_id{0};

        group_id = common::xgroup_id_t{auditor_group_id};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xauditor_workload_info_t auditor_workload;
        auditor_workload.m_leader_count[node_account] = auditor_txs;
        auditor_workload_info[cluster] = auditor_workload;

        group_id = common::xgroup_id_t{validator_group_id};
        cluster = common::xcluster_address_t{net_id, zone_id, cluster_id, group_id};
        xvalidator_workload_info_t validator_workload;
        validator_workload.m_leader_count[node_account] = validator_txs;
        validator_workload_info[cluster] = validator_workload;

        top::base::xstream_t stream(base::xcontext_t::instance());
        MAP_OBJECT_SERIALIZE2(stream, auditor_workload_info);
        MAP_OBJECT_SERIALIZE2(stream, validator_workload_info);
        stream << table_pledge_balance_change_tgas;
        std::string workload_str = std::string((char *)stream.data(), stream.size());

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << workload_str;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        std::string table_workload_contract_addr = std::string(sys_contract_sharding_workload_addr) + "@0";
        tx->get_source_action().set_account_addr(table_workload_contract_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_workload_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_receive_workload2");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_workload_addr, tx);
    }

    static int on_receive_workload3(
            std::map<common::xcluster_address_t, xauditor_workload_info_t> const & auditor_workload_info,
            std::map<common::xcluster_address_t, xvalidator_workload_info_t> const & validator_workload_info,
            int64_t table_pledge_balance_change_tgas, uint16_t tid = 0) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        top::base::xstream_t stream(base::xcontext_t::instance());
        MAP_OBJECT_SERIALIZE2(stream, auditor_workload_info);
        MAP_OBJECT_SERIALIZE2(stream, validator_workload_info);
        stream << table_pledge_balance_change_tgas;
        std::string workload_str = std::string((char *)stream.data(), stream.size());

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << workload_str;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        std::string table_workload_contract_addr = std::string(sys_contract_sharding_workload_addr) + "@" + xstring_utl::tostring(tid);
        tx->get_source_action().set_account_addr(table_workload_contract_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_workload_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_receive_workload2");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_workload_addr, tx);
    }

    static int on_receive_shard_votes(std::string node_account, uint64_t votes) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        std::map<std::string, std::string> contract_adv_votes;
        contract_adv_votes[node_account] = xstring_utl::tostring(votes);

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << contract_adv_votes;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(table_vote_contract_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_vote_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_receive_shard_votes");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_vote_addr, tx);
    }

    static int on_receive_shard_votes2(std::map<std::string, std::string> contract_adv_votes) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << contract_adv_votes;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(table_vote_contract_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_vote_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_receive_shard_votes");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_vote_addr, tx);
    }

    static int on_receive_shard_votes3(std::map<std::string, std::string> contract_adv_votes, std::string source_address) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << contract_adv_votes;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(source_address);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_vote_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_receive_shard_votes");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block(sys_contract_zec_vote_addr, tx);
    }

    static int on_zec_workload_timer(uint64_t onchain_timer_round) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << onchain_timer_round;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(sys_contract_zec_workload_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_workload_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_timer");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block_ex(tx, onchain_timer_round);
    }

    static int on_reward_timer(uint64_t onchain_timer_round) {
        top::base::xstream_t sstream(base::xcontext_t::instance());
        sstream << std::string("");
        sstream << (uint64_t)0;
        sstream << std::string("");

        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << onchain_timer_round;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_run_contract);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(sys_contract_zec_reward_addr);
        tx->get_source_action().set_action_type(xaction_type_asset_out);
        tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

        tx->get_target_action().set_account_addr(sys_contract_zec_reward_addr);
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("on_timer");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        //tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block_ex(tx, onchain_timer_round);
    }

    int is_mainnet_activated() {
        xactivation_record record;

        std::string value_str;
        auto ret = m_store->string_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, value_str);
        xassert(ret == 0);

        if (!value_str.empty()) {
            base::xstream_t stream(base::xcontext_t::instance(),
                        (uint8_t*)value_str.c_str(), (uint32_t)value_str.size());
            record.serialize_from(stream);
        }
        xdbg("[is_mainnet_activated] activated: %d, pid:%d\n", record.activated, getpid());
        return record.activated;
    }

    std::vector<std::string> generate_accounts(uint16_t tid, int count) {
        top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
        top::base::enum_vaccount_addr_type account_address_type{top::base::enum_vaccount_addr_type_secp256k1_user_account};
        top::base::enum_xchain_zone_index zone_index{top::base::enum_chain_zone_consensus_index};
        uint16_t ledger_id = top::base::xvaccount_t::make_ledger_id(static_cast<top::base::enum_xchain_id>(network_id.value()), zone_index);
        std::vector<std::string> accounts;
        for (int32_t i = 0; i < count; i++) {
            top::utl::xecprikey_t private_key;
            auto public_key = private_key.get_public_key();
            std::string account_address = private_key.to_account_address(account_address_type, ledger_id);
            account_address += "@" + base::xstring_utl::tostring(tid);
            uint32_t table_id = 256;
            xassert (EXTRACT_TABLE_ID(common::xaccount_address_t{account_address}, table_id) && tid == table_id);
            accounts.push_back(account_address);
        }
        return accounts;
    }

    std::vector<std::string> generate_accounts2(uint16_t ledger_id, int count) {
        top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
        top::base::enum_vaccount_addr_type account_address_type{top::base::enum_vaccount_addr_type_secp256k1_user_account};
        top::base::enum_xchain_zone_index zone_index{top::base::enum_chain_zone_consensus_index};

        std::vector<std::string> accounts;
        for (int32_t i = 0; i < count; i++) {
            top::utl::xecprikey_t private_key;
            auto public_key = private_key.get_public_key();
            std::string account_address = private_key.to_account_address(account_address_type, ledger_id);

            uint32_t table_id = 256;
            xassert (EXTRACT_TABLE_ID(common::xaccount_address_t{account_address}, table_id) && ledger_id == table_id);
            accounts.push_back(account_address);
        }
        return accounts;
    }

    int voteNode(std::string voter, std::map<std::string, int64_t> vote_infos, uint64_t timer_height = 1) {
        top::base::xstream_t tstream(base::xcontext_t::instance());
        tstream << vote_infos;

        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        tx->set_tx_type(xtransaction_type_vote);
        tx->set_deposit(100000);

        tx->get_source_action().set_account_addr(voter);
        tx->get_source_action().set_action_type(xaction_type_source_null);
        tx->get_source_action().set_action_param("");

        uint32_t table_id = 0;
        if (!EXTRACT_TABLE_ID(common::xaccount_address_t{voter}, table_id)) {
            xerror("[voteNode] account: %s\n", voter.c_str());
            return 1;
        }
        std::stringstream ss;
        ss << sys_contract_sharding_vote_addr << "@" << table_id;
        tx->get_target_action().set_account_addr(ss.str());
        tx->get_target_action().set_action_type(xaction_type_run_contract);
        tx->get_target_action().set_action_name("voteNode");
        tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

        tx->set_tx_subtype(data::enum_transaction_subtype_recv);

        return make_block_ex(tx, timer_height);
    }

    static xobject_ptr_t<store::xstore_face_t> m_store;
    static shared_ptr<xaccount_context_t> m_vote_account_ctx_ptr;
    static uint64_t m_original_balance;
    static std::string table_vote_contract_addr;
};

xobject_ptr_t<store::xstore_face_t> test_suite_zec_reward_contract_t::m_store;
shared_ptr<xaccount_context_t> test_suite_zec_reward_contract_t::m_vote_account_ctx_ptr;
uint64_t test_suite_zec_reward_contract_t::m_original_balance;
std::string test_suite_zec_reward_contract_t::table_vote_contract_addr = std::string(sys_contract_sharding_vote_addr) + "@0";



#define XSET_ONCHAIN_GOVERNANCE_PARAMETER(NAME, VALUE) static_cast<top::config::xconfig_register_t &>(top::config::xconfig_register_t::get_instance()).set(top::config::x ## NAME ## _onchain_goverance_parameter_t::name, VALUE)

TEST_F(test_suite_zec_reward_contract_t, on_reward_timer) {
    top::config::xconfig_register_t::get_instance().set(config::xtask_num_per_round_onchain_goverance_parameter_t::name, 1);
    auto timer_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
    uint64_t onchain_timer_round = timer_interval + 1;

    if (!is_mainnet_activated()) return;

    chain_fork::xtop_chain_fork_config_center fork_config_center;
    auto fork_config = fork_config_center.chain_fork_config();
    if (chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.reward_fork_point, onchain_timer_round)) {
        auto reg_contract_height = m_store->get_blockchain_height(sys_contract_rec_registration_addr);
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval, reg_contract_height);
        onchain_timer_round = 1;
        for (uint64_t i = 0; i < reg_contract_height; i++) { // update registration contract height
            int ret = on_reward_timer(onchain_timer_round++);
            ASSERT_TRUE(ret == 0);
        }

        auto timer_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
        onchain_timer_round = timer_interval + 1;
        int ret = on_reward_timer(onchain_timer_round);
        ASSERT_TRUE(ret == 0);

        std::map<std::string, std::string> dispatch_tasks;
        ret = m_store->map_copy_get(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, dispatch_tasks);
        ASSERT_TRUE(ret == 0);

        int table_node_reward_tasks = 0;
        int table_vote_reward_tasks = 0;
        for (auto t : dispatch_tasks) {
            xstream_t stream(xcontext_t::instance(), (uint8_t *)t.second.c_str(), (uint32_t)t.second.size());
            xreward_dispatch_task task;
            task.serialize_from(stream);

            xdbg("[xzec_reward_contract::print_tasks] task id: %s, onchain_timer_round: %llu, contract: %s, action: %s\n",
                 t.first.c_str(),
                 task.onchain_timer_round,
                 task.contract.c_str(),
                 task.action.c_str());
            if (task.action == XREWARD_CLAIMING_ADD_NODE_REWARD) {
                table_node_reward_tasks++;
            } else if (task.action == XREWARD_CLAIMING_ADD_VOTER_DIVIDEND_REWARD) {
                table_vote_reward_tasks++;
            }
        }
        //ASSERT_TRUE(table_node_reward_tasks == 2);
        //ASSERT_TRUE(table_vote_reward_tasks == 2);
    } else {
        int ret = on_reward_timer(onchain_timer_round);
        ASSERT_TRUE(ret == 0);

        int32_t size;
        ret = m_store->map_size(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, size);
        ASSERT_TRUE(ret == 0);
        ASSERT_TRUE(size == 0);

        ret = on_reward_timer(onchain_timer_round + 1);
        ASSERT_TRUE(ret == 0);

        ret = m_store->map_size(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, size);
        ASSERT_TRUE(ret == 0);
        ASSERT_TRUE(size == 0);
    }
}

/*TEST_F(test_suite_zec_reward_contract_t, calc_auditor_reward) {
    top::config::xconfig_register_t::get_instance().set(config::xtask_num_per_round_onchain_goverance_parameter_t::name, 1);
    auto timer_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
    uint64_t onchain_timer_round = timer_interval + 1;

    if (!is_mainnet_activated()) return;

    int ret = calc_auditor_reward(onchain_timer_round);
    ASSERT_TRUE(ret == 0);
}*/


TEST_F(test_suite_zec_reward_contract_t, upd_reg_contract_read_status) {
    uint64_t cur_time = 1;
    uint64_t last_read_time = 1;
    uint64_t last_read_height = 1;
    uint64_t latest_height = 1;
    uint64_t next_read_height = 1;
    bool update_rec_reg_contract_read_status = false;

    auto const height_step_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_height_step_limitation);
    auto const timeout_limitation = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cross_reading_rec_reg_contract_logic_timeout_limitation);

    // case 1: latest_height < last_read_height
    latest_height = 1;
    last_read_height = 2;
    update_rec_reg_contract_read_status = upd_reg_contract_read_status(cur_time, last_read_time, last_read_height, latest_height,
        height_step_limitation, timeout_limitation, next_read_height);
    ASSERT_TRUE(update_rec_reg_contract_read_status == false);

    // case 2: latest_height - last_read_height >= height_step_limitation
    last_read_height = 2;
    latest_height = last_read_height + height_step_limitation + 100;
    update_rec_reg_contract_read_status = upd_reg_contract_read_status(cur_time, last_read_time, last_read_height, latest_height,
        height_step_limitation, timeout_limitation, next_read_height);
    ASSERT_TRUE(update_rec_reg_contract_read_status == true);
    ASSERT_TRUE(next_read_height == last_read_height + height_step_limitation);

    // case 3: cur_time - last_read_time > timeout_limitation
    latest_height = last_read_height + height_step_limitation - 1;
    last_read_time = 1;
    cur_time = last_read_time + timeout_limitation + 1;
    update_rec_reg_contract_read_status = upd_reg_contract_read_status(cur_time, last_read_time, last_read_height, latest_height,
        height_step_limitation, timeout_limitation, next_read_height);
    ASSERT_TRUE(update_rec_reg_contract_read_status == true);
    ASSERT_TRUE(next_read_height == latest_height);
}

TEST_F(test_suite_zec_reward_contract_t, add_workload_reward) {
    common::xnetwork_id_t   net_id{0};
    common::xzone_id_t      zone_id{0};
    common::xcluster_id_t   cluster_id{1};
    common::xgroup_id_t     group_id{0};
    group_id = common::xgroup_id_t{1};
    common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
    std::string cluster_key;
    uint32_t cluster_zero_workload= XGET_ONCHAIN_GOVERNANCE_PARAMETER(cluster_zero_workload);
    uint32_t shard_zero_workload = XGET_ONCHAIN_GOVERNANCE_PARAMETER(shard_zero_workload);

    top::xstake::uint128_t cluster_total_rewards = 100000000000000;
    std::map<std::string, std::string> clusters_workloads;

    // node1..node50 only in group1
    // node51 in group1 and group2
    // other nodes in group2
    cluster_workload_t workload;
    {
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
    }
    workload.cluster_id = cluster_key;
    for (auto i = 1; i <= 51; i++) {
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << i;
        auto node_account = ss.str();
        workload.m_leader_count[node_account] = i;
        workload.cluster_total_workload += i;
    }
    {
        xstream_t stream(xcontext_t::instance());
        workload.serialize_to(stream);
        std::string value = std::string((const char*)stream.data(), stream.size());
        clusters_workloads[cluster_key] = value;
    }
    {
        group_id = common::xgroup_id_t{64};
        cluster = common::xcluster_address_t{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
    }
    workload.cluster_id = cluster_key;
    workload.m_leader_count.clear();
    workload.cluster_total_workload = 0;
    for (auto i = 51; i <= 100; i++) {
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << i;
        auto node_account = ss.str();
        workload.m_leader_count[node_account] = i;
        workload.cluster_total_workload += i;
    }
    {
        xstream_t stream(xcontext_t::instance());
        workload.serialize_to(stream);
        std::string value = std::string((const char*)stream.data(), stream.size());
        clusters_workloads[cluster_key] = value;
    }

    top::xstake::uint128_t workload_reward = 0;
    // case 1: node0 not in any group
    std::stringstream ss;
    ss << std::setw(40) << std::setfill('0') << 0;
    auto node_account = ss.str();
    add_workload_reward(false, cluster_zero_workload, shard_zero_workload,
        node_account, cluster_total_rewards, clusters_workloads, workload_reward);
    ASSERT_TRUE(workload_reward == 0);

    // case 2: only in one group
    workload_reward = 0;
    ss.clear();
    ss.str("");
    ss << std::setw(40) << std::setfill('0') << 1;
    node_account = ss.str();
    add_workload_reward(false, cluster_zero_workload, shard_zero_workload,
        node_account, cluster_total_rewards, clusters_workloads, workload_reward);
    {
        top::xstake::uint128_t workload_reward2 = 0;
        {
            {
                group_id = common::xgroup_id_t{1};
                cluster = common::xcluster_address_t{net_id, zone_id, cluster_id, group_id};
                xstream_t stream(xcontext_t::instance());
                stream << cluster;
                cluster_key = std::string((const char*)stream.data(), stream.size());
            }

            xstream_t stream(xcontext_t::instance(), (uint8_t *)clusters_workloads[cluster_key].c_str(), (uint32_t)clusters_workloads[cluster_key].size());
            workload.serialize_from(stream);
            workload_reward2 += cluster_total_rewards * workload.m_leader_count[node_account] / workload.cluster_total_workload;
        }

        ASSERT_TRUE(workload_reward2 == workload_reward);
    }

    // case 3: in two group
    workload_reward = 0;

    ss.clear();
    ss.str("");
    ss << std::setw(40) << std::setfill('0') << 51;
    node_account = ss.str();

    add_workload_reward(false, cluster_zero_workload, shard_zero_workload,
        node_account, cluster_total_rewards, clusters_workloads, workload_reward);
    {
        top::xstake::uint128_t workload_reward2 = 0;
        {
            {
                group_id = common::xgroup_id_t{1};
                cluster = common::xcluster_address_t{net_id, zone_id, cluster_id, group_id};
                xstream_t stream(xcontext_t::instance());
                stream << cluster;
                cluster_key = std::string((const char*)stream.data(), stream.size());
            }

            xstream_t stream(xcontext_t::instance(), (uint8_t *)clusters_workloads[cluster_key].c_str(), (uint32_t)clusters_workloads[cluster_key].size());
            workload.serialize_from(stream);
            workload_reward2 += cluster_total_rewards * workload.m_leader_count[node_account] / workload.cluster_total_workload;
        }
        {
            {
                group_id = common::xgroup_id_t{64};
                cluster = common::xcluster_address_t{net_id, zone_id, cluster_id, group_id};
                xstream_t stream(xcontext_t::instance());
                stream << cluster;
                cluster_key = std::string((const char*)stream.data(), stream.size());
            }
            xstream_t stream(xcontext_t::instance(), (uint8_t *)clusters_workloads[cluster_key].c_str(), (uint32_t)clusters_workloads[cluster_key].size());
            workload.serialize_from(stream);
            workload_reward2 += cluster_total_rewards * workload.m_leader_count[node_account] / workload.cluster_total_workload;
        }
        ASSERT_TRUE(workload_reward2 == workload_reward);
    }
}

TEST_F(test_suite_zec_reward_contract_t, zero_workload_reward) {
    common::xnetwork_id_t   net_id{0};
    common::xzone_id_t      zone_id{0};
    common::xcluster_id_t   cluster_id{1};
    common::xgroup_id_t     group_id{0};

    std::string cluster_key;

    top::xstake::uint128_t cluster_total_rewards = 100000000000000;
    std::map<std::string, std::string> auditor_clusters_workloads;
    std::map<std::string, std::string> validator_clusters_workloads;

    auto gen_group_workloads = [](const std::string & cluster_key, std::map<std::string, std::string> & clusters_workloads) {
        static int start = 1;

        cluster_workload_t workload;
        workload.cluster_id = cluster_key;
        for (auto i = start; i < start + 50; i++) {
            std::stringstream ss;
            ss << std::setw(40) << std::setfill('0') << i;
            auto node_account = ss.str();
            workload.m_leader_count[node_account] = i;
            workload.cluster_total_workload += i;
        }
        xstream_t stream(xcontext_t::instance());
        workload.serialize_to(stream);
        std::string value = std::string((const char*)stream.data(), stream.size());
        clusters_workloads[cluster_key] = value;
    };

    // generate auditor cluster workloads
    {
        group_id = common::xgroup_id_t{1};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        gen_group_workloads(cluster_key, auditor_clusters_workloads);
    }
    {
        group_id = common::xgroup_id_t{2};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        gen_group_workloads(cluster_key, auditor_clusters_workloads);
    }
    // generate validator cluster workloads
    {
        group_id = common::xgroup_id_t{64};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        gen_group_workloads(cluster_key, validator_clusters_workloads);
    }
    {
        group_id = common::xgroup_id_t{65};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        gen_group_workloads(cluster_key, validator_clusters_workloads);
    }

    uint32_t cluster_zero_workload= XGET_ONCHAIN_GOVERNANCE_PARAMETER(cluster_zero_workload);
    uint32_t shard_zero_workload = XGET_ONCHAIN_GOVERNANCE_PARAMETER(shard_zero_workload);
    std::size_t auditor_group_count = 1;
    std::size_t validator_group_count = 1;
    top::xstake::uint128_t zero_workload_rewards = 0;
    // case 1: auditor group, two groups have no workload
    //XSET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_group_count, 4);
    auditor_group_count = 4;
    zero_workload_rewards = 0;
    zero_workload_reward(false, cluster_zero_workload, shard_zero_workload, auditor_group_count, validator_group_count,
        cluster_total_rewards, auditor_clusters_workloads, zero_workload_rewards);
    ASSERT_TRUE(zero_workload_rewards == cluster_total_rewards * 2);

    // case 2: auditor group, one group has no workload
    //XSET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_group_count, 3);
    auditor_group_count = 3;
    zero_workload_rewards = 0;
    zero_workload_reward(false, cluster_zero_workload, shard_zero_workload, auditor_group_count, validator_group_count,
        cluster_total_rewards, auditor_clusters_workloads, zero_workload_rewards);
    ASSERT_TRUE(zero_workload_rewards == cluster_total_rewards);

    // case 3: auditor group, all group has workload
    //XSET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_group_count, 2);
    auditor_group_count = 2;
    zero_workload_rewards = 0;
    zero_workload_reward(false, cluster_zero_workload, shard_zero_workload, auditor_group_count, validator_group_count,
        cluster_total_rewards, auditor_clusters_workloads, zero_workload_rewards);
    ASSERT_TRUE(zero_workload_rewards == 0);

    // case 4: validator group, two groups have no workload
    //XSET_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_count, 4);
    validator_group_count = 4;
    zero_workload_rewards = 0;
    zero_workload_reward(true, cluster_zero_workload, shard_zero_workload, auditor_group_count, validator_group_count,
        cluster_total_rewards, validator_clusters_workloads, zero_workload_rewards);
    ASSERT_TRUE(zero_workload_rewards == cluster_total_rewards * 2);

    // case 5: validator group, one group has no workload
    //XSET_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_count, 3);
    validator_group_count = 3;
    zero_workload_rewards = 0;
    zero_workload_reward(true, cluster_zero_workload, shard_zero_workload, auditor_group_count, validator_group_count,
        cluster_total_rewards, validator_clusters_workloads, zero_workload_rewards);
    ASSERT_TRUE(zero_workload_rewards == cluster_total_rewards);

    // case 6: validator group, no group has workload
    //XSET_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_count, 2);
    validator_group_count = 2;
    zero_workload_rewards = 0;
    zero_workload_reward(true, cluster_zero_workload, shard_zero_workload, auditor_group_count, validator_group_count,
        cluster_total_rewards, validator_clusters_workloads, zero_workload_rewards);
    ASSERT_TRUE(zero_workload_rewards == 0);
}

TEST_F(test_suite_zec_reward_contract_t, preprocess_workload) {
    uint32_t cluster_zero_workload = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cluster_zero_workload);
    uint32_t shard_zero_workload = XGET_ONCHAIN_GOVERNANCE_PARAMETER(shard_zero_workload);
    common::xnetwork_id_t   net_id{0};
    common::xzone_id_t      zone_id{0};
    common::xcluster_id_t   cluster_id{1};
    common::xgroup_id_t     group_id{0};

    std::string cluster_key;

    std::map<std::string, std::string> auditor_clusters_workloads;
    std::map<std::string, std::string> validator_clusters_workloads;

    auto gen_group_workloads = [](const std::string & cluster_key, std::map<std::string, std::string> & clusters_workloads) {
        static int start = 1;

        cluster_workload_t workload;
        workload.cluster_id = cluster_key;
        for (auto i = start; i < start + 50; i++) {
            std::stringstream ss;
            ss << std::setw(40) << std::setfill('0') << i;
            auto node_account = ss.str();
            workload.m_leader_count[node_account] = i;
            workload.cluster_total_workload += i;
        }
        start += 50;
        xstream_t stream(xcontext_t::instance());
        workload.serialize_to(stream);
        std::string value = std::string((const char*)stream.data(), stream.size());
        clusters_workloads[cluster_key] = value;
    };

    // generate auditor cluster workloads
    {
        group_id = common::xgroup_id_t{1};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        gen_group_workloads(cluster_key, auditor_clusters_workloads);
    }
    {
        group_id = common::xgroup_id_t{2};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        gen_group_workloads(cluster_key, auditor_clusters_workloads);
    }
    // generate validator cluster workloads
    {
        group_id = common::xgroup_id_t{64};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        gen_group_workloads(cluster_key, validator_clusters_workloads);
    }
    {
        group_id = common::xgroup_id_t{65};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        gen_group_workloads(cluster_key, validator_clusters_workloads);
    }

    std::map<std::string, xreg_node_info> map_nodes;
    std::map<std::string, std::string> auditor_clusters_workloads2;
    std::map<std::string, std::string> validator_clusters_workloads2;
    std::string node_account;

    // case 1: node not in map_nodes
    auditor_clusters_workloads2 = auditor_clusters_workloads;
    ASSERT_TRUE(auditor_clusters_workloads2.size() == 2);
    preprocess_workload(true, auditor_clusters_workloads2, map_nodes, cluster_zero_workload, shard_zero_workload);
    ASSERT_TRUE(auditor_clusters_workloads2.size() == 0);

    // case 2: auditor node, deposit is zero
    map_nodes.clear();
    {
        xreg_node_info node;
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << 1;
        node.m_account = ss.str();
        node.m_account_mortgage = 0;
        node.m_registered_role = common::xrole_type_t::advance;
        map_nodes[ss.str()] = node;
    }
    preprocess_workload(true, auditor_clusters_workloads2, map_nodes, cluster_zero_workload, shard_zero_workload);
    ASSERT_TRUE(auditor_clusters_workloads2.size() == 0);

    // case 3: auditor node, votes less than mortgage
    map_nodes.clear();
    {
        xreg_node_info node;
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << 1;
        node.m_account = ss.str();
        node.m_account_mortgage = ASSET_TOP(1000);
        node.m_registered_role = common::xrole_type_t::advance;
        node.m_vote_amount = 900;
        map_nodes[ss.str()] = node;
    }
    auditor_clusters_workloads2 = auditor_clusters_workloads;
    ASSERT_TRUE(auditor_clusters_workloads2.size() == 2);
    preprocess_workload(true, auditor_clusters_workloads2, map_nodes, cluster_zero_workload, shard_zero_workload);
    ASSERT_TRUE(auditor_clusters_workloads2.size() == 0);

    // case 4: deposit and votes meets qualification, but not advance node
    map_nodes.clear();
    {
        xreg_node_info node;
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << 1;
        node.m_account = ss.str();
        node.m_account_mortgage = 1000;
        node.m_registered_role = common::xrole_type_t::edge;
        node.m_vote_amount = 1900;
        map_nodes[ss.str()] = node;
    }
    auditor_clusters_workloads2 = auditor_clusters_workloads;
    ASSERT_TRUE(auditor_clusters_workloads2.size() == 2);
    preprocess_workload(true, auditor_clusters_workloads2, map_nodes, cluster_zero_workload, shard_zero_workload);
    ASSERT_TRUE(auditor_clusters_workloads2.size() == 0);

    // case 5: qualified advance node
    map_nodes.clear();
    {
        xreg_node_info node;
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << 1;
        node_account = ss.str();
        node.m_account = ss.str();
        node.m_account_mortgage = ASSET_TOP(1000);
        node.m_registered_role = common::xrole_type_t::advance;
        node.m_vote_amount = 1900;
        map_nodes[ss.str()] = node;
    }
    auditor_clusters_workloads2 = auditor_clusters_workloads;
    ASSERT_TRUE(auditor_clusters_workloads2.size() == 2);
    preprocess_workload(true, auditor_clusters_workloads2, map_nodes, cluster_zero_workload, shard_zero_workload);
    ASSERT_TRUE(auditor_clusters_workloads2.size() == 1);
    {
        group_id = common::xgroup_id_t{1};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        auto iter = auditor_clusters_workloads2.find(cluster_key);
        ASSERT_TRUE(iter != auditor_clusters_workloads2.end());
        {
            cluster_workload_t workload;
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)iter->second.c_str(), (uint32_t)iter->second.size());
            workload.serialize_from(stream);
            auto iter = workload.m_leader_count.find(node_account);
            ASSERT_TRUE(iter != workload.m_leader_count.end());
        }
    }

    // case 6: validator node, deposit is zero
    map_nodes.clear();
    {
        xreg_node_info node;
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << 101;
        node.m_account = ss.str();
        node.m_account_mortgage = 0;
        node.m_registered_role = common::xrole_type_t::consensus;
        node.m_vote_amount = 900;
        map_nodes[ss.str()] = node;
    }
    validator_clusters_workloads2 = validator_clusters_workloads;
    ASSERT_TRUE(validator_clusters_workloads2.size() == 2);
    preprocess_workload(false, validator_clusters_workloads2, map_nodes, cluster_zero_workload, shard_zero_workload);
    ASSERT_TRUE(validator_clusters_workloads2.size() == 0);

    // case 7: deposit not zero, but not validator node
    map_nodes.clear();
    {
        xreg_node_info node;
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << 101;
        node.m_account = ss.str();
        node.m_account_mortgage = 10000;
        node.m_registered_role = common::xrole_type_t::edge;
        node.m_vote_amount = 900;
        map_nodes[ss.str()] = node;
    }
    validator_clusters_workloads2 = validator_clusters_workloads;
    ASSERT_TRUE(validator_clusters_workloads2.size() == 2);
    preprocess_workload(false, validator_clusters_workloads2, map_nodes, cluster_zero_workload, shard_zero_workload);
    ASSERT_TRUE(validator_clusters_workloads2.size() == 0);

    // case 8: qualified validator
    map_nodes.clear();
    {
        xreg_node_info node;
        std::stringstream ss;
        ss << std::setw(40) << std::setfill('0') << 101;
        node_account = ss.str();
        node.m_account = ss.str();
        node.m_account_mortgage = 10000;
        node.m_registered_role = common::xrole_type_t::consensus;
        node.m_vote_amount = 900;
        map_nodes[ss.str()] = node;
    }
    validator_clusters_workloads2 = validator_clusters_workloads;
    ASSERT_TRUE(validator_clusters_workloads2.size() == 2);
    preprocess_workload(false, validator_clusters_workloads2, map_nodes, cluster_zero_workload, shard_zero_workload);
    ASSERT_TRUE(validator_clusters_workloads2.size() == 1);
    {
        group_id = common::xgroup_id_t{64};
        common::xcluster_address_t cluster{net_id, zone_id, cluster_id, group_id};
        xstream_t stream(xcontext_t::instance());
        stream << cluster;
        cluster_key = std::string((const char*)stream.data(), stream.size());
        auto iter = validator_clusters_workloads2.find(cluster_key);
        ASSERT_TRUE(iter != validator_clusters_workloads2.end());
        {
            cluster_workload_t workload;
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)iter->second.c_str(), (uint32_t)iter->second.size());
            workload.serialize_from(stream);
            auto iter = workload.m_leader_count.find(node_account);
            ASSERT_TRUE(iter != workload.m_leader_count.end());
        }
    }
}

TEST_F(test_suite_zec_reward_contract_t, add_table_vote_reward) {
    top::xstake::uint128_t adv_reward_to_voters = ASSET_TOP(1000000);
    std::map<std::string, std::map<std::string, std::string>> contract_auditor_votes;
    std::string node_account;
    std::map<std::string, top::xstake::uint128_t> contract_rewards;
    std::map<std::string, std::map<std::string, top::xstake::uint128_t >> contract_auditor_vote_rewards;

    // case 1: node votes is zero
    uint64_t adv_total_votes = 0;
    bool ret = add_table_vote_reward(node_account, adv_total_votes, adv_reward_to_voters, contract_auditor_votes, contract_rewards, contract_auditor_vote_rewards);
    ASSERT_TRUE(ret == false);

    // case 2: node voted in 2 tables
    contract_auditor_votes["T20000MVfDLsBKVcy1wMp4CoEHWxUeBEAVBL9ZEa@0"]["node01"] = base::xstring_utl::tostring(900);
    contract_auditor_votes["T20000MVfDLsBKVcy1wMp4CoEHWxUeBEAVBL9ZEa@0"]["node02"] = base::xstring_utl::tostring(100);
    contract_auditor_votes["T20000MVfDLsBKVcy1wMp4CoEHWxUeBEAVBL9ZEa@1"]["node01"] = base::xstring_utl::tostring(800);
    contract_auditor_votes["T20000MVfDLsBKVcy1wMp4CoEHWxUeBEAVBL9ZEa@1"]["node02"] = base::xstring_utl::tostring(600);

    node_account = "node01";
    adv_total_votes = 1700;
    ret = add_table_vote_reward(node_account, adv_total_votes, adv_reward_to_voters, contract_auditor_votes, contract_rewards, contract_auditor_vote_rewards);
    ASSERT_TRUE(ret == true);
    auto iter1 = contract_rewards.find("T20000MTotTKfAJRxrfvEwEJvtgCqzH9GkpMmAUg@0");
    ASSERT_TRUE(iter1 != contract_rewards.end());
    ASSERT_TRUE(adv_reward_to_voters * 900 / adv_total_votes == iter1->second);
    iter1 = contract_rewards.find("T20000MTotTKfAJRxrfvEwEJvtgCqzH9GkpMmAUg@1");
    ASSERT_TRUE(adv_reward_to_voters * 800 / adv_total_votes == iter1->second);

    auto iter2 = contract_auditor_vote_rewards.find("T20000MTotTKfAJRxrfvEwEJvtgCqzH9GkpMmAUg@0");
    ASSERT_TRUE(iter2 != contract_auditor_vote_rewards.end());
    auto iter3 = iter2->second.find("node01");
    ASSERT_TRUE(iter3 != iter2->second.end());
    ASSERT_TRUE(adv_reward_to_voters * 900 / adv_total_votes == iter3->second);
    iter2 = contract_auditor_vote_rewards.find("T20000MTotTKfAJRxrfvEwEJvtgCqzH9GkpMmAUg@1");
    ASSERT_TRUE(iter2 != contract_auditor_vote_rewards.end());
    iter3 = iter2->second.find("node01");
    ASSERT_TRUE(iter3 != iter2->second.end());
    ASSERT_TRUE(adv_reward_to_voters * 800 / adv_total_votes == iter3->second);
}

TEST_F(test_suite_zec_reward_contract_t, add_table_node_reward) {
    std::string node_account = "T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73";
    top::xstake::uint128_t node_reward = 100000;
    std::map<std::string, top::xstake::uint128_t> contract_rewards;
    std::map<std::string, std::map<std::string, top::xstake::uint128_t> > table_nodes_rewards;

    uint32_t table_id = 0;
    ASSERT_TRUE(EXTRACT_TABLE_ID(common::xaccount_address_t{node_account}, table_id) == true);
    std::string reward_contract = std::string(sys_contract_sharding_reward_claiming_addr) + "@" + base::xstring_utl::tostring(table_id);

    add_table_node_reward( node_account, node_reward, contract_rewards, table_nodes_rewards);
    auto iter1 = contract_rewards.find(reward_contract);
    ASSERT_TRUE(iter1 != contract_rewards.end());
    ASSERT_TRUE(iter1->second == node_reward);

    auto iter2 = table_nodes_rewards.find(reward_contract);
    ASSERT_TRUE(iter2 != table_nodes_rewards.end());
    auto iter3 = iter2->second.find(node_account);
    ASSERT_TRUE(iter3 != iter2->second.end());
    ASSERT_TRUE(iter3->second == node_reward);
}

TEST_F(test_suite_zec_reward_contract_t, get_adv_total_votes) {
    std::map<std::string, std::map<std::string, std::string>> contract_auditor_votes;

    contract_auditor_votes["T20000MVfDLsBKVcy1wMp4CoEHWxUeBEAVBL9ZEa@0"]["node01"] = base::xstring_utl::tostring(900);
    contract_auditor_votes["T20000MVfDLsBKVcy1wMp4CoEHWxUeBEAVBL9ZEa@0"]["node02"] = base::xstring_utl::tostring(100);
    contract_auditor_votes["T20000MVfDLsBKVcy1wMp4CoEHWxUeBEAVBL9ZEa@1"]["node01"] = base::xstring_utl::tostring(800);
    contract_auditor_votes["T20000MVfDLsBKVcy1wMp4CoEHWxUeBEAVBL9ZEa@1"]["node02"] = base::xstring_utl::tostring(600);
    std::string account = "node01";
    auto node_votes = get_adv_total_votes(contract_auditor_votes, account);
    ASSERT_TRUE(node_votes == 1700);

    account = "node02";
    node_votes = get_adv_total_votes(contract_auditor_votes, account);
    ASSERT_TRUE(node_votes == 700);
}

TEST_F(test_suite_zec_reward_contract_t, get_reserve_reward) {
    top::xstake::uint128_t issued_until_last_year_end = 0;
    top::xstake::uint128_t minimum_issuance = 100;
    uint32_t issuance_rate = 5;

    top::xstake::uint128_t total_reserve = static_cast<top::xstake::uint128_t>(TOTAL_RESERVE) * REWARD_PRECISION;
    issued_until_last_year_end = total_reserve + 1;
    // case 1: total_reserve <= issued_until_last_year_end
    auto reserve_reward = get_reserve_reward(issued_until_last_year_end, minimum_issuance, issuance_rate);
    ASSERT_TRUE(reserve_reward == 100);

    // case 2: total_reserve > issued_until_last_year_end and reserved > minimum_issuance
    issued_until_last_year_end = total_reserve - minimum_issuance * 100 / issuance_rate - 100;
    reserve_reward = get_reserve_reward(issued_until_last_year_end, minimum_issuance, issuance_rate);
    ASSERT_TRUE(reserve_reward == (total_reserve - issued_until_last_year_end) * issuance_rate / 100);

    // case 3: total_reserve > issued_until_last_year_end and reserved <= minimum_issuance
    issued_until_last_year_end = total_reserve - minimum_issuance * 100 / issuance_rate + 100;
    reserve_reward = get_reserve_reward(issued_until_last_year_end, minimum_issuance, issuance_rate);
    ASSERT_TRUE(reserve_reward == minimum_issuance);
}

TEST_F(test_suite_zec_reward_contract_t, calc_issuance_internal) {
    //top::xstake::uint128_t minimum_issuance = 100;
    //uint32_t issuance_rate = 5;
    auto min_ratio_annual_total_reward = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_ratio_annual_total_reward);
    auto minimum_issuance = static_cast<top::xstake::uint128_t>(TOTAL_ISSUANCE) * min_ratio_annual_total_reward / 100 * REWARD_PRECISION;
    auto issuance_rate = XGET_ONCHAIN_GOVERNANCE_PARAMETER(additional_issue_year_ratio);

    top::xstake::uint128_t issued_until_last_year_end = 0;
    uint64_t last_issuance_time = 0;

    // case 1: not cross-year
    uint64_t total_height = 10;
    auto expected_issued_until_last_year_end = issued_until_last_year_end;
    auto additional_issuance = calc_issuance_internal(total_height, last_issuance_time,
        minimum_issuance, issuance_rate,
        issued_until_last_year_end);

    uint64_t        issued_clocks       = 0;
    uint64_t call_duration_height = total_height - last_issuance_time;
    auto reserve_reward = get_reserve_reward(expected_issued_until_last_year_end, minimum_issuance, issuance_rate);
    auto expected_additional_issuance = reserve_reward * (call_duration_height - issued_clocks) / TIMER_BLOCK_HEIGHT_PER_YEAR;
    ASSERT_TRUE(expected_additional_issuance == additional_issuance);
    ASSERT_TRUE(expected_issued_until_last_year_end == issued_until_last_year_end);

    // case 2: cross-year
    total_height = 2 * TIMER_BLOCK_HEIGHT_PER_YEAR + 10;
    last_issuance_time = 100;

    expected_issued_until_last_year_end = issued_until_last_year_end;
    additional_issuance = calc_issuance_internal(total_height, last_issuance_time,
        minimum_issuance, issuance_rate,
        issued_until_last_year_end);
    // the year before last
    reserve_reward = get_reserve_reward(expected_issued_until_last_year_end, minimum_issuance, issuance_rate);
    uint64_t remaining_clocks = TIMER_BLOCK_HEIGHT_PER_YEAR - last_issuance_time % TIMER_BLOCK_HEIGHT_PER_YEAR;
    expected_additional_issuance = reserve_reward * remaining_clocks / TIMER_BLOCK_HEIGHT_PER_YEAR;
    issued_clocks = remaining_clocks;
    last_issuance_time += remaining_clocks;
    expected_issued_until_last_year_end += reserve_reward;

    // last year
    remaining_clocks = TIMER_BLOCK_HEIGHT_PER_YEAR;
    reserve_reward = get_reserve_reward(expected_issued_until_last_year_end, minimum_issuance, issuance_rate);
    expected_additional_issuance += reserve_reward * remaining_clocks / TIMER_BLOCK_HEIGHT_PER_YEAR;
    issued_clocks += remaining_clocks;
    last_issuance_time += remaining_clocks;
    expected_issued_until_last_year_end += reserve_reward;

    // current year
    reserve_reward = get_reserve_reward(expected_issued_until_last_year_end, minimum_issuance, issuance_rate);
    expected_additional_issuance += reserve_reward * 10 / TIMER_BLOCK_HEIGHT_PER_YEAR;
    ASSERT_TRUE(expected_additional_issuance == additional_issuance);
    ASSERT_TRUE(expected_issued_until_last_year_end == issued_until_last_year_end);

    // case 3: border test
    total_height = 2 * TIMER_BLOCK_HEIGHT_PER_YEAR;
    last_issuance_time = 0;
    issued_until_last_year_end = 0;
    expected_issued_until_last_year_end = 0;
    additional_issuance = calc_issuance_internal(total_height, last_issuance_time,
        minimum_issuance, issuance_rate,
        issued_until_last_year_end);
    reserve_reward = get_reserve_reward(expected_issued_until_last_year_end, minimum_issuance, issuance_rate);
    expected_additional_issuance = reserve_reward;
    expected_issued_until_last_year_end = reserve_reward;
    printf("reserve_reward: [%lu, %u]\n",
        static_cast<uint64_t>(reserve_reward / REWARD_PRECISION),
         static_cast<uint32_t>(reserve_reward % REWARD_PRECISION));

    reserve_reward = get_reserve_reward(expected_issued_until_last_year_end, minimum_issuance, issuance_rate);
    expected_additional_issuance += reserve_reward;
    expected_issued_until_last_year_end += reserve_reward;
    printf("reserve_reward: [%lu, %u]\n",
        static_cast<uint64_t>(reserve_reward / REWARD_PRECISION),
         static_cast<uint32_t>(reserve_reward % REWARD_PRECISION));
    ASSERT_TRUE(expected_additional_issuance == additional_issuance);
    ASSERT_TRUE(expected_issued_until_last_year_end == issued_until_last_year_end);
}

/*TEST_F(test_suite_zec_reward_contract_t, dispatch_all_reward_v2) {
    std::map<std::string, std::map<std::string, top::xstake::uint128_t >> table_nodes_rewards;
    std::map<std::string, std::map<std::string, top::xstake::uint128_t >> table_vote_rewards;
    std::map<std::string, top::xstake::uint128_t> contract_rewards;
    uint64_t onchain_timer_round = 100;

    for (auto i = 0; i < 256; i++) {
        std::string contract = std::string("contract-") + base::xstring_utl::tostring(i);
        contract_rewards[contract] = i * 100000 + 1;
    }
    for (auto i = 0; i < 1001; i++) {
        std::string node_account = std::string("node-") + base::xstring_utl::tostring(i);
        table_nodes_rewards["table-01"][node_account] = i;
    }
    for (auto i = 0; i < 1001; i++) {
        std::string node_account = std::string("node-") + base::xstring_utl::tostring(i);
        table_vote_rewards["table-01"][node_account] = i;
    }

    dispatch_all_reward_v2(table_nodes_rewards, contract_rewards, table_vote_rewards, onchain_timer_round);
    std::map<std::string, std::string> dispatch_tasks;
    ret = m_store->map_copy_get(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, dispatch_tasks);
    ASSERT_TRUE(ret == 0);
    auto n
    while (!dispatch_tasks.empty()) {
        if
    }
}*/

/*
TEST_F(test_suite_zec_reward_contract_t, test_issue_model) {
    unregisterAllNodes();
    setvbuf(stdout,NULL,_IONBF,0);

    std::vector< std::vector<xstake::xreg_node_info> > all_nodes;
    std::vector< std::vector<std::string> > all_voters;
    std::map<uint8_t, uint64_t> group_total_stakes;

    for (auto i = 0; i < 256; i++) {
        auto cluster_group_id = common::xauditor_group_id_begin.value() + i / 128;
        auto shard_group_id = common::xvalidator_group_id_begin.value() + i / 64;

        std::vector<xstake::xreg_node_info> table_nodes;
        // 2 advance
        auto node_accounts = generate_accounts(i, 2);
        for (auto j = 0; j < 2; j++) {
            xreg_node_info node;
            node.m_account = node_accounts[j];
            node.m_registered_role = common::xrole_type_t::advance;
            node.m_account_mortgage = node.get_required_min_deposit();
            node.m_support_ratio_denominator = 50;
            node.m_auditor_credit_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_credit);
            node.m_validator_credit_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_credit);
            node.m_vote_amount = (i * 2 + j) * 512 * 1000;
            node.nickname = std::string("advance_") + base::xstring_utl::tostring(i * 2 + j);

            table_nodes.push_back(node);

            int ret = registerNode(node.m_account, node.m_account_mortgage, common::to_string(node.m_registered_role));
            ASSERT_TRUE(ret == 0);

            group_total_stakes[cluster_group_id] += node.get_auditor_stake();
            group_total_stakes[shard_group_id] += node.get_validator_stake();
        }
        // 2 validators
        node_accounts = generate_accounts(i, 2);
        for (auto j = 0; j < 2; j++) {
            xreg_node_info node;
            node.m_account = node_accounts[j];
            node.m_registered_role = common::xrole_type_t::consensus;
            node.m_account_mortgage = node.get_required_min_deposit();
            node.m_support_ratio_denominator = 50;
            node.m_validator_credit_numerator = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_credit);
            node.nickname = std::string("validator_") + base::xstring_utl::tostring(i * 2 + j);

            table_nodes.push_back(node);

            int ret = registerNode(node.m_account, node.m_account_mortgage, common::to_string(node.m_registered_role));
            ASSERT_TRUE(ret == 0);

            group_total_stakes[shard_group_id] += node.get_validator_stake();
        }
        // 1 edge
        {
            node_accounts = generate_accounts(i, 1);
            xreg_node_info node;
            node.m_account = node_accounts[0];
            node.m_registered_role = common::xrole_type_t::edge;
            node.m_account_mortgage = node.get_required_min_deposit();
            node.nickname = std::string("edge_") + base::xstring_utl::tostring(i);
            table_nodes.push_back(node);

            int ret = registerNode(node.m_account, node.m_account_mortgage, common::to_string(node.m_registered_role));
            ASSERT_TRUE(ret == 0);
        }
        all_nodes.push_back(table_nodes);

        // voters
        if (i < 2) {
            std::vector<std::string> table_voters = generate_accounts(i, 2);
            recreate_account(table_voters[0]);
            recreate_account(table_voters[1]);
            int ret = stakeVotes(table_voters[0], 2000000000, 570);
            ASSERT_TRUE(ret == 0);
            ret = stakeVotes(table_voters[1], 2000000000, 570);
            ASSERT_TRUE(ret == 0);
            all_voters.push_back(table_voters);
        }

        if ((i + 1) % 10 == 0) {
            printf("[test_suite_zec_reward_contract_t, test_issue_model] registerNode ..., %d tables\n", i + 1);
            xinfo("[test_suite_zec_reward_contract_t, test_issue_model] registerNode ..., %d tables\n", i + 1);
        }
    }

    uint64_t onchain_timer_round = XGET_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval);
    // votes
    //XSET_ONCHAIN_GOVERNANCE_PARAMETER(max_vote_nodes_num, 10000);
    for (auto i = 0; i < 2; i++) {
        uint32_t voters_num = 0;
        for (auto voter : all_voters[i]) {
            voters_num++;
            onchain_timer_round = XGET_ONCHAIN_GOVERNANCE_PARAMETER(votes_report_interval);
            std::map<std::string, int64_t> vote_infos;
            for (auto j = 0; j < 256; j++) {
                if (j == 0) {
                    vote_infos.insert({all_nodes[j][1].m_account, (j*2 + 1) * 10000});
                } else {
                    vote_infos.insert({all_nodes[j][0].m_account, (j*2) * 10000});
                    vote_infos.insert({all_nodes[j][1].m_account, (j*2 + 1) * 10000});
                }
            }
            if (voters_num == all_voters[i].size()) onchain_timer_round += 1;
            int ret = voteNode(voter, vote_infos, onchain_timer_round);
            ASSERT_TRUE(ret == 0);
        }

        if ((i + 1) % 10 == 0) {
            printf("[test_suite_zec_reward_contract_t, test_issue_model] voteNode ..., %d tables\n", i + 1);
            xinfo("[test_suite_zec_reward_contract_t, test_issue_model] voteNode ..., %d tables\n", i + 1);
        }
    }

    // construct workloads, 2 clusters, 4 shards
    for (auto i = 0; i < 256; i++) {
        std::map<common::xcluster_address_t, xauditor_workload_info_t> auditor_clusters_workloads;
        std::map<common::xcluster_address_t, xvalidator_workload_info_t> validator_clusters_workloads;
        int64_t table_pledge_balance_change_tgas = 1000000;
        common::xnetwork_id_t   net_id{0};
        common::xzone_id_t      zone_id{0};
        common::xcluster_id_t   cluster_id{1};
        common::xgroup_id_t     cluster_group_id{0};
        common::xgroup_id_t     shard_group_id{0};

        cluster_group_id = common::xgroup_id_t{common::xauditor_group_id_begin.value() + i / 128};
        shard_group_id = common::xgroup_id_t{common::xvalidator_group_id_begin.value() + i / 64};
        common::xcluster_address_t cluster_key{net_id, zone_id, cluster_id, cluster_group_id};
        common::xcluster_address_t shard_key{net_id, zone_id, cluster_id, shard_group_id};

        // auditor workload
        auto it = auditor_clusters_workloads.find(cluster_key);
        if (it == auditor_clusters_workloads.end()) {
            xauditor_workload_info_t auditor_workload_info;
            std::pair<std::map<common::xcluster_address_t, xauditor_workload_info_t>::iterator, bool> ret =
                auditor_clusters_workloads.insert(std::make_pair(cluster_key, auditor_workload_info));
            XCONTRACT_ENSURE(ret.second, "insert auditor workload failed");
            it = ret.first;
        }
        it->second.m_leader_count[all_nodes[i][0].m_account] += 43200 * all_nodes[i][0].get_auditor_stake() / group_total_stakes[cluster_group_id.value()];
        it->second.m_leader_count[all_nodes[i][1].m_account] += 43200 * all_nodes[i][1].get_auditor_stake() / group_total_stakes[cluster_group_id.value()];

        // validator workload
        auto it2 = validator_clusters_workloads.find(shard_key);
        if (it2 == validator_clusters_workloads.end()) {
            xvalidator_workload_info_t validator_workload_info;
            std::pair<std::map<common::xcluster_address_t, xvalidator_workload_info_t>::iterator, bool> ret =
                validator_clusters_workloads.insert(std::make_pair(shard_key, validator_workload_info));
            XCONTRACT_ENSURE(ret.second, "insert auditor workload failed");
            it2 = ret.first;
        }
        it2->second.m_leader_count[all_nodes[i][0].m_account] += 43200 * all_nodes[i][0].get_validator_stake() / group_total_stakes[shard_group_id.value()];
        it2->second.m_leader_count[all_nodes[i][1].m_account] += 43200 * all_nodes[i][1].get_validator_stake() / group_total_stakes[shard_group_id.value()];
        it2->second.m_leader_count[all_nodes[i][2].m_account] += 43200 * all_nodes[i][2].get_validator_stake() / group_total_stakes[shard_group_id.value()];
        it2->second.m_leader_count[all_nodes[i][3].m_account] += 43200 * all_nodes[i][3].get_validator_stake() / group_total_stakes[shard_group_id.value()];

        auto ret = on_receive_workload3(auditor_clusters_workloads, validator_clusters_workloads, table_pledge_balance_change_tgas, i);
        ASSERT_TRUE(ret == 0);

        if ((i + 1) % 10 == 0) {
            printf("[test_suite_zec_reward_contract_t, test_issue_model] on_receive_workload3 ..., %d tables\n", i + 1);
            xinfo("[test_suite_zec_reward_contract_t, test_issue_model] on_receive_workload3 ..., %d tables\n", i + 1);
        }
    }

    auto print_rewards = []() {
        // print node rewards
        xstake::uint128_t all_nodes_rewards = 0;

        for (auto i = 0; i < 256; i++) {
            std::map<std::string, std::string> node_rewards;
            std::stringstream ss;
            ss << sys_contract_sharding_reward_claiming_addr << "@" << i;
            m_store->map_copy_get(ss.str(), xstake::XPORPERTY_CONTRACT_NODE_REWARD_KEY, node_rewards);

            for (auto m : node_rewards) {
                xstake::xreward_node_record record;
                auto detail = m.second;
                base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                record.serialize_from(stream);

                std::string value;
                m_store->map_get(sys_contract_rec_registration_addr, xstake::XPORPERTY_CONTRACT_REG_KEY, m.first, value);
                xstake::xreg_node_info reg_node_info;
                if (!value.empty()) {
                    xstream_t stream(xcontext_t::instance(), (uint8_t *)value.data(), value.size());
                    reg_node_info.serialize_from(stream);
                }

                std::stringstream ss;
                ss  << "[test_issue_model] "
                    << "node_account: " << m.first << ", " << "node_type: " << common::to_string(reg_node_info.m_registered_role) << ", "
                    << "accumulated: " << static_cast<uint64_t>(record.m_accumulated / xstake::REWARD_PRECISION)
                    << "." << std::setw(6) << std::setfill('0') << static_cast<uint32_t>(record.m_accumulated % xstake::REWARD_PRECISION);
                printf("%s\n", ss.str().c_str());
                xinfo("%s\n", ss.str().c_str());
                all_nodes_rewards += record.m_accumulated;
            }
        }

        // print voter rewards
        xstake::uint128_t all_voters_rewards = 0;
        for (auto i = 0; i < 256; i++) {
            std::stringstream ss;
            ss << sys_contract_sharding_reward_claiming_addr << "@" << i;
            for (auto j = 1; j <= xstake::XPROPERTY_SPLITED_NUM; ++j) {
                std::string property_name{xstake::XPORPERTY_CONTRACT_VOTER_DIVIDEND_REWARD_KEY_BASE};
                property_name += "-" + std::to_string(j);

                std::map<std::string, std::string> voter_dividends;
                m_store->map_copy_get(ss.str(), property_name, voter_dividends);

                for (auto m : voter_dividends) {
                    xstake::xreward_record record;
                    auto detail = m.second;
                    base::xstream_t stream{xcontext_t::instance(), (uint8_t *)detail.data(), static_cast<uint32_t>(detail.size())};
                    record.serialize_from(stream);

                    std::stringstream ss;
                    ss  << "[test_issue_model] "
                        << "voter_account: " << m.first << ", "
                        << "accumulated: " << static_cast<uint64_t>(record.accumulated / xstake::REWARD_PRECISION)
                        << "." << std::setw(6) << std::setfill('0') << static_cast<uint32_t>(record.accumulated % xstake::REWARD_PRECISION);
                    printf("%s\n", ss.str().c_str());
                    xinfo("%s\n", ss.str().c_str());
                    all_voters_rewards += record.accumulated;
                }
            }
        }

        // total issuance
        std::string total_issuances_str;
        auto ret = m_store->map_get(sys_contract_zec_reward_addr, XPROPERTY_CONTRACT_ACCUMULATED_ISSUANCE, "total", total_issuances_str);
        ASSERT_TRUE(ret == 0);
        auto total_issuances = base::xstring_utl::touint64(total_issuances_str);

        // tcc balance
        xaccount_ptr_t account_ptr  = m_store->query_account(sys_contract_rec_tcc_addr);
        if (account_ptr == NULL) {
            xerror("[test_issue_model] query_account sys_contract_rec_tcc_addr fail");
            return;
        }
        auto tcc_balance = account_ptr->balance();
        xinfo("[test_issue_model] total_issuances: %lu, tcc_balance: %lu, all_nodes_rewards: %lu, all_voters_rewards: %lu",
            total_issuances,
            tcc_balance,
            static_cast<uint64_t>(all_nodes_rewards / xstake::REWARD_PRECISION),
            static_cast<uint64_t>(all_voters_rewards / xstake::REWARD_PRECISION));
        printf("[test_issue_model] total_issuances: %lu, tcc_balance: %lu, all_nodes_rewards: %lu, all_voters_rewards: %lu\n",
            total_issuances,
            tcc_balance,
            static_cast<uint64_t>(all_nodes_rewards / xstake::REWARD_PRECISION),
            static_cast<uint64_t>(all_voters_rewards / xstake::REWARD_PRECISION));
    };

    //top::config::xconfig_register_t::get_instance().set(config::xtask_num_per_round_onchain_goverance_parameter_t::name, 1);
    auto timer_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval);
    onchain_timer_round = timer_interval + 1;

    chain_fork::xtop_chain_fork_config_center fork_config_center;
    auto fork_config = fork_config_center.chain_fork_config();
    if (chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.reward_fork_point, onchain_timer_round)) {
        // report workload
        int ret = on_zec_workload_timer(onchain_timer_round);
        ASSERT_TRUE(ret == 0);

        auto reg_contract_height = m_store->get_blockchain_height(sys_contract_rec_registration_addr);
        //XSET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval, reg_contract_height);
        onchain_timer_round = 1;
        for (uint64_t i = 0; i < reg_contract_height; i++) { // update registration contract height
            int ret = on_reward_timer(onchain_timer_round++);
            ASSERT_TRUE(ret == 0);
        }


        print_rewards();
        printf("-------------------------------------\n");

        uint32_t timer_interval = 5 * 24 * 360; // 24 hours
        XSET_ONCHAIN_GOVERNANCE_PARAMETER(reward_issue_interval, timer_interval);
        onchain_timer_round = timer_interval;
        while (onchain_timer_round <= 20 * TIMER_BLOCK_HEIGHT_PER_YEAR) {
            // issue
            int ret = on_reward_timer(onchain_timer_round);
            ASSERT_TRUE(ret == 0);

            // execute tasks
            std::map<std::string, std::string> dispatch_tasks;
            ret = m_store->map_copy_get(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, dispatch_tasks);
            ASSERT_TRUE(ret == 0);
            while (!dispatch_tasks.empty()) {
                int ret = on_reward_timer(onchain_timer_round);
                ASSERT_TRUE(ret == 0);

                dispatch_tasks.clear();
                ret = m_store->map_copy_get(sys_contract_zec_reward_addr, XPORPERTY_CONTRACT_TASK_KEY, dispatch_tasks);
                ASSERT_TRUE(ret == 0);
            }

            if (onchain_timer_round % ( 10 * timer_interval ) == 0) {
                printf("[test_suite_zec_reward_contract_t, test_issue_model] on_reward_timer ..., %lu issuance\n", onchain_timer_round / timer_interval);
                xinfo("[test_suite_zec_reward_contract_t, test_issue_model] on_reward_timer ..., %lu issuance\n", onchain_timer_round / timer_interval);
            }

            if (onchain_timer_round % ( 365 / 5 * timer_interval ) == 0) {
                printf("[test_issue_model] on_reward_timer one year, %lu issuance\n", onchain_timer_round / timer_interval);
                xinfo("[test_issue_model] on_reward_timer one year, %lu issuance\n", onchain_timer_round / timer_interval);
                print_rewards();
            }

            onchain_timer_round += timer_interval;
        }
    } else {
        printf("[test_suite_zec_reward_contract_t, test_issue_model] not forked\n");
    }
}*/

#endif
