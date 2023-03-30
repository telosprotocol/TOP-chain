#include <string>
#include <chrono>

#include <gtest/gtest.h>

#include "tests/xelection/xmocked_vnode_service.h"

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"
#include "xblockstore/xblockstore_face.h"
#include "xchain_timer/xchain_timer.h"
#include "xdata/xblock_statistics_data.h"
#include "xdata/xfulltableblock_account_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xvm/xsystem_contracts/xslash/xtable_statistic_info_collection_contract.h"
#include "tests/mock/xvchain_creator.hpp"

using namespace top;
using namespace top::xvm;
using namespace top::xvm::xcontract;
using namespace top::tests::election;
using namespace top::data;

// const uint16_t AUDITOR_ACCOUNT_ADDR_NUM = 64;
// const uint16_t VALIDATOR_ACCOUNT_ADDR_NUM = 128;
const uint16_t AUDITOR_ACCOUNT_ADDR_NUM = 256;
const uint16_t VALIDATOR_ACCOUNT_ADDR_NUM = 512;

class test_table_slash_contract: public xtable_statistic_info_collection_contract, public testing::Test {
public:
    test_table_slash_contract() : xtable_statistic_info_collection_contract{common::xnetwork_id_t{0}}, node_serv{common::xaccount_address_t::build_from(table_statistic_info_contract_base_address), "null"} {};

    void SetUp(){
        mock::xvchain_creator _creator;// TOOD(Jimmy)
        create_account_addrs(AUDITOR_ACCOUNT_ADDR_NUM, VALIDATOR_ACCOUNT_ADDR_NUM);

        uint64_t elect_blk_height = 1;
        nodeservice_add_group(elect_blk_height, create_group_xip2(elect_blk_height, uint8_t(1), AUDITOR_ACCOUNT_ADDR_NUM), auditor_account_addrs);
        nodeservice_add_group(elect_blk_height, create_group_xip2(elect_blk_height, uint8_t(64), VALIDATOR_ACCOUNT_ADDR_NUM), validator_account_addrs);
    }
    void TearDown(){}



    void create_account_addrs(uint32_t auditor_account_num, uint32_t validator_account_num);
    common::xip2_t create_group_xip2(uint64_t elect_blk_height, uint8_t group_id, uint16_t group_size);
    void nodeservice_add_group(uint64_t elect_blk_height, common::xip2_t const& group_xip, std::vector<common::xaccount_address_t> const& nodes);
    void set_according_block_statistic_data(uint64_t elect_blk_height, std::vector<common::xip2_t> const& group_xips);
    void set_according_block_accounts_data(uint64_t elect_blk_height, std::vector<common::xip2_t> const& group_xips);
    data::system_contract::xunqualified_node_info_v1_t process_statistic_data(top::data::xstatistics_data_t const & block_statistic_data,
                                                                              std::vector<base::xvnode_t *> const & auditor_nodes,
                                                                              std::vector<base::xvnode_t *> const & validator_nodes);

public:
    std::vector<common::xaccount_address_t> auditor_account_addrs;
    std::vector<common::xaccount_address_t> validator_account_addrs;
    xstatistics_data_t data;
    data::xfulltableblock_statistic_accounts  accounts;
    xmocked_vnodesvr_t node_serv;
};


void test_table_slash_contract::create_account_addrs(uint32_t auditor_account_num, uint32_t validator_account_num) {
    auditor_account_addrs.resize(auditor_account_num);
    validator_account_addrs.resize(validator_account_num);

    for (uint32_t i = 0; i < auditor_account_num; ++i) {
        std::ostringstream oss;
        oss << "T00000LLC7zGtrhdcCp64hB2GnU1EuKX8aXLu" << std::setfill('0') << std::setw(3) << i;

        auditor_account_addrs[i] = common::xaccount_address_t{oss.str()};
        // top::utl::xecprikey_t prikey;
        // auditor_account_addrs[i] = common::xaccount_address_t{prikey.to_account_address('0', 0)};

    }

    for (uint32_t i = 0; i < validator_account_num; ++i) {
        std::ostringstream oss;
        oss << "T00000LZkNHe2YChVTKNmGXUaXjtZpzmfCA2a" << std::setfill('0') << std::setw(3) << i;

        validator_account_addrs[i] = common::xaccount_address_t{oss.str()};
        // top::utl::xecprikey_t prikey;
        // validator_account_addrs[i] = common::xaccount_address_t{prikey.to_account_address('0', 0)};

    }


}

