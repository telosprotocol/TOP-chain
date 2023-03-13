// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <gtest/gtest.h>
#include <string>

#include "xbase/xobject_ptr.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"
#include "xblockstore/xblockstore_face.h"
#include "xtxstore/xtxstore_face.h"
#include "xchain_timer/xchain_timer.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xtransaction.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblocktool.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xdata/xsystem_contract/xdata_structures.h"


#include "xchain_upgrade/xchain_data_galileo.h"
#include "tests/mock/xdatamock_unit.hpp"
#include "nlohmann/json.hpp"
#include "xdbstore/xstore_face.h"
#define private public
#include "tests/xelection/xmocked_vnode_service.h"
#include "xvm/xsystem_contracts/xslash/xtable_statistic_info_collection_contract.h"
#include "xvm/xvm_service.h"
#include "xvm/xvm_trace.h"
#include "xdata/xgenesis_data.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"
#include "xdata/xtransaction_v2.h"
#include "xgenesis/xgenesis_manager.h"


using namespace top;
using namespace top::xvm;
using namespace top::contract;
using namespace top::tests::election;
using namespace top::xvm::xcontract;
using json = nlohmann::json;

static top::common::xaccount_address_t build_account_address(std::string const & account_prefix, size_t index) {
    auto account_string = account_prefix + std::to_string(index);
    if (account_string.length() < top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH) {
        account_string.append(top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH - account_string.length(), 'x');
    }
    assert(account_string.length() == top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH);
    return common::xaccount_address_t{account_string};
}

class test_table_slash_contract_other: public xtable_statistic_info_collection_contract, public testing::Test {
public:
    test_table_slash_contract_other(): xtable_statistic_info_collection_contract{common::xnetwork_id_t{0}}{};

    void SetUp() {
        top::base::xvchain_t::instance().clean_all();
        m_store = store::xstore_factory::create_store_with_memdb();
        top::base::xvchain_t::instance().set_xdbstore(m_store.get());
        base::xvblockstore_t * blockstore = store::create_vblockstore(m_store.get());
        base::xvchain_t::instance().set_xblockstore(blockstore);
        m_blockstore.attach(base::xvchain_t::instance().get_xblockstore());
        auto mbus =  top::make_unique<mbus::xmessage_bus_t>(true, 1000);
        std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
        std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
        top::base::xvchain_t::instance().set_xtxstore(txstore::create_txstore(make_observer<mbus::xmessage_bus_face_t>(mbus.get()), timer_driver));
        auto chain_timer = top::make_object_ptr<time::xchain_timer_t>(timer_driver);
        auto& config_center = top::config::xconfig_register_t::get_instance();
        std::unique_ptr<genesis::xgenesis_manager_t> m_genesis_manager = top::make_unique<genesis::xgenesis_manager_t>(top::make_observer(m_blockstore.get()));
        contract::xcontract_deploy_t::instance().deploy_sys_contracts();
        contract::xcontract_manager_t::instance().instantiate_sys_contracts();
        contract::xcontract_manager_t::instance().register_address();
        std::error_code ec;
        m_genesis_manager->init_genesis_block(ec);
        config::xconfig_loader_ptr_t loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(mbus.get()), make_observer(chain_timer));
        config_center.add_loader(loader);
        config_center.load();
    }

    void TearDown(){
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    data::xtransaction_ptr_t on_collect_statistic_info(std::string data_str, uint64_t height, uint64_t tgas) {
        top::base::xstream_t target_stream(base::xcontext_t::instance());
        target_stream << data_str;
        target_stream << height;
        target_stream << tgas;

        data::xtransaction_ptr_t slash_colletion_trx = make_object_ptr<data::xtransaction_v2_t>();
        slash_colletion_trx->source_address(shard_table_slash_addr);
        slash_colletion_trx->target_address(shard_table_slash_addr);
        slash_colletion_trx->set_target_action_name("on_collect_statistic_info");
        slash_colletion_trx->set_target_action_para(std::string((char*) target_stream.data(), target_stream.size()));
        slash_colletion_trx->set_same_source_target_address(shard_table_slash_addr.to_string());
        return slash_colletion_trx;
    }

    data::xtransaction_ptr_t report_summarized_statistic_info(uint64_t timestamp) {
        top::base::xstream_t target_stream(base::xcontext_t::instance());
        target_stream << timestamp;

        data::xtransaction_ptr_t slash_colletion_trx = make_object_ptr<data::xtransaction_v2_t>();
        slash_colletion_trx->source_address(shard_table_slash_addr);
        slash_colletion_trx->target_address(shard_table_slash_addr);
        slash_colletion_trx->set_target_action_name("report_summarized_statistic_info");
        slash_colletion_trx->set_target_action_para(std::string((char*) target_stream.data(), target_stream.size()));
        slash_colletion_trx->set_same_source_target_address(shard_table_slash_addr.to_string());
        return slash_colletion_trx;
    }

   xobject_ptr_t<store::xstore_face_t> m_store;
   xobject_ptr_t<base::xvblockstore_t> m_blockstore;
   shared_ptr<xaccount_context_t> m_table_slash_account_ctx_ptr;
   common::xaccount_address_t shard_table_slash_addr{common::xaccount_address_t::build_from(table_statistic_info_contract_base_address, common::xtable_id_t{3})};
};


