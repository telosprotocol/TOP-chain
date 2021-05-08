// #include "gtest/gtest.h"
// #include "xblockmaker/xunit_maker.h"
// #include "xblockmaker/xunit_builder.h"
// #include "test_common.hpp"
// #include "xstore/xstore_face.h"
// #include "xstore/xaccount_context.h"
// #include "xblockstore/xblockstore_face.h"
// #include "xdata/xtransaction_maker.hpp"
// #include "tests/mock/xdatamock_table.hpp"
// #include "tests/mock/xdatamock_tx.hpp"
// #include "tests/mock/xcertauth_util.hpp"

// using namespace top::blockmaker;
// using namespace top::store;
// using namespace top::base;
// using namespace top::mock;

// class test_unit_maker : public testing::Test {
//  protected:
//     void SetUp() override {

//     }

//     void TearDown() override {
//     }
//  public:

// };

// TEST_F(test_unit_maker, blockmaker_set_latest_block) {
//     std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
//     xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();

//     xemptyunit_builder_t builder;
//     xblock_ptr_t genesis_block = builder.build_genesis_block(account1);
//     xassert(!genesis_block->get_block_hash().empty());
//     std::vector<xblock_ptr_t>   blocks;
//     blocks.push_back(genesis_block);

//     xblock_ptr_t prev_block = genesis_block;
//     for (uint64_t height = 1; height < 100; height++) {
//         xblock_consensus_para_t cs_para(xvip2_t{10,10}, prev_block.get());
//         xblock_ptr_t nongenesis_block = builder.build_empty_block(prev_block, cs_para);
//         xassert(nongenesis_block != nullptr);
//         xdatamock_tx::do_mock_signature(nongenesis_block.get());
//         xassert(!nongenesis_block->get_block_hash().empty());
//         blocks.push_back(nongenesis_block);
//         prev_block = nongenesis_block;
//     }

//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         unitmaker->set_latest_block(blocks[50]);
//         ASSERT_EQ(unitmaker->get_highest_height_block()->get_height(), 50);
//         ASSERT_EQ(unitmaker->get_lowest_height_block()->get_height(), 50);
//         ASSERT_EQ(unitmaker->get_latest_blocks().size(), 1);
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         unitmaker->set_latest_block(blocks[50]);
//         unitmaker->set_latest_block(blocks[47]);
//         ASSERT_EQ(unitmaker->get_highest_height_block()->get_height(), 50);
//         ASSERT_EQ(unitmaker->get_lowest_height_block()->get_height(), 50);
//         ASSERT_EQ(unitmaker->get_latest_blocks().size(), 1);
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         unitmaker->set_latest_block(blocks[50]);
//         unitmaker->set_latest_block(blocks[53]);
//         ASSERT_EQ(unitmaker->get_highest_height_block()->get_height(), 53);
//         ASSERT_EQ(unitmaker->get_lowest_height_block()->get_height(), 53);
//         ASSERT_EQ(unitmaker->get_latest_blocks().size(), 1);
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         unitmaker->set_latest_block(blocks[50]);
//         unitmaker->set_latest_block(blocks[51]);
//         unitmaker->set_latest_block(blocks[52]);
//         unitmaker->set_latest_block(blocks[53]);
//         ASSERT_EQ(unitmaker->get_highest_height_block()->get_height(), 53);
//         ASSERT_EQ(unitmaker->get_lowest_height_block()->get_height(), 51);
//         ASSERT_EQ(unitmaker->get_latest_blocks().size(), 3);
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         unitmaker->set_latest_block(blocks[50]);
//         unitmaker->set_latest_block(blocks[51]);
//         unitmaker->set_latest_block(blocks[52]);
//         unitmaker->set_latest_block(blocks[55]);
//         ASSERT_EQ(unitmaker->get_highest_height_block()->get_height(), 55);
//         ASSERT_EQ(unitmaker->get_lowest_height_block()->get_height(), 55);
//         ASSERT_EQ(unitmaker->get_latest_blocks().size(), 1);
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         unitmaker->set_latest_block(blocks[50]);
//         unitmaker->set_latest_block(blocks[51]);
//         unitmaker->set_latest_block(blocks[52]);
//         unitmaker->set_latest_block(blocks[54]);
//         unitmaker->set_latest_block(blocks[55]);
//         ASSERT_EQ(unitmaker->get_highest_height_block()->get_height(), 55);
//         ASSERT_EQ(unitmaker->get_lowest_height_block()->get_height(), 54);
//         ASSERT_EQ(unitmaker->get_latest_blocks().size(), 2);
//     }
// }

// TEST_F(test_unit_maker, blockmaker_verify_latest_blocks) {
//     std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
//     xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();

//     xemptyunit_builder_t builder;
//     xblock_ptr_t genesis_block = builder.build_genesis_block(account1);
//     xassert(!genesis_block->get_block_hash().empty());
//     std::vector<xblock_ptr_t>   blocks;
//     blocks.push_back(genesis_block);

