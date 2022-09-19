// // Copyright (c) 2017-2018 Telos Foundation & contributors
// // Distributed under the MIT software license, see the accompanying
// // file COPYING or http://www.opensource.org/licenses/mit-license.php.
// #include <gtest/gtest.h>
// #include <string>

// #include "xbase/xobject_ptr.h"
// 
// #include "xchain_timer/xchain_timer.h"
// #include "xloader/xconfig_onchain_loader.h"
// #include "xdata/xtransaction.h"
// #include "xdata/xnative_contract_address.h"
// #include "xconfig/xpredefined_configurations.h"
// #include "xdata/xblocktool.h"
// #include "xdata/tests/test_blockutl.hpp"
// #include "tests/xblockstore_test/test_blockmock.hpp"
// #include "xblockstore/xblockstore_face.h"
// #include "xvm/xsystem_contracts/xslash/xtable_statistic_info_collection_contract.h"
// #include "xvm/xsystem_contracts/xslash/xzec_slash_info_contract.h"
// #include "xstake/xdata_structures.h"

// #define private public
// #include "xvm/xvm_service.h"
// #include "xvm/xvm_trace.h"
// #include "xdata/xgenesis_data.h"
// #include "xvm/manager/xcontract_manager.h"


// using namespace top;
// using namespace top::xvm;
// using namespace top::contract;
// using namespace top::xvm::xcontract;


// std::string shard_table_slash_addr = std::string(sys_contract_sharding_statistic_info_addr) + std::string("@3");

// class test_slash_contract_other: public testing::Test {
// public:

//     static void SetUpTestCase() {
//         m_store = xstore_factory::create_store_with_memdb();
//         auto mbus =  top::make_unique<mbus::xmessage_bus_t>(true, 1000);
//         std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
//         std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
//         auto chain_timer = top::make_object_ptr<time::xchain_timer_t>(timer_driver);
//         auto& config_center = top::config::xconfig_register_t::get_instance();

//         config::xconfig_loader_ptr_t loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(m_store), make_observer(mbus.get()), make_observer(chain_timer));
//         config_center.add_loader(loader);
//         config_center.load();

//         // slash contract
//         xcontract_manager_t::instance().register_contract<xtable_statistic_info_collection_contract>(common::xaccount_address_t{sys_contract_sharding_statistic_info_addr}, common::xtopchain_network_id);
//         xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_sharding_statistic_info_addr}, common::xaccount_address_t{shard_table_slash_addr});
//         xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_sharding_statistic_info_addr}, m_store.get());


//         xcontract_manager_t::instance().register_contract<xzec_slash_info_contract>(common::xaccount_address_t{sys_contract_zec_slash_info_addr}, common::xtopchain_network_id);
//         xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_zec_slash_info_addr}, common::xaccount_address_t{sys_contract_zec_slash_info_addr});
//         xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_zec_slash_info_addr}, m_store.get());
//     }


//     static data::xtransaction_ptr_t on_collect_statistic_info(uint64_t timestamp) {
//         xaction_t source_action;
//         xaction_t destination_action;
//         source_action.set_account_addr(shard_table_slash_addr);
//         destination_action.set_account_addr(shard_table_slash_addr);
//         destination_action.set_action_name("on_collect_statistic_info");

//         top::base::xstream_t target_stream(base::xcontext_t::instance());
//         target_stream << timestamp;
//         destination_action.set_action_param(std::string((char*) target_stream.data(), target_stream.size()));

//         data::xtransaction_ptr_t slash_colletion_trx = make_object_ptr<xtransaction_t>();
//         slash_colletion_trx->set_source_action(source_action);
//         slash_colletion_trx->set_target_action(destination_action);
//         return slash_colletion_trx;
//     }

//     static data::xtransaction_ptr_t summarize_slash_info(std::string const& slash_info) {

//         xaction_t source_action;
//         xaction_t destination_action;
//         // source_action.set_account_addr(sys_contract_zec_slash_info_addr);
//         source_action.set_account_addr(shard_table_slash_addr);
//         destination_action.set_account_addr(sys_contract_zec_slash_info_addr);
//         destination_action.set_action_name("summarize_slash_info");


