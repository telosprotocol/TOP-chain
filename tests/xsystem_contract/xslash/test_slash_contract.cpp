// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <gtest/gtest.h>
#include <string>

#include "tests/xelection/xmocked_vnode_service.h"

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"
#include "xblockstore/xblockstore_face.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xblock_statistics_data.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xstake/xstake_algorithm.h"
#include "xvm/xsystem_contracts/xslash/xzec_slash_info_contract.h"


using namespace top;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::tests::election;
using namespace top::data;

const uint16_t ACCOUNT_ADDR_NUM = 10;

class test_slash_info_contract: public xzec_slash_info_contract, public testing::Test {
public:
    test_slash_info_contract(): xzec_slash_info_contract{common::xnetwork_id_t{0}}, node_serv{common::xaccount_address_t{"mocked_nodesvr"}, "null"}{};

    void SetUp(){
        create_account_addrs(ACCOUNT_ADDR_NUM);

        uint64_t elect_blk_height = 1;
        nodeservice_add_group(elect_blk_height, create_group_xip2(elect_blk_height, uint8_t(1), ACCOUNT_ADDR_NUM), account_addrs);
        nodeservice_add_group(elect_blk_height, create_group_xip2(elect_blk_height, uint8_t(64), ACCOUNT_ADDR_NUM), account_addrs);
    }
    void TearDown(){}



    void create_account_addrs(uint32_t account_num);
    common::xip2_t create_group_xip2(uint64_t elect_blk_height, uint8_t group_id, uint16_t group_size);
    void nodeservice_add_group(uint64_t elect_blk_height, common::xip2_t const& group_xip, std::vector<common::xaccount_address_t> const& nodes);
    void set_according_block_statistic_data(uint64_t elect_blk_height, std::vector<common::xip2_t> const& group_xip);


public:
    std::vector<common::xaccount_address_t> account_addrs;
    xstatistics_data_t data;
    xmocked_vnodesvr_t node_serv;
};


void test_slash_info_contract::create_account_addrs(uint32_t account_num) {
     account_addrs.resize(account_num);
     for (uint32_t i = 0; i < account_num; ++i) {
         account_addrs[i] = common::xaccount_address_t{std::string{"account_"} + std::to_string(i)};
     }
 }

common::xip2_t test_slash_info_contract::create_group_xip2(uint64_t elect_blk_height, uint8_t group_id, uint16_t group_size) {
     return common::xip2_t{
        common::xnetwork_id_t{0},
        common::xconsensus_zone_id,
        common::xdefault_cluster_id,
        common::xgroup_id_t{group_id},
        group_size,
        elect_blk_height
     };
 }

void test_slash_info_contract::nodeservice_add_group(uint64_t elect_blk_height, common::xip2_t const& group_xip, std::vector<common::xaccount_address_t> const& nodes) {
    node_serv.add_group(
        group_xip.network_id(),
        group_xip.zone_id(),
        group_xip.cluster_id(),
        group_xip.group_id(),
        (uint16_t)group_xip.size(),
        elect_blk_height
    );


    xmocked_vnode_group_t* node_group = dynamic_cast<xmocked_vnode_group_t*>(node_serv.get_group(group_xip).get());
    node_group->reset_nodes();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        node_group->add_node(nodes[i]);
    }


    assert(node_group->get_nodes().size() == nodes.size());
    // auto group_nodes = node_serv.get_group(group_xip)->get_nodes();
    // std::cout << "node size: " << group_nodes.size() << "\n";
    // for (std::size_t i = 0; i < group_nodes.size(); ++i) {
    //     std::cout << group_nodes[i]->get_account() << "\n";
    // }

}

void test_slash_info_contract::set_according_block_statistic_data(uint64_t elect_blk_height, std::vector<common::xip2_t> const& group_xips) {
    xelection_related_statistics_data_t elect_data;
    for (std::size_t i = 0; i < group_xips.size(); ++i) {
        auto vnode_group = node_serv.get_group(group_xips[i]);
        assert(vnode_group != nullptr);

        xgroup_related_statistics_data_t group_data;
        // set vote data
        for (std::size_t i = 0; i < vnode_group->get_size(); ++i) {
            xaccount_related_statistics_data_t account_data;
            account_data.vote_data.block_count = i;
            account_data.vote_data.vote_count = i;

            group_data.account_statistics_data.push_back(account_data);
        }

        common::xgroup_address_t  group_addr{group_xips[i].xip()};
        elect_data.group_statistics_data[group_addr] = group_data;
        // std::cout << "[test_slash_info_contract::set_according_block_statistic_data] " << common::xgroup_address_t{group_xips[i].xip()}.to_string() << "\n";
        // std::cout << "[test_slash_info_contract::set_according_block_statistic_data] " << std::hex <<(int)common::xgroup_address_t{group_xips[i].xip()}.type() << "\n";
    }

    data.detail[elect_blk_height] = elect_data;

}

