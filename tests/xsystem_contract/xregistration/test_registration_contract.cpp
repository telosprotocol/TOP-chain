// // Copyright (c) 2017-2018 Telos Foundation & contributors
// // Distributed under the MIT software license, see the accompanying
// // file COPYING or http://www.opensource.org/licenses/mit-license.php.
// #include <gtest/gtest.h>

// #include <string>

// #define private public
// #include "xbase/xobject_ptr.h"
// #include "xbasic/xasio_io_context_wrapper.h"
// #include "xbasic/xtimer_driver.h"
// #include "xchain_timer/xchain_timer.h"
// #include "xdata/xgenesis_data.h"
// #include "xloader/xconfig_onchain_loader.h"
// 
// #include "xvm/manager/xcontract_manager.h"
// #include "xtxexecutor/xtransaction_context.h"
// #include "xdata/xblocktool.h"
// #include "xvm/xsystem_contracts/xregistration/xrec_registration_contract.h"
// #include "xvm/xvm_service.h"
// #include "xvm/xvm_trace.h"
// #include "xloader/xconfig_offchain_loader.h"
// #include "xloader/xconfig_genesis_loader.h"
// #include "xblockstore/xblockstore_face.h"
// #include "xchain_fork/xutility.h"

// using namespace top;
// using namespace top::xvm;
// using namespace top::xvm::xcontract;
// using namespace top::xstake;
// using namespace top::contract;

// class xtop_hash_t : public base::xhashplugin_t
// {
// public:
//     xtop_hash_t()
//         :base::xhashplugin_t(-1) //-1 = support every hash types
//     {
//     }
// private:
//     xtop_hash_t(const xtop_hash_t &) = delete;
//     xtop_hash_t & operator = (const xtop_hash_t &) = delete;
// public:
//     virtual ~xtop_hash_t(){};
//     virtual const std::string hash(const std::string & input,enum_xhash_type type) override
//     {
//         xassert(type == enum_xhash_type_sha2_256);
//         auto hash = utl::xsha2_256_t::digest(input);
//         return std::string(reinterpret_cast<char*>(hash.data()), hash.size());
//     }
// };

// static bool create_rootblock(const std::string & config_file) {
//     auto genesis_loader = std::make_shared<top::loader::xconfig_genesis_loader_t>(config_file);
//     // config::xconfig_register_t::get_instance().add_loader(genesis_loader);
//     // config::xconfig_register_t::get_instance().load();
//     xrootblock_para_t rootblock_para;
//     if (false == genesis_loader->extract_genesis_para(rootblock_para)) {
//         xerror("create_rootblock extract genesis para fail");
//         return false;
//     }
//     if (false == xrootblock_t::init(rootblock_para)) {
//         xerror("create_rootblock rootblock init fail");
//         return false;
//     }
//     xinfo("create_rootblock success");
//     return true;
// }

// class test_suite_xcontract_t : public testing::Test {
// public:
//     static void SetUpTestCase() {
//         m_store = store::xstore_factory::create_store_with_memdb();
//         top::base::xvchain_t::instance().set_xdbstore(m_store.get());
//         m_blockstore.attach(top::store::get_vblockstore());
//         auto mbus = std::make_shared<top::mbus::xmessage_bus_t>(true, 1000);
//         std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
//         std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
//         auto chain_timer = top::make_object_ptr<time::xchain_timer_t>(timer_driver);

//         //std::string config_file = "/home/yinhp/xchain/src/libraries/xcomponent_administration/config/config.rec1.json";
//         std::string config_file = "";
//         using namespace std;
//         using namespace base;

//         auto hash_plugin = new xtop_hash_t();

//         auto& config_center = top::config::xconfig_register_t::get_instance();
//         auto offchain_loader = std::make_shared<loader::xconfig_offchain_loader_t>(config_file, "");
//         config_center.add_loader(offchain_loader);
//         config_center.load();
//         config_center.remove_loader(offchain_loader);
//         config_center.init_static_config();
//         config_center.dump();

//         if (false == create_rootblock(config_file)) {
//             xassert(0);
//         }
//         config::xconfig_loader_ptr_t loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(m_store), make_observer(mbus), make_observer(chain_timer.get()));
//         config_center.add_loader(loader);
//         config_center.load();

