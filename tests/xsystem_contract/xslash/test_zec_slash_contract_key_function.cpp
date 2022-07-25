// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <gtest/gtest.h>
#include <string>
#include <chrono>

#include "tests/xelection/xmocked_vnode_service.h"

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"
#include "xblockstore/xblockstore_face.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xblock_statistics_data.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xvm/xsystem_contracts/xslash/xzec_slash_info_contract.h"


using namespace top;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::tests::election;
using namespace top::data;

// const uint16_t AUDITOR_ACCOUNT_ADDR_NUM = 64;
// const uint16_t VALIDATOR_ACCOUNT_ADDR_NUM = 128;
const uint16_t AUDITOR_ACCOUNT_ADDR_NUM = 256;
const uint16_t VALIDATOR_ACCOUNT_ADDR_NUM = 512;

class test_zec_slash_contract: public xzec_slash_info_contract, public testing::Test {
public:
    test_zec_slash_contract(): xzec_slash_info_contract{common::xnetwork_id_t{0}}, node_serv{common::xaccount_address_t{"T00000LVwBxzPTQxKKhuxjjhmces35SZcYcZJnXq"}, "MbtRS6k1n0qQI4hqBhwPxXFj+s34lO+58JCxmc9znUo="}{};

    void SetUp(){
        create_account_addrs(AUDITOR_ACCOUNT_ADDR_NUM, VALIDATOR_ACCOUNT_ADDR_NUM);

        uint64_t elect_blk_height = 1;
        nodeservice_add_group(elect_blk_height, create_group_xip2(elect_blk_height, uint8_t(1), AUDITOR_ACCOUNT_ADDR_NUM), auditor_account_addrs);
        nodeservice_add_group(elect_blk_height, create_group_xip2(elect_blk_height, uint8_t(64), VALIDATOR_ACCOUNT_ADDR_NUM), validator_account_addrs);
    }
    void TearDown(){}



    void create_account_addrs(uint32_t auditor_account_num, uint32_t validator_account_num);
    common::xip2_t create_group_xip2(uint64_t elect_blk_height, uint8_t group_id, uint16_t group_size);
    void nodeservice_add_group(uint64_t elect_blk_height, common::xip2_t const& group_xip, std::vector<common::xaccount_address_t> const& nodes);
    void set_according_block_statistic_data(uint64_t elect_blk_height, std::vector<common::xip2_t> const& group_xip);
    data::system_contract::xunqualified_node_info_v1_t process_statistic_data(top::data::xstatistics_data_t const& block_statistic_data,
                                                    std::vector<base::xvnode_t*> const & auditor_nodes, std::vector<base::xvnode_t*> const & validator_nodes);


public:
    std::vector<common::xaccount_address_t> auditor_account_addrs;
    std::vector<common::xaccount_address_t> validator_account_addrs;
    xstatistics_data_t data;
    xmocked_vnodesvr_t node_serv;
};

static top::common::xaccount_address_t build_account_address(std::string const & account_prefix, size_t index) {
    auto account_string = account_prefix + std::to_string(index);
    if (account_string.length() < top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH) {
        account_string.append(top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH - account_string.length(), 'x');
    }
    assert(account_string.length() == top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH);
    return common::xaccount_address_t{account_string};
}


void test_zec_slash_contract::create_account_addrs(uint32_t auditor_account_num, uint32_t validator_account_num) {
    auditor_account_addrs.resize(auditor_account_num);
    validator_account_addrs.resize(validator_account_num);

    for (uint32_t i = 0; i < auditor_account_num; ++i) {
        auditor_account_addrs[i] = build_account_address("T00000auditor_account__", i);
    }

    for (uint32_t i = 0; i < validator_account_num; ++i) {
        validator_account_addrs[i] = build_account_address("T00000validator_account__", i);
    }


}

common::xip2_t test_zec_slash_contract::create_group_xip2(uint64_t elect_blk_height, uint8_t group_id, uint16_t group_size) {
     return common::xip2_t{
        common::xnetwork_id_t{0},
        common::xconsensus_zone_id,
        common::xdefault_cluster_id,
        common::xgroup_id_t{group_id},
        group_size,
        elect_blk_height
     };
 }

