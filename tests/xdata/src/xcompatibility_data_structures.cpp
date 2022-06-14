#include "gtest/gtest.h"

#include "xdata/xsystem_contract/xdata_structures.h"

using namespace top;

TEST(data_structures_test, xissue_detail) {
    data::system_contract::reward_detail_v2 r_v2;
    r_v2.m_edge_reward = 1;
    r_v2.m_archive_reward = 2;
    r_v2.m_validator_reward = 3;
    r_v2.m_auditor_reward = 4;
    r_v2.m_vote_reward = 5;
    r_v2.m_self_reward = 6;
    r_v2.m_evm_auditor_reward = 7;
    r_v2.m_evm_validator_reward = 8;

    data::system_contract::xissue_detail_v2 i_v2;
    i_v2.onchain_timer_round = 100;
    i_v2.m_zec_vote_contract_height = 1;
    i_v2.m_zec_workload_contract_height = 2;
    i_v2.m_zec_reward_contract_height = 3;
    i_v2.m_edge_reward_ratio = 4;
    i_v2.m_archive_reward_ratio = 5;
    i_v2.m_validator_reward_ratio = 6;
    i_v2.m_auditor_reward_ratio = 7;
    i_v2.m_evm_auditor_reward_ratio = 8;
    i_v2.m_vote_reward_ratio = 9;
    i_v2.m_governance_reward_ratio = 10;
    i_v2.m_auditor_group_count = 11;
    i_v2.m_validator_group_count = 12;
    i_v2.m_evm_auditor_group_count = 13;
    i_v2.m_node_rewards["test"] = r_v2;

    auto i_v1 = static_cast<data::system_contract::xissue_detail_v1>(i_v2);
    auto str = i_v1.to_string();

    data::system_contract::xissue_detail_v2 i_v2_new;
    i_v2_new.from_string(str);
    EXPECT_TRUE(i_v2_new.onchain_timer_round == 100);
    EXPECT_TRUE(i_v2_new.m_zec_vote_contract_height == 1);
    EXPECT_TRUE(i_v2_new.m_zec_workload_contract_height == 2);
    EXPECT_TRUE(i_v2_new.m_zec_reward_contract_height == 3);
    EXPECT_TRUE(i_v2_new.m_edge_reward_ratio == 4);
    EXPECT_TRUE(i_v2_new.m_archive_reward_ratio == 5);
    EXPECT_TRUE(i_v2_new.m_validator_reward_ratio == 6);
    EXPECT_TRUE(i_v2_new.m_auditor_reward_ratio == 7);
    EXPECT_TRUE(i_v2_new.m_evm_auditor_reward_ratio == 0);
    EXPECT_TRUE(i_v2_new.m_vote_reward_ratio == 9);
    EXPECT_TRUE(i_v2_new.m_governance_reward_ratio == 10);
    EXPECT_TRUE(i_v2_new.m_auditor_group_count == 11);
    EXPECT_TRUE(i_v2_new.m_validator_group_count == 12);
    EXPECT_TRUE(i_v2_new.m_evm_auditor_group_count == 0);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_edge_reward == 1);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_archive_reward == 2);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_validator_reward == 3);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_auditor_reward == 4);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_vote_reward == 5);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_self_reward == 6);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_evm_auditor_reward == 0);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_evm_validator_reward == 0);
}