//         destination_action.set_action_param(slash_info);

//         data::xtransaction_ptr_t slash_summarize_trx = make_object_ptr<xtransaction_t>();
//         slash_summarize_trx->set_source_action(source_action);
//         slash_summarize_trx->set_target_action(destination_action);
//         return slash_summarize_trx;

//     }

//     static data::xtransaction_ptr_t do_unqualified_node_slash(uint64_t timestamp) {
//         xaction_t source_action;
//         xaction_t destination_action;
//         source_action.set_account_addr(sys_contract_zec_slash_info_addr);
//         destination_action.set_account_addr(sys_contract_zec_slash_info_addr);
//         destination_action.set_action_name("do_unqualified_node_slash");

//         top::base::xstream_t target_stream(base::xcontext_t::instance());
//         target_stream << timestamp;
//         destination_action.set_action_param(std::string((char*) target_stream.data(), target_stream.size()));

//         data::xtransaction_ptr_t slash_colletion_trx = make_object_ptr<xtransaction_t>();
//         slash_colletion_trx->set_source_action(source_action);
//         slash_colletion_trx->set_target_action(destination_action);
//         return slash_colletion_trx;
//     }

//    static xobject_ptr_t<xstore_face_t> m_store;
//    static shared_ptr<xaccount_context_t> m_table_slash_account_ctx_ptr;
//    static shared_ptr<xaccount_context_t> m_zec_slash_account_ctx_ptr;

// };

// xobject_ptr_t<xstore_face_t> test_slash_contract_other::m_store;
// shared_ptr<xaccount_context_t> test_slash_contract_other::m_table_slash_account_ctx_ptr;
// shared_ptr<xaccount_context_t> test_slash_contract_other::m_zec_slash_account_ctx_ptr;


// xaccount_cmd_ptr_t create_or_modify_property(xstore_face_t* store, const std::string &address, const std::string& list_name, const std::string& item_value) {
//     auto account = store->clone_account(address);
//     xaccount_cmd_ptr_t cmd;

//     int32_t ret;
//     if (account == nullptr) {
//         account = new xblockchain2_t(address);
//         cmd = std::make_shared<xaccount_cmd>(account, store);
//         auto ret = cmd->list_create(list_name, true);
//         assert(ret == 0);
//     } else {
//         int32_t error_code;
//         cmd = std::make_shared<xaccount_cmd>(account, store);
//         auto prop_ptr = cmd->get_property(list_name, error_code);

//         if (prop_ptr == nullptr) {
//             assert(0);
//         }
//         auto ret = cmd->list_push_back(list_name, item_value, true);
//         assert(ret == 0);
//     }

//     return cmd;
// }

// xtransaction_ptr_t create_tx_create_user_account(const std::string &from, uint16_t expire_time = 100) {
//     uint64_t last_nonce = 0;
//     uint256_t last_hash;

//     xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//     tx->set_deposit(100000);
//     tx->make_tx_create_user_account(from);
//     tx->set_last_trans_hash_and_nonce(last_hash, last_nonce);
//     tx->set_same_source_target_address(from);
//     tx->set_fire_and_expire_time(expire_time);
//     tx->set_digest();
//     return tx;
// }

// TEST_F(test_slash_contract_other, table_slash_info_collection) {
//     // for block
//     std::string table_block_addr = std::string(sys_contract_sharding_table_block_addr) + std::string("@3");
//     std::string address = xblocktool_t::make_address_user_account(table_block_addr);
//     xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*m_store.get(), address);

//     test_blockmock_t blockmock(m_store.get());
//     std::string prop_name = "for test slash info";

//     base::xvblock_t *prev_block = (blockstore->get_genesis_block(address).get());
//     base::xvblock_t *curr_block = nullptr;