void test_zec_slash_contract::nodeservice_add_group(uint64_t elect_blk_height, common::xip2_t const& group_xip, std::vector<common::xaccount_address_t> const& nodes) {
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

void test_zec_slash_contract::set_according_block_statistic_data(uint64_t elect_blk_height, std::vector<common::xip2_t> const& group_xips) {
    xelection_related_statistics_data_t elect_data;
    for (std::size_t i = 0; i < group_xips.size(); ++i) {
        auto vnode_group = node_serv.get_group(group_xips[i]);
        assert(vnode_group != nullptr);

        xgroup_related_statistics_data_t group_data;
        // set vote data
        for (std::size_t i = 0; i < vnode_group->get_size(); ++i) {
            xaccount_related_statistics_data_t account_data;
            account_data.vote_data.block_count = i + 1;
            account_data.vote_data.vote_count = i;

            group_data.account_statistics_data.push_back(account_data);
        }

        common::xgroup_address_t  group_addr{group_xips[i].xip()};
        elect_data.group_statistics_data[group_addr] = group_data;
        // std::cout << "[test_zec_slash_contract::set_according_block_statistic_data] " << common::xgroup_address_t{group_xips[i].xip()}.to_string() << "\n";
        // std::cout << "[test_zec_slash_contract::set_according_block_statistic_data] " << std::hex <<(int)common::xgroup_address_t{group_xips[i].xip()}.type() << "\n";
    }

    data.detail[elect_blk_height] = elect_data;

}

data::system_contract::xunqualified_node_info_v1_t test_zec_slash_contract::process_statistic_data(top::data::xstatistics_data_t const& block_statistic_data, std::vector<base::xvnode_t*> const & auditor_nodes, std::vector<base::xvnode_t*> const & validator_nodes) {
    data::system_contract::xunqualified_node_info_v1_t res_node_info;

    // process one full tableblock statistic data
    for (auto const & static_item: block_statistic_data.detail) {
        auto elect_statistic = static_item.second;
        for (auto const & group_item: elect_statistic.group_statistics_data) {
            xgroup_related_statistics_data_t const& group_account_data = group_item.second;
            common::xgroup_address_t const& group_addr = group_item.first;
            xvip2_t const& group_xvip2 = top::common::xip2_t{
                group_addr.network_id(),
                group_addr.zone_id(),
                group_addr.cluster_id(),
                group_addr.group_id(),
                (uint16_t)group_account_data.account_statistics_data.size(),
                static_item.first
            };
            // process auditor group
            if (top::common::has<top::common::xnode_type_t::consensus_auditor>(group_addr.type())) {
                for (std::size_t slotid = 0; slotid < group_account_data.account_statistics_data.size(); ++slotid) {
                    auto account_addr = auditor_nodes[slotid]->get_account();
                    res_node_info.auditor_info[common::xnode_id_t{account_addr}].subset_count += group_account_data.account_statistics_data[slotid].vote_data.block_count;
                    res_node_info.auditor_info[common::xnode_id_t{account_addr}].block_count += group_account_data.account_statistics_data[slotid].vote_data.vote_count;
                    xdbg("[xzec_slash_info_contract][do_unqualified_node_slash] incremental auditor data: {gourp id: %d, account addr: %s, slot id: %u, subset count: %u, block_count: %u}", group_addr.group_id().value(), account_addr.c_str(),
                        slotid, group_account_data.account_statistics_data[slotid].vote_data.block_count, group_account_data.account_statistics_data[slotid].vote_data.vote_count);
                }
            } else if (top::common::has<top::common::xnode_type_t::consensus_validator>(group_addr.type())) {// process validator group
                for (std::size_t slotid = 0; slotid < group_account_data.account_statistics_data.size(); ++slotid) {
                    auto account_addr = validator_nodes[slotid]->get_account();
                    res_node_info.validator_info[common::xnode_id_t{account_addr}].subset_count += group_account_data.account_statistics_data[slotid].vote_data.block_count;
                    res_node_info.validator_info[common::xnode_id_t{account_addr}].block_count += group_account_data.account_statistics_data[slotid].vote_data.vote_count;
                    xdbg("[xzec_slash_info_contract][do_unqualified_node_slash] incremental validator data: {gourp id: %d, account addr: %s, slot id: %u, subset count: %u, block_count: %u}", group_addr.group_id().value(), account_addr.c_str(),
                        slotid, group_account_data.account_statistics_data[slotid].vote_data.block_count, group_account_data.account_statistics_data[slotid].vote_data.vote_count);
                }

            } else { // invalid group
                xwarn("[xzec_slash_info_contract][do_unqualified_node_slash] invalid group id: %d", group_addr.group_id().value());
                std::error_code ec{ xvm::enum_xvm_error_code::enum_vm_exception };
                top::error::throw_error(ec, "[xzec_slash_info_contract][do_unqualified_node_slash] invalid group");
            }

        }

    }

    return res_node_info;
}

TEST_F(test_zec_slash_contract, test_statistic_data) {

    uint64_t elect_blk_height = 1;
    auto auditor_group_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
    auto validator_group_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
    set_according_block_statistic_data(1, std::vector<common::xip2_t>{auditor_group_xip2, validator_group_xip2});

    for (std::size_t i = 0; i < auditor_account_addrs.size(); ++i) {
        EXPECT_EQ(data.detail[elect_blk_height].group_statistics_data[common::xgroup_address_t{auditor_group_xip2.xip()}].account_statistics_data[i].vote_data.block_count, i+1);
        EXPECT_EQ(data.detail[elect_blk_height].group_statistics_data[common::xgroup_address_t{auditor_group_xip2.xip()}].account_statistics_data[i].vote_data.vote_count, i);
    }

    for (std::size_t i = 0; i < validator_account_addrs.size(); ++i) {
        EXPECT_EQ(data.detail[elect_blk_height].group_statistics_data[common::xgroup_address_t{validator_group_xip2.xip()}].account_statistics_data[i].vote_data.block_count, i+1);
        EXPECT_EQ(data.detail[elect_blk_height].group_statistics_data[common::xgroup_address_t{validator_group_xip2.xip()}].account_statistics_data[i].vote_data.vote_count, i);
    }
}

TEST_F(test_zec_slash_contract, test_accumulate_node_info) {
    data::system_contract::xunqualified_node_info_v1_t origin_info;

    for (std::size_t i = 0; i < auditor_account_addrs.size(); ++i) {
        data::system_contract::xnode_vote_percent_t node_vote;
        node_vote.subset_count = i;
        node_vote.block_count = i;
        origin_info.auditor_info[auditor_account_addrs[i]] = node_vote;
    }

    for (std::size_t i = 0; i < validator_account_addrs.size(); ++i) {
        data::system_contract::xnode_vote_percent_t node_vote;
        node_vote.subset_count = i;
        node_vote.block_count = i;
        origin_info.validator_info[validator_account_addrs[i]] = node_vote;
    }

    data::system_contract::xunqualified_node_info_v1_t summarize_slash_info;
    accumulate_node_info(origin_info, summarize_slash_info);

    for (std::size_t i = 0; i < auditor_account_addrs.size(); ++i) {
        EXPECT_EQ(summarize_slash_info.auditor_info[auditor_account_addrs[i]].subset_count, i);
        EXPECT_EQ(summarize_slash_info.auditor_info[auditor_account_addrs[i]].block_count, i);
    }

    for (std::size_t i = 0; i < validator_account_addrs.size(); ++i) {
        EXPECT_EQ(summarize_slash_info.validator_info[validator_account_addrs[i]].subset_count, i);
        EXPECT_EQ(summarize_slash_info.validator_info[validator_account_addrs[i]].block_count, i);
    }


    accumulate_node_info(origin_info, summarize_slash_info);
    for (std::size_t i = 0; i < auditor_account_addrs.size(); ++i) {
        EXPECT_EQ(summarize_slash_info.auditor_info[auditor_account_addrs[i]].subset_count, 2 * i);
        EXPECT_EQ(summarize_slash_info.auditor_info[auditor_account_addrs[i]].block_count, 2 * i);
    }

    for (std::size_t i = 0; i < validator_account_addrs.size(); ++i) {
        EXPECT_EQ(summarize_slash_info.validator_info[validator_account_addrs[i]].subset_count, 2 * i);
        EXPECT_EQ(summarize_slash_info.validator_info[validator_account_addrs[i]].block_count, 2 * i);
    }
}


TEST_F(test_zec_slash_contract, test_filter_node) {
    data::system_contract::xunqualified_node_info_v1_t origin_info;

    for (std::size_t i = 0; i < auditor_account_addrs.size(); ++i) {
        data::system_contract::xnode_vote_percent_t node_vote;
        node_vote.block_count = i;
        node_vote.subset_count = 10;
        origin_info.auditor_info[auditor_account_addrs[i]] = node_vote;
    }

    for (std::size_t i = 0; i < validator_account_addrs.size(); ++i) {
        data::system_contract::xnode_vote_percent_t node_vote;
        node_vote.block_count = i;
        node_vote.subset_count = 10;
        origin_info.validator_info[validator_account_addrs[i]] = node_vote;
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

    EXPECT_TRUE(award_auditor_node_size != award_validator_node_size);
    // EXPECT_TRUE(award_auditor_node_size == 6);
    EXPECT_TRUE(award_auditor_node_size == AUDITOR_ACCOUNT_ADDR_NUM * 8 / 10);
    EXPECT_TRUE(award_validator_node_size == VALIDATOR_ACCOUNT_ADDR_NUM * 8 / 10);

}

TEST_F(test_zec_slash_contract, test_print_summarize_info) {
    data::system_contract::xunqualified_node_info_v1_t  node_info;
    for (auto i = 0; i < 5; ++i) {
        data::system_contract::xnode_vote_percent_t node_content;
        node_content.block_count = i + 1;
        node_content.subset_count = i + 1;
        node_info.auditor_info[build_account_address("T00000auditor", i)] = node_content;
        node_info.validator_info[build_account_address("T00000validator", i)] = node_content;
    }

    print_summarize_info(node_info);
}


TEST_F(test_zec_slash_contract, test_summarize_info_internal) {
    uint64_t elect_blk_height = 1;
    auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
    auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
    set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});
    auto auditor_group_nodes = node_serv.get_group(group_1_xip2)->get_nodes();
    auto validator_group_nodes = node_serv.get_group(group_64_xip2)->get_nodes();
    auto node_info = process_statistic_data(data, auditor_group_nodes, validator_group_nodes);

    uint64_t report_height = 32;
    base::xstream_t stream(base::xcontext_t::instance());
    node_info.serialize_to(stream);
    stream << report_height;
    auto shard_slash_collect = std::string((char *)stream.data(), stream.size());

    stream.reset();
    node_info.serialize_to(stream);
    std::string summarize_info_str = std::string((char*)stream.data(), (size_t)stream.size());

    uint32_t summarize_tableblock_count_for_str = 16;
    stream.reset();
    stream << summarize_tableblock_count_for_str;
    std::string summarize_tableblock_count_str = std::string((char*)stream.data(), (size_t)stream.size());

    data::system_contract::xunqualified_node_info_v1_t summarize_info;
    uint32_t summarize_tableblock_count = 0;
    std::uint64_t cur_statistic_height = 0;

    // fail height
    auto res1 = summarize_slash_info_internal(shard_slash_collect, summarize_info_str, summarize_tableblock_count_str, 32,
                                            summarize_info, summarize_tableblock_count, cur_statistic_height);
    EXPECT_EQ(cur_statistic_height, 32);
    EXPECT_FALSE(res1);

    // success height
    auto res2 = summarize_slash_info_internal(shard_slash_collect, summarize_info_str, summarize_tableblock_count_str, 64,
                                            summarize_info, summarize_tableblock_count, cur_statistic_height);

    EXPECT_EQ(cur_statistic_height, 32);
    EXPECT_FALSE(res2);
}
// #ifdef SLASH_TEST
// TEST_F(test_zec_slash_contract, test_do_unqualified_node_slash_internal_normal) {
//     uint64_t elect_blk_height = 1;
//     auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
//     auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
//     set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});
//     auto auditor_group_nodes = node_serv.get_group(group_1_xip2)->get_nodes();
//     auto validator_group_nodes = node_serv.get_group(group_64_xip2)->get_nodes();
//     auto node_info = process_statistic_data(data, auditor_group_nodes, validator_group_nodes);