//     xblock_ptr_t prev_block = genesis_block;
//     for (uint64_t height = 1; height < 100; height++) {
//         xblock_consensus_para_t cs_para(xvip2_t{10,10}, prev_block.get());
//         xblock_ptr_t nongenesis_block = builder.build_empty_block(prev_block, cs_para);
//         xassert(nongenesis_block != nullptr);
//         xdatamock_tx::do_mock_signature(nongenesis_block.get());
//         xassert(!nongenesis_block->get_block_hash().empty());
//         blocks.push_back(nongenesis_block);
//         prev_block = nongenesis_block;
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         ASSERT_TRUE(unitmaker->verify_latest_blocks(blocks[0].get(), blocks[0].get(), blocks[0].get()));
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         ASSERT_TRUE(unitmaker->verify_latest_blocks(blocks[1].get(), blocks[0].get(), blocks[0].get()));
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         ASSERT_FALSE(unitmaker->verify_latest_blocks(blocks[2].get(), blocks[1].get(), blocks[0].get()));
//         blocks[1]->set_block_flag(base::enum_xvblock_flag_locked);
//         blocks[2]->set_block_flag(base::enum_xvblock_flag_executed);
//         ASSERT_TRUE(unitmaker->verify_latest_blocks(blocks[2].get(), blocks[1].get(), blocks[0].get()));
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         ASSERT_FALSE(unitmaker->verify_latest_blocks(blocks[3].get(), blocks[2].get(), blocks[1].get()));
//         blocks[2]->set_block_flag(base::enum_xvblock_flag_locked);
//         ASSERT_FALSE(unitmaker->verify_latest_blocks(blocks[3].get(), blocks[2].get(), blocks[1].get()));
//         blocks[3]->set_block_flag(base::enum_xvblock_flag_locked);
//         ASSERT_FALSE(unitmaker->verify_latest_blocks(blocks[3].get(), blocks[2].get(), blocks[1].get()));
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         ASSERT_FALSE(unitmaker->verify_latest_blocks(blocks[10].get(), blocks[8].get(), blocks[7].get()));
//     }
//     {
//         xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//         ASSERT_FALSE(unitmaker->verify_latest_blocks(blocks[11].get(), blocks[10].get(), blocks[9].get()));
//         blocks[9]->set_block_flag(base::enum_xvblock_flag_executed);
//         blocks[9]->set_block_flag(base::enum_xvblock_flag_committed);
//         blocks[9]->set_block_flag(base::enum_xvblock_flag_connected);
//         blocks[10]->set_block_flag(base::enum_xvblock_flag_locked);
//         ASSERT_TRUE(unitmaker->verify_latest_blocks(blocks[11].get(), blocks[10].get(), blocks[9].get()));
//     }
// }



// TEST_F(test_unit_maker, unit_maker_1) {
//     std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
//     xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
//     xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//     ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);
//     ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);
//     ASSERT_FALSE(unitmaker->can_make_next_block());
// }

// TEST_F(test_unit_maker, DISABLED_unit_maker_2) {
//     std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
//     xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
//     std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");

//     xdatamock_tx datamock_tx(resouces, account1);

//     xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
//     ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);

//     auto txs = datamock_tx.generate_transfer_tx(account2, 5);
//     xunit_proposal_input_t proposal_input;
//     proposal_input.set_input_txs(txs);
//     ASSERT_FALSE(unitmaker->can_make_next_block());
//     xblock_consensus_para_t cs_para;
//     cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
//     cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");

//     xunitmaker_result_t result;
//     xblock_ptr_t unit1 = unitmaker->make_proposal(proposal_input, cs_para, result);
//     ASSERT_NE(unit1, nullptr);
//     ASSERT_EQ(unit1->get_height(), 1);
//     ASSERT_EQ(unit1->get_block_class(), base::enum_xvblock_class_light);
//     ASSERT_EQ(unit1->get_txs_count(), 5);
//     xdatamock_tx::do_mock_signature(unit1.get());
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(unit1->get_account()), unit1.get()));

//     ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);
//     ASSERT_TRUE(unitmaker->can_make_next_block());
//     xunit_proposal_input_t proposal_input2;
//     xblock_ptr_t unit2 = unitmaker->make_proposal(proposal_input2, cs_para, result);
//     ASSERT_NE(unit2, nullptr);
//     ASSERT_EQ(unit2->get_height(), 2);
//     ASSERT_EQ(unit2->get_block_class(), base::enum_xvblock_class_nil);
//     xdatamock_tx::do_mock_signature(unit2.get());
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(unit2->get_account()), unit2.get()));

//     ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);
//     ASSERT_TRUE(unitmaker->can_make_next_block());
//     xunit_proposal_input_t proposal_input3;
//     xblock_ptr_t unit3 = unitmaker->make_proposal(proposal_input3, cs_para, result);
//     ASSERT_NE(unit3, nullptr);
//     ASSERT_EQ(unit3->get_height(), 3);
//     ASSERT_EQ(unit3->get_block_class(), base::enum_xvblock_class_nil);
//     xdatamock_tx::do_mock_signature(unit3.get());
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(unit3->get_account()), unit3.get()));

//     ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);
//     ASSERT_FALSE(unitmaker->can_make_next_block());
//     xunit_proposal_input_t proposal_input4;
//     xblock_ptr_t unit4 = unitmaker->make_proposal(proposal_input4, cs_para, result);
//     ASSERT_EQ(unit4, nullptr);

//     auto txs2 = datamock_tx.generate_transfer_tx(account2, 2);
//     ASSERT_EQ(unitmaker->check_latest_state(), xsuccess);
//     xunit_proposal_input_t proposal_input5;
//     proposal_input5.set_input_txs(txs2);
//     xblock_ptr_t unit5 = unitmaker->make_proposal(proposal_input5, cs_para, result);
//     ASSERT_NE(unit5, nullptr);
//     ASSERT_EQ(unit5->get_height(), 4);
//     ASSERT_EQ(unit5->get_block_class(), base::enum_xvblock_class_light);
//     ASSERT_EQ(unit5->get_txs_count(), 2);
//     xdatamock_tx::do_mock_signature(unit5.get());
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(unit5->get_account()), unit5.get()));
// }