#if 0  // TODO(wens) will fail when XCHAIN_FORKED_BY_DEFAULT
TEST_F(test_table_slash_contract_other, table_slash_on_collect_statistic_info) {
    using namespace top::mock;
    xdatamock_unit  table_account{shard_table_slash_addr};

    m_table_slash_account_ctx_ptr = make_shared<xaccount_context_t>(table_account.get_account_state(), m_store.get());
    m_table_slash_account_ctx_ptr->map_create(xstake::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY);
    m_table_slash_account_ctx_ptr->map_create(xstake::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY);
    m_table_slash_account_ctx_ptr->map_create(xstake::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY);
    set_contract_helper(std::make_shared<xcontract_helper>(m_table_slash_account_ctx_ptr.get(), top::common::xnode_id_t{table_account.get_account()}, table_account.get_account()));


    std::vector<common::xaccount_address_t> auditor_account_addrs;
    std::vector<common::xaccount_address_t> validator_account_addrs;
    xelection_related_statistics_data_t elect_data;
    uint32_t auditor_account_num = 256;
    uint32_t validator_account_num = 512;
    auditor_account_addrs.resize(auditor_account_num);
    validator_account_addrs.resize(validator_account_num);

    for (uint32_t i = 0; i < auditor_account_num; ++i) {
        auditor_account_addrs[i] = common::xaccount_address_t{std::string{"auditor_account__"} + std::to_string(i)};
    }

    for (uint32_t i = 0; i < validator_account_num; ++i) {
        validator_account_addrs[i] = common::xaccount_address_t{std::string{"validator_account__"} + std::to_string(i)};
    }

    auto group_1 =  common::xip2_t{
        common::xnetwork_id_t{0},
        common::xconsensus_zone_id,
        common::xdefault_cluster_id,
        common::xgroup_id_t{1},
        auditor_account_num,
        1
     };

    auto group_64 = common::xip2_t{
        common::xnetwork_id_t{0},
        common::xconsensus_zone_id,
        common::xdefault_cluster_id,
        common::xgroup_id_t{64},
        validator_account_num,
        1
     };

    xmocked_vnodesvr_t node_serv{common::xaccount_address_t{"mocked_nodesvr"}, "null"};
    node_serv.add_group(
        group_1.network_id(),
        group_1.zone_id(),
        group_1.cluster_id(),
        group_1.group_id(),
        (uint16_t)group_1.size(),
        1
    );

    xmocked_vnode_group_t* node_group_1 = dynamic_cast<xmocked_vnode_group_t*>(node_serv.get_group(group_1).get());
    node_group_1->reset_nodes();
    for (std::size_t i = 0; i < auditor_account_addrs.size(); ++i) {
        node_group_1->add_node(auditor_account_addrs[i]);
    }
    assert(node_group_1->get_nodes().size() == auditor_account_addrs.size());
    xgroup_related_statistics_data_t group1_data;
    // set vote data
    for (std::size_t i = 0; i < node_group_1->get_size(); ++i) {
        xaccount_related_statistics_data_t account_data;
        account_data.vote_data.block_count = i + 1;
        account_data.vote_data.vote_count = i;

        group1_data.account_statistics_data.push_back(account_data);
    }

    common::xgroup_address_t  group1_addr{group_1.xip()};
    elect_data.group_statistics_data[group1_addr] = group1_data;


    node_serv.add_group(
        group_64.network_id(),
        group_64.zone_id(),
        group_64.cluster_id(),
        group_64.group_id(),
        (uint16_t)group_64.size(),
        1
    );
    xmocked_vnode_group_t* node_group_2 = dynamic_cast<xmocked_vnode_group_t*>(node_serv.get_group(group_64).get());
    node_group_2->reset_nodes();
    for (std::size_t i = 0; i < validator_account_addrs.size(); ++i) {
        node_group_2->add_node(validator_account_addrs[i]);
    }
    assert(node_group_2->get_nodes().size() == validator_account_addrs.size());
    xgroup_related_statistics_data_t group2_data;
    // set vote data
    for (std::size_t i = 0; i < node_group_1->get_size(); ++i) {
        xaccount_related_statistics_data_t account_data;
        account_data.vote_data.block_count = i + 1;
        account_data.vote_data.vote_count = i;

        group2_data.account_statistics_data.push_back(account_data);
    }

    common::xgroup_address_t  group64_addr{group_64.xip()};
    elect_data.group_statistics_data[group64_addr] = group2_data;


    xstatistics_data_t data;
    data.detail[1] = elect_data;
    auto statistic_buffer = data.serialize_based_on<base::xstream_t>();


    m_table_slash_account_ctx_ptr->map_set(xstake::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, "FULLTABLE_HEIGHT", std::to_string(16));
    m_table_slash_account_ctx_ptr->map_set(xstake::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, "FULLTABLE_NUM", std::to_string(16));
    m_table_slash_account_ctx_ptr->map_set(xstake::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, "UNQUALIFIED_NODE", std::string((char*)statistic_buffer.data(), (size_t)statistic_buffer.size()));

    auto trx_ptr = on_collect_statistic_info(std::string((char*)statistic_buffer.data(), (size_t)statistic_buffer.size()), 128, 1);

    xvm_service vs;
    xtransaction_trace_ptr trace = vs.deal_transaction(trx_ptr, m_table_slash_account_ctx_ptr.get());
    // std::cout << trace->m_errno << "\n";
    EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);

}