//     std::string last_slash_time_str = "100";
//     xunqualified_node_info_t summarize_info = node_info;
//     uint32_t summarize_tableblock_count = 32;
//     auto punish_interval_table_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block);
//     auto punish_interval_time_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block);

//     // get filter param
//     auto slash_vote = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_publishment_threshold_value);
//     auto slash_persent = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value);
//     auto award_vote = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_reward_threshold_value);
//     auto award_persent = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_reward_threshold_value);

//     uint64_t timestamp = 200;
//     std::vector<xaction_node_info_t> node_to_action;
//     auto res = do_unqualified_node_slash_internal(last_slash_time_str, summarize_tableblock_count, punish_interval_table_block_param, punish_interval_time_block_param, timestamp,
//                                            summarize_info, slash_vote, slash_persent, award_vote, award_persent, node_to_action);
//     EXPECT_TRUE(res);

// }

// TEST_F(test_zec_slash_contract, test_do_unqualified_node_slash_internal_fail1) {
//     uint64_t elect_blk_height = 1;
//     auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
//     auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
//     set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});
//     auto auditor_group_nodes = node_serv.get_group(group_1_xip2)->get_nodes();
//     auto validator_group_nodes = node_serv.get_group(group_64_xip2)->get_nodes();
//     auto node_info = process_statistic_data(data, auditor_group_nodes, validator_group_nodes);