//         // node registration contract
//         xcontract_manager_t::instance().register_contract<xrec_registration_contract>(common::xaccount_address_t{sys_contract_rec_registration_addr},
//                                                                                          common::xtopchain_network_id);
//         xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_rec_registration_addr},
//                                                                           common::xaccount_address_t{sys_contract_rec_registration_addr});
//         xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_rec_registration_addr}, m_blockstore.get());

//         // timer contract
//         /*xcontract_manager_t::instance().register_contract<top::elect::xbeacon_timer_contract>(common::xaccount_address_t{sys_contract_beacon_timer_addr},
//                                                                                               common::xtopchain_network_id);
//         xcontract_manager_t::instance().register_contract_cluster_address(common::xaccount_address_t{sys_contract_beacon_timer_addr},
//                                                                           common::xaccount_address_t{sys_contract_beacon_timer_addr});
//         xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{sys_contract_beacon_timer_addr}, m_store.get());*/
//         //m_clock_account = sys_contract_beacon_timer_addr;
//     }

//     static void TearDownTestCase() {}

//     static int make_block(const std::string & address, const xtransaction_ptr_t & tx) {
//         // auto account = m_store->clone_account(address);
//         // xassert(account != NULL);
//         // /*if (account == nullptr) {
//         //     base::xvblock_t* genesis_block = xblocktool_t::create_genesis_lightunit(address, 1000000000000);
//         //     assert(m_store->set_vblock(genesis_block));
//         //     account = m_store->clone_account(address);
//         // }*/
//         // tx->set_last_nonce(account->account_send_trans_number() + 1);
//         // tx->set_digest();

//         // xaccount_context_t ac(address, m_store.get());

//         // xvm::xvm_service s;
//         // xtransaction_trace_ptr trace = s.deal_transaction(tx, &ac);
//         // xassert(0 == (int) trace->m_errno);
//         // store::xtransaction_result_t result;
//         // ac.get_transaction_result(result);

//         // xlightunit_block_para_t para1;
//         // para1.set_one_input_tx(tx);
//         // para1.set_transaction_result(result);
//         // base::xauto_ptr<base::xvblock_t> block(xlightunit_block_t::create_next_lightunit(para1, account));
//         // xassert(block);
//         // xassert(block->get_block_hash().empty());
//         // static uint64_t clock = 1;
//         // block->get_cert()->set_clock(clock++);
//         // block->get_cert()->set_viewid(block->get_height() + 1);
//         // block->get_cert()->set_validator({1, (uint64_t)-1});
//         // block->get_cert()->set_viewtoken(1111);
//         // if (block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_cert) {
//         //     block->set_extend_cert("1");
//         //     block->set_extend_data("1");
//         // } else {
//         //     block->set_verify_signature("1");
//         // }
//         // block->set_block_flag(base::enum_xvblock_flag_authenticated);
//         // block->set_block_flag(base::enum_xvblock_flag_locked);
//         // block->set_block_flag(base::enum_xvblock_flag_committed);
//         // block->set_block_flag(base::enum_xvblock_flag_executed);
//         // block->set_block_flag(base::enum_xvblock_flag_connected);

//         // auto ret = m_store->set_vblock(block->get_account(), block.get());
//         // xassert(ret);
//         // ret = m_store->execute_block(block.get());
//         // xassert(ret);
//         return 0;
//     }

//     static int recreate_account(const std::string & account) {
//         auto genesis_block = data::xblocktool_t::create_genesis_lightunit(account, 2000000000000);
//         base::xauto_ptr<base::xvblock_t> auto_genesis_block(genesis_block);
//         xassert(genesis_block);

//         auto ret = m_store->set_vblock("./db", genesis_block);
//         if (!ret) {
//             xerror("xtop_application::create_genesis_account store genesis block fail");
//             return ret;
//         }
//         ret = m_store->execute_block(genesis_block);
//         if (!ret) {
//             xerror("xtop_application::create_genesis_account execute genesis block fail");
//             return ret;
//         }

//         data::xunitstate_ptr_t account_ptr  = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_state(account);
//         if (account_ptr == NULL) {
//             xerror("xtop_application::create_genesis_account execute genesis block fail");
//             return 1;
//         }

//         xdbg("[recreate_account] account_balance: %llu", account_ptr->balance());
//         m_original_balance = account_ptr->balance();

//         // m_vote_account_ctx_ptr = std::make_shared<xaccount_context_t>(
//         //         sys_contract_rec_registration_addr,
//         //         m_store.get());
//         return 0;
//     }