common::xip2_t test_table_slash_contract::create_group_xip2(uint64_t elect_blk_height, uint8_t group_id, uint16_t group_size) {
     return common::xip2_t{
        common::xnetwork_id_t{0},
        common::xconsensus_zone_id,
        common::xdefault_cluster_id,
        common::xgroup_id_t{group_id},
        group_size,
        elect_blk_height
     };
 }

void test_table_slash_contract::nodeservice_add_group(uint64_t elect_blk_height, common::xip2_t const& group_xip, std::vector<common::xaccount_address_t> const& nodes) {
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

void test_table_slash_contract::set_according_block_statistic_data(uint64_t elect_blk_height, std::vector<common::xip2_t> const& group_xips) {
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
        // std::cout << "[test_table_slash_contract::set_according_block_statistic_data] " << common::xgroup_address_t{group_xips[i].xip()}.to_string() << "\n";
        // std::cout << "[test_table_slash_contract::set_according_block_statistic_data] " << std::hex <<(int)common::xgroup_address_t{group_xips[i].xip()}.type() << "\n";
    }

    data.detail[elect_blk_height] = elect_data;

}

void test_table_slash_contract::set_according_block_accounts_data(uint64_t elect_blk_height, std::vector<common::xip2_t> const& group_xips) {
    data::xfulltableblock_group_data_t group_data;

    for (std::size_t i = 0; i < group_xips.size(); ++i) {
        common::xgroup_address_t  group_addr{group_xips[i].xip()};
        data::xfulltableblock_account_data_t account_data;
        if (group_xips[i].group_id().value() == 1) {
            account_data.account_data = auditor_account_addrs;
            group_data.group_data[group_addr] = account_data;
        } else if (group_xips[i].group_id().value() == 64) {
            account_data.account_data = validator_account_addrs;
            group_data.group_data[group_addr] = account_data;
        }
    }

    accounts.accounts_detail[elect_blk_height] = group_data;

}

data::system_contract::xunqualified_node_info_v1_t test_table_slash_contract::process_statistic_data(top::data::xstatistics_data_t const& block_statistic_data, std::vector<base::xvnode_t*> const & auditor_nodes, std::vector<base::xvnode_t*> const & validator_nodes) {
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

TEST_F(test_table_slash_contract, collect_slash_statistic_info_BENCH) {
    uint64_t elect_blk_height = 1;
    auto group_1_xip2 = create_group_xip2(elect_blk_height, 1, auditor_account_addrs.size());
    auto group_64_xip2 = create_group_xip2(elect_blk_height, 64, validator_account_addrs.size());
    set_according_block_statistic_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});
    set_according_block_accounts_data(1, std::vector<common::xip2_t>{group_1_xip2, group_64_xip2});

    auto auditor_group_nodes = node_serv.get_group(group_1_xip2)->get_nodes();
    auto validator_group_nodes = node_serv.get_group(group_64_xip2)->get_nodes();
    auto node_info = process_statistic_data(data, auditor_group_nodes, validator_group_nodes);

    base::xstream_t stream(base::xcontext_t::instance());
    node_info.serialize_to(stream);
    std::string summarize_info_str = std::string((char*)stream.data(), (size_t)stream.size());

    uint32_t summarize_tableblock_count_for_str = 16;
    stream.reset();
    stream << summarize_tableblock_count_for_str;
    std::string summarize_tableblock_count_str = std::string((char*)stream.data(), (size_t)stream.size());

    data::system_contract::xunqualified_node_info_v1_t summarize_info;
    uint32_t summarize_tableblock_count = 0;
    int count = 1000;

    int total_time = 0;
    for (auto i = 0; i < count; ++i) {
        auto time_start = std::chrono::system_clock::now();
        // success height
        collect_slash_statistic_info(data, accounts, summarize_info_str, summarize_tableblock_count_str,
                                                summarize_info, summarize_tableblock_count);
        auto durarion = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - time_start);
        total_time += durarion.count();
    }

    std::cout << "collect_slash_statistic_info average timecost: " << total_time/count << "\n";

}