//     std::string last_slash_time_str = "100";
//     xunqualified_node_info_t summarize_info = node_info;
//     uint32_t summarize_tableblock_count = 10;
//     auto punish_interval_table_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block);
//     auto punish_interval_time_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block);

//     // get filter param
//     auto slash_vote = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_publishment_threshold_value);
//     auto slash_persent = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value);
//     auto award_vote = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_reward_threshold_value);
//     auto award_persent = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_reward_threshold_value);

//     uint64_t timestamp = 200;
//     std::vector<xaction_node_info_t> node_to_action;
//     auto res = do_unqualified_node_slash_internal(last_slash_time_str, summarize_tableblock_count, punish_interval_table_block_param, punish_interval_time_block_param, timestamp,
//                                            summarize_info, slash_vote, slash_persent, award_vote, award_persent, node_to_action);

//     EXPECT_FALSE(res);

// }

// TEST_F(test_zec_slash_contract, test_do_unqualified_node_slash_internal_fail2) {
//     uint64_t elect_blk_height = 1;
//     auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
//     auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
//     set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});
//     auto auditor_group_nodes = node_serv.get_group(group_1_xip2)->get_nodes();
//     auto validator_group_nodes = node_serv.get_group(group_64_xip2)->get_nodes();
//     auto node_info = process_statistic_data(data, auditor_group_nodes, validator_group_nodes);