//     static int registerNode(std::string node_account, uint64_t deposit, std::string node_types) {
//         top::base::xstream_t sstream(base::xcontext_t::instance());
//         sstream << std::string("");
//         sstream << deposit;
//         sstream << std::string("");

//         top::base::xstream_t tstream(base::xcontext_t::instance());
//         tstream << node_types;
//         tstream << std::string("test1");
//         tstream << std::string();
//         tstream << static_cast<uint32_t>(0);
// #if defined XENABLE_MOCK_ZEC_STAKE
//         tstream << node_account;
// #endif
//         xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//         tx->set_tx_type(xtransaction_type_run_contract);
//         tx->set_deposit(100000);

//         tx->get_source_action().set_account_addr(node_account);
//         tx->get_source_action().set_action_type(xaction_type_asset_out);
//         tx->get_source_action().set_action_param(std::string((char *)sstream.data(), sstream.size()));

//         tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
//         tx->get_target_action().set_action_type(xaction_type_run_contract);
//         tx->get_target_action().set_action_name("registerNode");
//         tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));

//         return make_block(sys_contract_rec_registration_addr, tx);
//     }

//     static int create_set_dividend_rate_tx(std::string node_account, int dividend_rate) {
//         top::base::xstream_t tstream(base::xcontext_t::instance());
//         tstream << dividend_rate;

//         xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//         tx->get_source_action().set_account_addr(node_account);
//         tx->get_source_action().set_action_type(xaction_type_asset_out);
//         tx->get_source_action().set_action_param(std::string(""));

//         tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
//         tx->get_target_action().set_action_type(xaction_type_run_contract);
//         tx->get_target_action().set_action_name("setDividendRatio");
//         tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));
//         return make_block(sys_contract_rec_registration_addr, tx);
//     }

//     static int create_update_node_credit_tx(std::string node_account) {
//         std::set<std::string> accounts;
//         accounts.insert(node_account);

//         top::base::xstream_t tstream(base::xcontext_t::instance());
//         tstream << accounts;

//         xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//         tx->get_source_action().set_account_addr(sys_contract_rec_registration_addr);
//         tx->get_source_action().set_action_type(xaction_type_asset_out);
//         tx->get_source_action().set_action_param(std::string(""));

//         tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
//         tx->get_target_action().set_action_type(xaction_type_run_contract);
//         tx->get_target_action().set_action_name("update_node_credit");
//         tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));
//         return make_block(sys_contract_rec_registration_addr, tx);
//     }

//     static int create_update_batch_stake_tx(std::string node_account, uint64_t votes, uint64_t cur_time) {
//         std::map<std::string, std::string> adv_votes;
//         adv_votes[node_account] = base::xstring_utl::tostring(votes);

//         top::base::xstream_t tstream(base::xcontext_t::instance());
//         std::string target_action;
//         tstream << cur_time;
//         tstream << adv_votes;
//         target_action = "update_batch_stake_v2";

//         xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//         tx->get_source_action().set_account_addr(table_vote_contract_addr);
//         tx->get_source_action().set_action_type(xaction_type_asset_out);
//         tx->get_source_action().set_action_param(std::string(""));

//         tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
//         tx->get_target_action().set_action_type(xaction_type_run_contract);
//         tx->get_target_action().set_action_name(target_action);
//         tx->get_target_action().set_action_param(std::string((char *)tstream.data(), tstream.size()));
//         return make_block(sys_contract_rec_registration_addr, tx);
//     }

//     static int create_node_deregister_tx(std::string node_account) {
//         xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//         tx->get_source_action().set_account_addr(node_account);
//         tx->get_source_action().set_action_type(xaction_type_asset_out);
//         tx->get_source_action().set_action_param(std::string(""));

//         tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
//         tx->get_target_action().set_action_type(xaction_type_run_contract);
//         tx->get_target_action().set_action_name("unregisterNode");
//         tx->get_target_action().set_action_param(std::string(""));
//         return make_block(sys_contract_rec_registration_addr, tx);
//     }

//     static int create_redeem_tx(std::string node_account) {
//         xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
//         tx->get_source_action().set_account_addr(node_account);
//         tx->get_source_action().set_action_type(xaction_type_asset_out);
//         tx->get_source_action().set_action_param(std::string(""));