//     std::vector<base::xvblock_t*> saved_blocks;
//     saved_blocks.push_back(prev_block);
//     int count = 1;
//     for (int i = 1; i <= count; i++) {
//         std::string value = std::to_string(i);
//         xaccount_cmd_ptr_t cmd = create_or_modify_property(m_store.get(), address, prop_name, value);
//         curr_block = blockmock.create_sample_block(prev_block, cmd.get(), address);

//         ASSERT_TRUE(blockstore->store_block(curr_block));
//         ASSERT_EQ(curr_block->get_height(), i);
//         uint64_t chainheight = m_store->get_blockchain_height(address);
//         ASSERT_EQ(chainheight, curr_block->get_height());
//         prev_block = curr_block;
//         saved_blocks.push_back(prev_block);
//     }


//     m_table_slash_account_ctx_ptr = make_shared<xaccount_context_t>(sys_contract_sharding_statistic_info_addr, m_store.get());
//     m_table_slash_account_ctx_ptr->string_create(XPORPERTY_CONTRACT_TIME_KEY);

//     auto const table_time_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(table_statistic_report_schedule_interval);
//     auto trx_ptr = on_collect_statistic_info(table_time_interval + 1);

//     xvm_service vs;
//     xtransaction_trace_ptr trace = make_shared<xtransaction_trace>();
//     xvm_context context{vs, trx_ptr, m_table_slash_account_ctx_ptr.get(), trace};
//     // context.m_contract_account = common::xaccount_address_t{std::string(sys_contract_table_slash_info_collection_addr) + std::string("-088")};
//     // context.m_contract_helper = make_shared<xvm::xcontract_helper>(m_table_slash_account_ctx_ptr.get(), common::xnode_id_t{std::string(sys_contract_table_slash_info_collection_addr) + std::string("-088")}, std::string{""});

//     trace = vs.deal_transaction(trx_ptr, m_table_slash_account_ctx_ptr.get());
//     // EXPECT_EQ(enum_xvm_error_code::enum_vm_exception, trace->m_errno);
//     EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);

// }


// TEST_F(test_slash_contract_other, beacon_slash_info_summarize) {

//     m_zec_slash_account_ctx_ptr = make_shared<xaccount_context_t>(sys_contract_zec_slash_info_addr, store::xstore_factory::create_store_with_memdb().get());
//     m_zec_slash_account_ctx_ptr->map_create(xstake::XPORPERTY_CONTRACT_UNQUALIFIED_NODE_KEY);
//     m_zec_slash_account_ctx_ptr->map_create(xstake::XPROPERTY_CONTRACT_TABLEBLOCK_NUM_KEY);

//     xunqualified_node_info_t  node_info;
//     for (auto i = 0; i < 5; ++i) {
//         xnode_vote_percent_t node_content;
//         node_content.block_count = i + 1;
//         node_content.subset_count = i + 1;
//         node_info.auditor_info[common::xnode_id_t{"auditor" + std::to_string(i)}] = node_content;
//         node_info.validator_info[common::xnode_id_t{"validator" + std::to_string(i)}] = node_content;
//     }

//     base::xstream_t target_stream(base::xcontext_t::instance());
//     node_info.serialize_to(target_stream);
//     uint32_t tableblock_num = 147456 + 1;
//     target_stream << tableblock_num;
//     std::string shard_slash_collect = std::string((char*)target_stream.data(), target_stream.size());

//     target_stream.reset();
//     target_stream << shard_slash_collect;
//     auto trx_ptr = summarize_slash_info(std::string((char*) target_stream.data(), target_stream.size()));

//     xvm_service vs;
//     xtransaction_trace_ptr trace = vs.deal_transaction(trx_ptr, m_zec_slash_account_ctx_ptr.get());
//     EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);
// }

// TEST_F(test_slash_contract_other, beacon_slash_do_slash) {
//     auto const table_time_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(table_statistic_report_schedule_interval);
//     auto trx_ptr = do_unqualified_node_slash(table_time_interval + 1);

//     xvm_service vs;
//     xtransaction_trace_ptr trace = vs.deal_transaction(trx_ptr, m_zec_slash_account_ctx_ptr.get());
//     EXPECT_EQ(enum_xvm_error_code::ok, trace->m_errno);

// }