//     std::string last_slash_time_str = "100";
//     xunqualified_node_info_t summarize_info = node_info;
//     uint32_t summarize_tableblock_count = 32;
//     auto punish_interval_table_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block);
//     auto punish_interval_time_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block);

//     // get filter param
//     auto slash_vote = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_publishment_threshold_value);
//     auto slash_persent = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value);
//     auto award_vote = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_reward_threshold_value);
//     auto award_persent = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_reward_threshold_value);

//     uint64_t timestamp = 120;
//     std::vector<xaction_node_info_t> node_to_action;
//     auto res = do_unqualified_node_slash_internal(last_slash_time_str, summarize_tableblock_count, punish_interval_table_block_param, punish_interval_time_block_param, timestamp,
//                                            summarize_info, slash_vote, slash_persent, award_vote, award_persent, node_to_action);
//     EXPECT_FALSE(res);

// }
// #endif

TEST_F(test_zec_slash_contract, serialize_and_deserialize_BENCH) {
    uint64_t elect_blk_height = 1;
    auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
    auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
    set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});

    auto data_string = data.serialize_based_on<base::xstream_t>();

    int count = 1000;

    int total_time = 0;
    for (auto i = 0; i < count; ++i) {
        auto time_start = std::chrono::system_clock::now();
        xstatistics_data_t process_data;
        process_data.deserialize_based_on<base::xstream_t>(data_string);
        auto durarion = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time_start);
        total_time += durarion.count();

    }

    std::cout << "serialize_and_deserialize average timecost: " << total_time/count << "\n";

}