TEST_F(test_slash_info_contract, test_statistic_data) {

    uint64_t elect_blk_height = 1;
    auto group_xip2 = create_group_xip2(elect_blk_height, 1, account_addrs.size());
    set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_xip2});

    for (std::size_t i = 0; i < account_addrs.size(); ++i) {
        EXPECT_EQ(data.detail[elect_blk_height].group_statistics_data[common::xgroup_address_t{group_xip2.xip()}].account_statistics_data[i].vote_data.block_count, i);
        EXPECT_EQ(data.detail[elect_blk_height].group_statistics_data[common::xgroup_address_t{group_xip2.xip()}].account_statistics_data[i].vote_data.vote_count, i);
    }
}

TEST_F(test_slash_info_contract, test_accumulate_node_info) {
    xunqualified_node_info_t origin_info;

    for (std::size_t i = 0; i < account_addrs.size(); ++i) {
        xnode_vote_percent_t node_vote;
        node_vote.subset_count = i;
        node_vote.block_count = i;
        origin_info.auditor_info[account_addrs[i]] = node_vote;
        origin_info.validator_info[account_addrs[i]] = node_vote;
    }

    xunqualified_node_info_t summarize_slash_info;
    accumulate_node_info(origin_info, summarize_slash_info);

    for (std::size_t i = 0; i < account_addrs.size(); ++i) {
        EXPECT_EQ(summarize_slash_info.auditor_info[account_addrs[i]].subset_count, i);
        EXPECT_EQ(summarize_slash_info.auditor_info[account_addrs[i]].block_count, i);
        EXPECT_EQ(summarize_slash_info.auditor_info[account_addrs[i]].subset_count, i);
        EXPECT_EQ(summarize_slash_info.validator_info[account_addrs[i]].block_count, i);
    }

    accumulate_node_info(origin_info, summarize_slash_info);
    for (std::size_t i = 0; i < account_addrs.size(); ++i) {
        EXPECT_EQ(summarize_slash_info.auditor_info[account_addrs[i]].subset_count, 2 * i);
        EXPECT_EQ(summarize_slash_info.auditor_info[account_addrs[i]].block_count, 2 * i);
        EXPECT_EQ(summarize_slash_info.auditor_info[account_addrs[i]].subset_count, 2 * i);
        EXPECT_EQ(summarize_slash_info.validator_info[account_addrs[i]].block_count, 2 * i);
    }
}


TEST_F(test_slash_info_contract, test_filter_node) {
    xunqualified_node_info_t origin_info;

    for (std::size_t i = 0; i < account_addrs.size(); ++i) {
        xnode_vote_percent_t node_vote;
        node_vote.block_count = i;
        node_vote.subset_count = 10;
        origin_info.auditor_info[account_addrs[i]] = node_vote;
        origin_info.validator_info[account_addrs[i]] = node_vote;
    }

    auto  action_node_info = filter_helper(origin_info, 0, 10, 30, 80);
    int slash_auditor_node_size = 0, award_auditor_node_size= 0;
    int slash_validator_node_size = 0, award_validator_node_size = 0;


    for (std::size_t i = 0; i < action_node_info.size(); ++i) {
        if (action_node_info[i].node_type == common::xnode_type_t::consensus_auditor && action_node_info[i].action_type) {
            slash_auditor_node_size++;
        } else if (action_node_info[i].node_type == common::xnode_type_t::consensus_auditor && !action_node_info[i].action_type) {
            award_auditor_node_size++;
        } else if (action_node_info[i].node_type == common::xnode_type_t::consensus_validator && action_node_info[i].action_type) {
            slash_validator_node_size++;
        } else if (action_node_info[i].node_type == common::xnode_type_t::consensus_validator && !action_node_info[i].action_type) {
            award_validator_node_size++;
        }
    }

    // std::cout << "slash: " << slash_auditor_node_size << ", " << slash_validator_node_size << "\n";
    // std::cout << "award: " << award_auditor_node_size << ", " << award_validator_node_size << "\n";
    EXPECT_TRUE(slash_auditor_node_size == slash_validator_node_size);
    EXPECT_TRUE(slash_auditor_node_size == 1);

    EXPECT_TRUE(award_auditor_node_size == award_validator_node_size);
    EXPECT_TRUE(award_auditor_node_size == 6);
}


TEST_F(test_slash_info_contract, test_process_statistic_data) {
    uint64_t elect_blk_height = 1;
    auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, account_addrs.size());
    auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, account_addrs.size());
    set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});

    auto node_info = process_statistic_data(data, &node_serv);

    for (std::size_t i = 0; i < account_addrs.size(); ++i) {
        EXPECT_EQ(node_info.auditor_info[account_addrs[i]].subset_count, i);
        EXPECT_EQ(node_info.auditor_info[account_addrs[i]].block_count, i);
    }

    for (std::size_t i = 0; i < account_addrs.size(); ++i) {
        EXPECT_EQ(node_info.validator_info[account_addrs[i]].subset_count, i);
        EXPECT_EQ(node_info.validator_info[account_addrs[i]].block_count, i);
    }


}