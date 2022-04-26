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
    r_v2.m_eth_reward = 7;

    data::system_contract::xissue_detail_v2 i_v2;
    i_v2.onchain_timer_round = 100;
    i_v2.m_zec_vote_contract_height = 1;
    i_v2.m_zec_workload_contract_height = 2;
    i_v2.m_zec_reward_contract_height = 3;
    i_v2.m_edge_reward_ratio = 4;
    i_v2.m_archive_reward_ratio = 5;
    i_v2.m_validator_reward_ratio = 6;
    i_v2.m_auditor_reward_ratio = 7;
    i_v2.m_eth_reward_ratio = 8;
    i_v2.m_vote_reward_ratio = 9;
    i_v2.m_governance_reward_ratio = 10;
    i_v2.m_auditor_group_count = 11;
    i_v2.m_validator_group_count = 12;
    i_v2.m_eth_group_count = 13;
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
    EXPECT_TRUE(i_v2_new.m_eth_reward_ratio == 0);
    EXPECT_TRUE(i_v2_new.m_vote_reward_ratio == 9);
    EXPECT_TRUE(i_v2_new.m_governance_reward_ratio == 10);
    EXPECT_TRUE(i_v2_new.m_auditor_group_count == 11);
    EXPECT_TRUE(i_v2_new.m_validator_group_count == 12);
    EXPECT_TRUE(i_v2_new.m_eth_group_count == 0);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_edge_reward == 1);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_archive_reward == 2);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_validator_reward == 3);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_auditor_reward == 4);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_vote_reward == 5);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_self_reward == 6);
    EXPECT_TRUE(i_v2_new.m_node_rewards["test"].m_eth_reward == 0);
}

TEST(data_structures_test, xunqualified_node_info) {
    data::system_contract::xunqualified_node_info_v2_t i_v2;
    data::system_contract::xnode_vote_percent_t a1;
    a1.block_count = 1;
    a1.subset_count = 2;
    data::system_contract::xnode_vote_percent_t a2;
    a2.block_count = 10;
    a2.subset_count = 20;
    i_v2.auditor_info[common::xnode_id_t{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"}] = a1;
    i_v2.auditor_info[common::xnode_id_t{"T00000LfxdAPxPUrbYvDCkpijvicSQCXTBT7J7WW"}] = a2;
    data::system_contract::xnode_vote_percent_t v1;
    v1.block_count = 2;
    v1.subset_count = 4;
    data::system_contract::xnode_vote_percent_t v2;
    v2.block_count = 20;
    v2.subset_count = 40;
    i_v2.validator_info[common::xnode_id_t{"T00000LhLPkC9q7BfcjPpwWcb4pgZ3fHqTxdUywi"}] = v1;
    i_v2.validator_info[common::xnode_id_t{"T00000LWfXZgVPa8mmuDhxQjAtSgVaB9exKsQBWE"}] = v2;
    data::system_contract::xnode_vote_percent_t e1;
    e1.block_count = 3;
    e1.subset_count = 6;
    data::system_contract::xnode_vote_percent_t e2;
    e2.block_count = 30;
    e2.subset_count = 60;
    i_v2.evm_info[common::xnode_id_t{"T00000LNomDyH8kN2zQhexGbg4dMJY1ZbAAekZ8Y"}] = e1;
    i_v2.evm_info[common::xnode_id_t{"T00000LcYvxYB3EAnPcBo9hPybrDM2ZB5v5JFcyg"}] = e2;

    base::xstream_t stream(base::xcontext_t::instance());
    auto i_v1 = static_cast<data::system_contract::xunqualified_node_info_v1_t>(i_v2);
    auto s = i_v1.serialize_to(stream);

    data::system_contract::xunqualified_node_info_v2_t i_v2_new;
    i_v2_new.serialize_from(stream);
    EXPECT_EQ(i_v2_new.auditor_info.size(), 2);
    EXPECT_EQ(i_v2_new.validator_info.size(), 2);
    EXPECT_EQ(i_v2_new.evm_info.size(), 0);
    EXPECT_TRUE(i_v2_new.auditor_info[common::xnode_id_t{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"}].block_count == 1);
    EXPECT_TRUE(i_v2_new.auditor_info[common::xnode_id_t{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"}].subset_count == 2);
    EXPECT_TRUE(i_v2_new.auditor_info[common::xnode_id_t{"T00000LfxdAPxPUrbYvDCkpijvicSQCXTBT7J7WW"}].block_count == 10);
    EXPECT_TRUE(i_v2_new.auditor_info[common::xnode_id_t{"T00000LfxdAPxPUrbYvDCkpijvicSQCXTBT7J7WW"}].subset_count == 20);
    EXPECT_TRUE(i_v2_new.validator_info[common::xnode_id_t{"T00000LhLPkC9q7BfcjPpwWcb4pgZ3fHqTxdUywi"}].block_count == 2);
    EXPECT_TRUE(i_v2_new.validator_info[common::xnode_id_t{"T00000LhLPkC9q7BfcjPpwWcb4pgZ3fHqTxdUywi"}].subset_count == 4);
    EXPECT_TRUE(i_v2_new.validator_info[common::xnode_id_t{"T00000LWfXZgVPa8mmuDhxQjAtSgVaB9exKsQBWE"}].block_count == 20);
    EXPECT_TRUE(i_v2_new.validator_info[common::xnode_id_t{"T00000LWfXZgVPa8mmuDhxQjAtSgVaB9exKsQBWE"}].subset_count == 40);
}