//         tx->get_target_action().set_account_addr(sys_contract_rec_registration_addr);
//         tx->get_target_action().set_action_type(xaction_type_run_contract);
//         tx->get_target_action().set_action_name("redeemNodeDeposit");
//         tx->get_target_action().set_action_param(std::string(""));
//         return make_block(sys_contract_rec_registration_addr, tx);
//     }

//     static int set_curtime(uint64_t cur_time) {
//         /*xtransaction_result_t result;
//         xaccount_context_t ac(sys_contract_beacon_timer_addr, m_store.get(), {});
//         ac.set_timer_height();
//         // ASSERT_TRUE(ac.get_transaction_result(result));
//         ac.get_transaction_result(result);

//         xblock_ptr_t block = store::xblock_maker::make_light_unit(&ac, {}, result, {});
//         block->calc_block_hash();
//         // ASSERT_EQ(0, m_store->set_consensus_unit(block, {}, {}, result.m_prop_log));
//         m_store->set_consensus_unit(block, {}, {}, result.m_prop_log);*/

//         /*base::xauto_ptr<base::xvblock_t> last_block = m_blockstore->get_latest_cert_block(m_clock_account); //return ptr that has been added reference
//         if(last_block == nullptr)//blockstore has been closed
//             return 1;
//         std::string empty_txs;
//         base::xauto_ptr<base::xvblock_t> last_full_block = m_blockstore->get_latest_full_block(m_clock_account);
//         base::xvblock_t* clock_block = xclockblock_t::create_clockblock(m_clock_account,last_block->get_height() + 1,last_block->get_clock() + 1,last_block->get_viewid() + 1,last_block->get_block_hash(),last_full_block->get_block_hash(),last_full_block->get_height(),empty_txs,empty_txs);
//         clock_block->reset_prev_block(last_block.get()); //point previous block
//         xvip2_t _wildaddr{(xvip_t)(-1),(uint64_t)-1};
//         clock_block->get_cert()->set_validator(_wildaddr);
//         //convert proposal to commit block for test purpose
//         clock_block->set_verify_signature("test-signature");
//         clock_block->set_block_flag(base::enum_xvblock_flag_authenticated);
//         clock_block->set_block_flag(base::enum_xvblock_flag_locked);
//         clock_block->set_block_flag(base::enum_xvblock_flag_committed);
//         m_blockstore->store_block(clock_block);*/

//         return 0;
//     }

//     static xobject_ptr_t<store::xstore_face_t> m_store;
//     static xobject_ptr_t<base::xvblockstore_t> m_blockstore;
//     static shared_ptr<xaccount_context_t> m_vote_account_ctx_ptr;
//     static uint64_t m_original_balance;
//     uint64_t node_morgage{1000000000000};
//     static std::string m_clock_account;
//     static std::string table_vote_contract_addr;
// };

// xobject_ptr_t<store::xstore_face_t> test_suite_xcontract_t::m_store;
// xobject_ptr_t<base::xvblockstore_t> test_suite_xcontract_t::m_blockstore;
// shared_ptr<xaccount_context_t> test_suite_xcontract_t::m_vote_account_ctx_ptr;
// uint64_t test_suite_xcontract_t::m_original_balance;
// std::string test_suite_xcontract_t::m_clock_account{sys_contract_beacon_timer_addr};
// std::string test_suite_xcontract_t::table_vote_contract_addr = std::string(sys_contract_sharding_vote_addr) + "@0";

// TEST_F(test_suite_xcontract_t, registerNode) {
//     std::string node_account = "T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73";


//     recreate_account(node_account);

//     std::string node_types = "advance";
//     /*xtransaction_ptr_t tx = create_node_register_tx(node_account, node_morgage, node_types);
//     xvm_service vs;
//     xtransaction_trace_ptr trace = vs.deal_transaction(tx, m_vote_account_ctx_ptr.get());
//     ASSERT_TRUE(0 == (int) trace->m_errno);*/
//     int ret = registerNode(node_account, node_morgage, node_types);
//     ASSERT_TRUE(ret == 0);

//     node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
//     recreate_account(node_account);

//     ret = registerNode(node_account, node_morgage, node_types);
//     ASSERT_TRUE(ret == 0);

//     {
//         uint64_t latest_height = m_store->get_blockchain_height(sys_contract_rec_registration_addr);
//         ASSERT_TRUE(latest_height == 2);