TEST_F(test_zec_slash_contract, process_statistic_data_BENCH) {
    uint64_t elect_blk_height = 1;
    auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
    auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
    set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});
    auto auditor_group_nodes = node_serv.get_group(group_1_xip2)->get_nodes();
    auto validator_group_nodes = node_serv.get_group(group_64_xip2)->get_nodes();
    auto data_string = data.serialize_based_on<base::xstream_t>();

    int count = 1000;

    int total_time = 0;
    for (auto i = 0; i < count; ++i) {
        auto time_start = std::chrono::system_clock::now();
        auto node_info = process_statistic_data(data, auditor_group_nodes, validator_group_nodes);
        auto durarion = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time_start);
        total_time += durarion.count();

    }

    std::cout << "process_statistic_data average timecost: " << total_time/count << "\n";

}


TEST_F(test_zec_slash_contract, accumulate_node_info_BENCH) {
    data::system_contract::xunqualified_node_info_v1_t origin_info;

    for (std::size_t i = 0; i < auditor_account_addrs.size(); ++i) {
        data::system_contract::xnode_vote_percent_t node_vote;
        node_vote.subset_count = i;
        node_vote.block_count = i;
        origin_info.auditor_info[auditor_account_addrs[i]] = node_vote;
    }

    for (std::size_t i = 0; i < validator_account_addrs.size(); ++i) {
        data::system_contract::xnode_vote_percent_t node_vote;
        node_vote.subset_count = i;
        node_vote.block_count = i;
        origin_info.auditor_info[validator_account_addrs[i]] = node_vote;
    }


    int count = 1000;

    int total_time = 0;
    for (auto i = 0; i < count; ++i) {
        data::system_contract::xunqualified_node_info_v1_t summarize_slash_info;
        auto time_start = std::chrono::system_clock::now();
        accumulate_node_info(origin_info, summarize_slash_info);
        auto durarion = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time_start);
        total_time += durarion.count();
    }

    std::cout << "accumulate_node_info average timecost: " << total_time/count << "\n";

}

TEST_F(test_zec_slash_contract, filter_helper_BENCH) {
    data::system_contract::xunqualified_node_info_v1_t origin_info;

    for (std::size_t i = 0; i < auditor_account_addrs.size(); ++i) {
        data::system_contract::xnode_vote_percent_t node_vote;
        node_vote.block_count = i;
        node_vote.subset_count = 10;
        origin_info.auditor_info[auditor_account_addrs[i]] = node_vote;
    }

    for (std::size_t i = 0; i < validator_account_addrs.size(); ++i) {
        data::system_contract::xnode_vote_percent_t node_vote;
        node_vote.block_count = i;
        node_vote.subset_count = 10;
        origin_info.validator_info[validator_account_addrs[i]] = node_vote;
    }

    int count = 1000;

    int total_time = 0;
    for (auto i = 0; i < count; ++i) {
        auto time_start = std::chrono::system_clock::now();
        auto  action_node_info = filter_helper(origin_info, 0, 10, 30, 80);
        auto durarion = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time_start);
        total_time += durarion.count();

    }

    std::cout << "filter_helper average timecost: " << total_time/count << "\n";

}