TEST_F(test_table_slash_contract_other, table_slash_report_statistic_info) {
    using namespace top::mock;
    xdatamock_unit  table_account{shard_table_slash_addr};

    m_table_slash_account_ctx_ptr = make_shared<xaccount_context_t>(table_account.get_account_state(), m_store.get());
    m_table_slash_account_ctx_ptr->map_create(xstake::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY);
    m_table_slash_account_ctx_ptr->map_create(xstake::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY);
    m_table_slash_account_ctx_ptr->map_create(xstake::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY);
    set_contract_helper(std::make_shared<xcontract_helper>(m_table_slash_account_ctx_ptr.get(), top::common::xnode_id_t{table_account.get_account()}, table_account.get_account()));

    xunqualified_node_info_t  node_info;
    for (auto i = 0; i < 5; ++i) {
        xnode_vote_percent_t node_content;
        node_content.block_count = i + 1;
        node_content.subset_count = i + 1;
        node_info.auditor_info[common::xnode_id_t{"auditor" + std::to_string(i)}] = node_content;
        node_info.validator_info[common::xnode_id_t{"validator" + std::to_string(i)}] = node_content;
    }

    base::xstream_t target_stream(base::xcontext_t::instance());
    node_info.serialize_to(target_stream);

    m_table_slash_account_ctx_ptr->map_set( xstake::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, "FULLTABLE_NUM", std::to_string(16));
    m_table_slash_account_ctx_ptr->map_set(xstake::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY, "UNQUALIFIED_NODE", std::string((char*)target_stream.data(), (size_t)target_stream.size()));
    m_table_slash_account_ctx_ptr->map_set(xstake::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY, "FULLTABLE_HEIGHT", std::to_string(16));

    auto trx_ptr = report_summarized_statistic_info(100);

    xvm_service vs;
    xtransaction_trace_ptr trace = vs.deal_transaction(trx_ptr, m_table_slash_account_ctx_ptr.get());
    // std::cout << trace->m_errno << "\n";
    EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);

}
#endif

TEST_F(test_table_slash_contract_other, update_slash_statistic_info) {
    data::system_contract::xunqualified_node_info_v1_t  node_info;
    for (auto i = 0; i < 5; ++i) {
        data::system_contract::xnode_vote_percent_t node_content;
        node_content.block_count = i + 1;
        node_content.subset_count = i + 1;
        node_info.auditor_info[build_account_address("T00000auditor", i)] = node_content;
        node_info.validator_info[build_account_address("T00000validator", i)] = node_content;
    }

    using namespace top::mock;
    xdatamock_unit  table_account{shard_table_slash_addr.to_string()};

    m_table_slash_account_ctx_ptr = make_shared<xaccount_context_t>(table_account.get_account_state());
    m_table_slash_account_ctx_ptr->map_create(data::system_contract::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY);
    m_table_slash_account_ctx_ptr->map_create(data::system_contract::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY);
    m_table_slash_account_ctx_ptr->map_create(data::system_contract::XPROPERTY_CONTRACT_EXTENDED_FUNCTION_KEY);
    set_contract_helper(std::make_shared<xcontract_helper>(m_table_slash_account_ctx_ptr.get(), top::common::xnode_id_t{table_account.get_account()}, shard_table_slash_addr));


    update_slash_statistic_info(node_info, 32, 2);
}