//         std::map<std::string, std::string> map_nodes;
//         //GET_MAP_PROPERTY(XPORPERTY_CONTRACT_REG_KEY, map_nodes, latest_height, sys_contract_rec_registration_addr);
//         statestore::xstatestore_hub_t::instance()->get_map_property(sys_contract_rec_registration_addr, latest_height, XPORPERTY_CONTRACT_REG_KEY, map_nodes);
//         xdbg("[xzec_reward_contract::calc_nodes_rewards_v2] map_nodes size: %d", map_nodes.size());

//         auto it = map_nodes.find(node_account);
//         ASSERT_TRUE(it != map_nodes.end());
//         xreg_node_info node;
//         xstream_t stream(xcontext_t::instance(), (uint8_t *)it->second.data(), it->second.size());
//         node.serialize_from(stream);
//         xdbg("[xzec_reward_contract::calc_nodes_rewards_v2] map_nodes: account: %s, deposit: %llu, node_type: %s, votes: %llu",
//             node.m_account.c_str(),
//             node.get_deposit(),
//             node.m_genesis_node ? "advance,validator,edge" : common::to_string(node.m_registered_miner_type).c_str(),
//             node.m_vote_amount);
//     }


//     std::string value;
//     // ret = m_vote_account_ctx_ptr->map_get(XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//     // xaccount_context_t ac(node_account, m_store.get(), {});
//     ret = statestore::xstatestore_hub_t::instance()->map_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//     ASSERT_TRUE(ret == 0);

//     base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
//     xreg_node_info node_info;
//     node_info.serialize_from(stream);

//     ASSERT_TRUE(node_info.m_account.value() == node_account);
// #ifndef XENABLE_MOCK_ZEC_STAKE
//     ASSERT_TRUE(node_info.m_account_mortgage == node_morgage);
// #endif
//     ASSERT_TRUE(node_info.could_be_auditor());
// }

// TEST_F(test_suite_xcontract_t, setDividendRatio) {
//     // recreate_account();

//     std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
//     uint dividend_rate = 10;
//     int ret = create_set_dividend_rate_tx(node_account, dividend_rate);
//     ASSERT_TRUE(ret == 0);

//     std::string value;
//     m_vote_account_ctx_ptr->map_get(XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//     base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
//     xreg_node_info node_info;
//     node_info.serialize_from(stream);

//     ASSERT_TRUE(node_info.m_account.value() == node_account);
//     ASSERT_TRUE(node_info.m_support_ratio_numerator == dividend_rate);
//     ASSERT_TRUE(node_info.could_be_auditor());
// }

// TEST_F(test_suite_xcontract_t, update_batch_stake) {
//     // recreate_account();

//     std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
//     uint64_t votes = 10000;
//     uint64_t cur_time = 10;

//     int ret = create_update_batch_stake_tx(node_account, votes, cur_time);
//     ASSERT_TRUE(ret == 0);

//     std::string value;
//     //m_vote_account_ctx_ptr->map_get(XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//     ret = statestore::xstatestore_hub_t::instance()->map_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//     ASSERT_TRUE(ret == 0);
//     base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
//     xreg_node_info node_info;
//     node_info.serialize_from(stream);
//     ASSERT_TRUE(node_info.m_vote_amount == votes);
//     uint64_t auditor_stake = (node_info.m_account_mortgage / TOP_UNIT + node_info.m_vote_amount / 2) * node_info.m_auditor_credit_numerator / node_info.m_auditor_credit_denominator;
//     ASSERT_TRUE(node_info.get_auditor_stake() == auditor_stake);

//     {
//         node_account = "T00000LWUw2ioaCw3TYJ9Lsgu767bbNpmj75kv73";
//         int ret = create_update_batch_stake_tx(node_account, votes, cur_time);
//         ASSERT_TRUE(ret == 0);

//         std::string value;
//         //m_vote_account_ctx_ptr->map_get(XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//         ret = statestore::xstatestore_hub_t::instance()->map_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//         ASSERT_TRUE(ret == 0);
//         base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
//         xreg_node_info node_info;
//         node_info.serialize_from(stream);
//         ASSERT_TRUE(node_info.m_vote_amount == votes);
//         uint64_t auditor_stake = (node_info.m_account_mortgage / TOP_UNIT + node_info.m_vote_amount / 2) * node_info.m_auditor_credit_numerator / node_info.m_auditor_credit_denominator;
//         ASSERT_TRUE(node_info.get_auditor_stake() == auditor_stake);