TEST_F(test_zec_slash_contract, summarize_info_internal_BENCH) {
    uint64_t elect_blk_height = 1;
    auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
    auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
    set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});
    auto auditor_group_nodes = node_serv.get_group(group_1_xip2)->get_nodes();
    auto validator_group_nodes = node_serv.get_group(group_64_xip2)->get_nodes();
    auto node_info = process_statistic_data(data, auditor_group_nodes, validator_group_nodes);

    uint64_t report_height = 32;
    base::xstream_t stream(base::xcontext_t::instance());
    node_info.serialize_to(stream);
    stream << report_height;
    auto shard_slash_collect = std::string((char *)stream.data(), stream.size());

    stream.reset();
    node_info.serialize_to(stream);
    std::string summarize_info_str = std::string((char*)stream.data(), (size_t)stream.size());

    uint32_t summarize_tableblock_count_for_str = 16;
    stream.reset();
    stream << summarize_tableblock_count_for_str;
    std::string summarize_tableblock_count_str = std::string((char*)stream.data(), (size_t)stream.size());

    data::system_contract::xunqualified_node_info_v1_t summarize_info;
    uint32_t summarize_tableblock_count = 0;
    std::uint64_t cur_statistic_height = 0;



    int count = 1000;

    int total_time = 0;
    for (auto i = 0; i < count; ++i) {
        data::system_contract::xunqualified_node_info_v1_t summarize_slash_info;
        auto time_start = std::chrono::system_clock::now();
        // success height
        auto res = summarize_slash_info_internal(shard_slash_collect, summarize_info_str, summarize_tableblock_count_str, 16,
                                            summarize_info, summarize_tableblock_count, cur_statistic_height);
        EXPECT_TRUE(res);
        auto durarion = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time_start);
        total_time += durarion.count();
    }

    std::cout << "summarize_info_internal average timecost: " << total_time/count << "\n";

}


// #ifdef SLASH_TEST
// TEST_F(test_zec_slash_contract, do_unqualified_node_slash_internal_BENCH) {
//     uint64_t elect_blk_height = 1;
//     auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
//     auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
//     set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});
//     auto auditor_group_nodes = node_serv.get_group(group_1_xip2)->get_nodes();
//     auto validator_group_nodes = node_serv.get_group(group_64_xip2)->get_nodes();
//     auto node_info = process_statistic_data(data, auditor_group_nodes, validator_group_nodes);

//     std::string last_slash_time_str = "100";
//     xunqualified_node_info_t summarize_info = node_info;
//     uint32_t summarize_tableblock_count = 32;
//     auto punish_interval_table_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_table_block);
//     auto punish_interval_time_block_param = XGET_ONCHAIN_GOVERNANCE_PARAMETER(punish_interval_time_block);

//     // get filter param
//     auto slash_vote = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_publishment_threshold_value);
//     auto slash_persent = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_publishment_threshold_value);
//     auto award_vote = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_ranking_reward_threshold_value);
//     auto award_persent = XGET_ONCHAIN_GOVERNANCE_PARAMETER(sign_block_reward_threshold_value);

//     uint64_t timestamp = 200;
//     std::vector<xaction_node_info_t> node_to_action;

//     int count = 1000;

//     int total_time = 0;
//     for (auto i = 0; i < count; ++i) {
//         auto time_start = std::chrono::system_clock::now();
//         auto res = do_unqualified_node_slash_internal(last_slash_time_str, summarize_tableblock_count, punish_interval_table_block_param, punish_interval_time_block_param, timestamp,
//                                            summarize_info, slash_vote, slash_persent, award_vote, award_persent, node_to_action);
//         EXPECT_TRUE(res);
//         auto durarion = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time_start);
//         total_time += durarion.count();
//     }

//     std::cout << "do_unqualified_node_slash_internal average timecost: " << total_time/count << "\n";
// }
// #endif