//         {
//             value.clear();
//             ret = statestore::xstatestore_hub_t::instance()->map_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_TICKETS_KEY, table_vote_contract_addr, value);
//             ASSERT_TRUE(ret == 0);
//             std::map<std::string, std::string> contract_adv_votes;
//             base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
//             stream >> contract_adv_votes;
//             xdbg("[test_suite_xcontract_t update_batch_stake] contract_adv_votes size: %d", contract_adv_votes.size());
//             ASSERT_TRUE(contract_adv_votes.size() == 2);
//         }
//     }

//     {
//         cur_time = 11;
//         int ret = create_update_batch_stake_tx(node_account, votes, cur_time);
//         ASSERT_TRUE(ret == 0);

//         std::string value;
//         //m_vote_account_ctx_ptr->map_get(XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//         ret = statestore::xstatestore_hub_t::instance()->map_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//         ASSERT_TRUE(ret == 0);
//         base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
//         xreg_node_info node_info;
//         node_info.serialize_from(stream);
//         ASSERT_TRUE(node_info.m_vote_amount == votes);
//         uint64_t auditor_stake = (node_info.m_account_mortgage / TOP_UNIT + node_info.m_vote_amount / 2) * node_info.m_auditor_credit_numerator / node_info.m_auditor_credit_denominator;
//         ASSERT_TRUE(node_info.get_auditor_stake() == auditor_stake);

//         {
//             value.clear();
//             ret = statestore::xstatestore_hub_t::instance()->map_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_TICKETS_KEY, table_vote_contract_addr, value);
//             ASSERT_TRUE(ret == 0);
//             std::map<std::string, std::string> contract_adv_votes;
//             base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
//             stream >> contract_adv_votes;
//             xdbg("[test_suite_xcontract_t update_batch_stake] contract_adv_votes size: %d", contract_adv_votes.size());
//             ASSERT_TRUE(contract_adv_votes.size() == 1);
//         }
//     }
// }

// TEST_F(test_suite_xcontract_t, unregisterNode) {
//     // recreate_account();

//     std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
//     int ret = create_node_deregister_tx(node_account);
//     ASSERT_TRUE(ret == 0);

//     std::string value;
//     //ret = m_vote_account_ctx_ptr->map_get(XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//     //ASSERT_TRUE(ret != 0);
//     ret = statestore::xstatestore_hub_t::instance()->map_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REG_KEY, node_account, value);
//     ASSERT_TRUE(ret != 0);

//     value.clear();
//     ret = m_vote_account_ctx_ptr->map_get(XPORPERTY_CONTRACT_REFUND_KEY, node_account, value);
//     ASSERT_TRUE(ret == 0);

//     xrefund_info refund;
//     base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), (uint32_t)value.size());

//     refund.serialize_from(stream);
// #ifndef XENABLE_MOCK_ZEC_STAKE
//     ASSERT_TRUE(refund.refund_amount == node_morgage);
// #endif
// }

// TEST_F(test_suite_xcontract_t, redeemNodeDeposit) {
//     // recreate_account();
//     /*for (uint64_t i = 0; i < REDEEM_INTERVAL + 10; i++) {
//         set_curtime(i);
//     }

//     std::string node_account = "T00000LPgQJCR5AR3kTWSr6NUgMskcJVf6Wkdm8h";
//     xtransaction_ptr_t tx = create_redeem_tx(node_account);
//     xvm_service vs;
//     xtransaction_trace_ptr trace = vs.deal_transaction(tx, m_vote_account_ctx_ptr.get());
//     ASSERT_TRUE(0 == (int)trace->m_errno);

//     std::string value;
//     //int32_t ret = m_vote_account_ctx_ptr->map_get(XPORPERTY_CONTRACT_REFUND_KEY, node_account, value);
//     ret = statestore::xstatestore_hub_t::instance()->map_get(sys_contract_rec_registration_addr, XPORPERTY_CONTRACT_REFUND_KEY, node_account, value);
//     ASSERT_TRUE(ret != 0);

//     data::xunitstate_ptr_t account_ptr = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_state(node_account);
//     ASSERT_TRUE(account_ptr->balance() == m_original_balance);*/
// }
