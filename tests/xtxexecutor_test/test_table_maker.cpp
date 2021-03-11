#include "gtest/gtest.h"
#include "xtxexecutor/xtable_maker.h"
#include "xstore/xstore_face.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xtransaction_maker.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_tx.hpp"
#include "tests/mock/xcertauth_util.hpp"
#include "test_common.hpp"

using namespace top::txexecutor;
using namespace top::store;
using namespace top::base;
using namespace top::mock;

class test_table_maker : public testing::Test {
 protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
 public:

};

TEST_F(test_table_maker, table_maker_1) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");

    xdatamock_tx datamock_tx(resouces, account1);

    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);

    // ASSERT_FALSE(table_maker->can_make_next_block());
}

TEST_F(test_table_maker, table_maker_2) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string dstaccount = xblocktool_t::make_address_user_account("222222222222233333");

    xdatamock_tx datamock_account1(resouces, account1);

    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);

    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);
    ASSERT_TRUE(table_maker->push_tx(account1, txs1));
    ASSERT_TRUE(table_maker->can_make_next_block());

    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");
    int32_t error_code;
    xblock_ptr_t table1 = table_maker->make_next_block(cs_para, error_code);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_NE(table1, nullptr);
    ASSERT_EQ(table1->get_height(), 1);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 1);
    ASSERT_EQ(table1->get_txs_count(), 2);

    ASSERT_TRUE(resouces->get_blockstore()->store_block(table1.get()));

    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
    ASSERT_EQ(table_maker->get_proposal_prev_block()->get_height(), 1);
    ASSERT_EQ(table_maker->get_lock_block()->get_height(), 0);
    ASSERT_TRUE(table_maker->can_make_next_block());
    xblock_ptr_t table2 = table_maker->make_next_block(cs_para, error_code);
    ASSERT_NE(table2, nullptr);
    xdatamock_tx::do_mock_signature(table2.get());
    ASSERT_NE(table2, nullptr);
    ASSERT_EQ(table2->get_height(), 2);
    ASSERT_EQ(table2->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table2->get_tableblock_units(false).size(), 1);
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table2.get()));

    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
    xblock_ptr_t table3 = table_maker->make_next_block(cs_para, error_code);
    xdatamock_tx::do_mock_signature(table3.get());
    ASSERT_NE(table3, nullptr);
    ASSERT_EQ(table3->get_height(), 3);
    ASSERT_EQ(table3->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table3->get_tableblock_units(false).size(), 1);
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table3.get()));

    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
    ASSERT_FALSE(table_maker->can_make_next_block());
    xblock_ptr_t table4 = table_maker->make_next_block(cs_para, error_code);
    ASSERT_EQ(table4, nullptr);
}

TEST_F(test_table_maker, table_maker_3) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");
    std::string dstaccount = xblocktool_t::make_address_user_account("222222222222233333");

    xdatamock_tx datamock_account1(resouces, account1);
    xdatamock_tx datamock_account2(resouces, account2);

    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);

    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);
    auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 2);
    ASSERT_TRUE(table_maker->push_tx(account1, txs1));
    ASSERT_TRUE(table_maker->push_tx(account2, txs2));
    ASSERT_TRUE(table_maker->can_make_next_block());

    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");
    int32_t error_code;
    xblock_ptr_t table1 = table_maker->make_next_block(cs_para, error_code);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_NE(table1, nullptr);
    ASSERT_EQ(table1->get_height(), 1);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 4);

    ASSERT_TRUE(resouces->get_blockstore()->store_block(table1.get()));

    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
    xblock_ptr_t table2 = table_maker->make_next_block(cs_para, error_code);
    xdatamock_tx::do_mock_signature(table2.get());
    ASSERT_NE(table2, nullptr);
    ASSERT_EQ(table2->get_height(), 2);
    ASSERT_EQ(table2->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table2.get()));

    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
    xblock_ptr_t table3 = table_maker->make_next_block(cs_para, error_code);
    xdatamock_tx::do_mock_signature(table3.get());
    ASSERT_NE(table3, nullptr);
    ASSERT_EQ(table3->get_height(), 3);
    ASSERT_EQ(table3->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table3.get()));

    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
    ASSERT_FALSE(table_maker->can_make_next_block());
    xblock_ptr_t table4 = table_maker->make_next_block(cs_para, error_code);
    ASSERT_EQ(table4, nullptr);
}

TEST_F(test_table_maker, table_maker_update_latest_blocks_1) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();

    xdatamock_table mocktable(1, 1, resouces->get_store());
    mocktable.genrate_table_chain(3);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    for (auto & table : tables) {
        ASSERT_TRUE(resouces->get_blockstore()->store_block(table.get()));
    }
    auto latest_cert_block = resouces->get_blockstore()->get_latest_cert_block(mocktable.get_account());
    ASSERT_EQ(latest_cert_block->get_height(), 3);

    std::string taccount = mocktable.get_account();
    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
}

TEST_F(test_table_maker, table_maker_update_latest_blocks_2) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();

    xdatamock_table mocktable(1, 1, resouces->get_store());
    mocktable.genrate_table_chain(100);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    for (auto & table : tables) {
        ASSERT_TRUE(resouces->get_blockstore()->store_block(table.get()));
    }
    auto latest_cert_block = resouces->get_blockstore()->get_latest_cert_block(mocktable.get_account());
    ASSERT_EQ(latest_cert_block->get_height(), 100);

    std::string taccount = mocktable.get_account();
    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
}

TEST_F(test_table_maker, table_maker_roll_back_1) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string dstaccount = xblocktool_t::make_address_user_account("222222222222233333");

    xdatamock_tx datamock_account1(resouces, account1);

    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);


    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);

    xblock_ptr_t first_table;
    xblock_ptr_t second_table;
    {
        xblock_consensus_para_t cs_para;
        cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
        cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");
        int32_t error_code;

        ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
        ASSERT_TRUE(table_maker->push_tx(account1, txs1));
        ASSERT_TRUE(table_maker->can_make_next_block());
        first_table = table_maker->make_next_block(cs_para, error_code);
        xdatamock_tx::do_mock_signature(first_table.get());
    }
    {
        xblock_consensus_para_t cs_para;
        cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 20, 10, 10);
        cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");
        int32_t error_code;

        ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
        ASSERT_TRUE(table_maker->push_tx(account1, txs1));
        ASSERT_TRUE(table_maker->can_make_next_block());
        second_table = table_maker->make_next_block(cs_para, error_code);
        xdatamock_tx::do_mock_signature(second_table.get());
    }

    {
        ASSERT_TRUE(resouces->get_blockstore()->store_block(first_table.get()));
        ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
        ASSERT_EQ(table_maker->get_proposal_prev_block()->get_block_hash(), first_table->get_block_hash());
    }
    {
        ASSERT_TRUE(resouces->get_blockstore()->store_block(second_table.get()));
        ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
        ASSERT_EQ(table_maker->get_proposal_prev_block()->get_block_hash(), second_table->get_block_hash());
    }
}

TEST_F(test_table_maker, table_maker_make_proposal_1) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");
    std::string dstaccount = xblocktool_t::make_address_user_account("222222222222233333");

    xdatamock_tx datamock_account1(resouces, account1);
    xdatamock_tx datamock_account2(resouces, account2);

    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);

    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);
    auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 2);

    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");

    base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
    xtablemaker_para_t table_para(latest_blocks);
    table_para.set_unitmaker_txs(account1, txs1);
    table_para.set_unitmaker_txs(account2, txs2);

    xtablemaker_result_t result;
    xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
    ASSERT_NE(table1, nullptr);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_EQ(table1->get_height(), 1);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 4);

    ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));

    ASSERT_TRUE(resouces->get_blockstore()->store_block(table1.get()));

    base::xblock_mptrs latest_blocks2 = resouces->get_blockstore()->get_latest_blocks(taccount);
    xtablemaker_para_t table_para2(latest_blocks2);
    table_para2.set_unitmaker_txs(account1, txs1);
    table_para2.set_unitmaker_txs(account2, txs2);
    ASSERT_NE(xsuccess, table_maker->verify_proposal(table1.get(), table_para2, cs_para));
}

TEST_F(test_table_maker, table_maker_make_proposal_2) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");
    std::string dstaccount = xblocktool_t::make_address_user_account("222222222222233333");

    xdatamock_tx datamock_account1(resouces, account1);
    xdatamock_tx datamock_account2(resouces, account2);

    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);

    {
    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);
    auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 2);
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");

    base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
    xtablemaker_para_t table_para(latest_blocks);
    table_para.set_unitmaker_txs(account1, txs1);
    table_para.set_unitmaker_txs(account2, txs2);

    xtablemaker_result_t result;
    xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
    ASSERT_NE(table1, nullptr);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_EQ(table1->get_height(), 1);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 4);

    ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table1.get()));
    }

    {
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");

    base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
    xtablemaker_para_t table_para(latest_blocks);

    xtablemaker_result_t result;
    xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
    ASSERT_NE(table1, nullptr);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_EQ(table1->get_height(), 2);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 0);

    ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table1.get()));
    }

    {
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");

    base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
    xtablemaker_para_t table_para(latest_blocks);

    xtablemaker_result_t result;
    xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
    ASSERT_NE(table1, nullptr);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_EQ(table1->get_height(), 3);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 0);

    ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table1.get()));
    }

    {
    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 3);
    auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 3);
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");

    base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
    xtablemaker_para_t table_para(latest_blocks);
    table_para.set_unitmaker_txs(account1, txs1);
    table_para.set_unitmaker_txs(account2, txs2);

    xtablemaker_result_t result;
    xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
    ASSERT_NE(table1, nullptr);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_EQ(table1->get_height(), 4);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 6);

    ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table1.get()));
    }
}

TEST_F(test_table_maker, table_maker_make_proposal_3) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");
    std::string dstaccount = xblocktool_t::make_address_user_account("222222222222233333");

    xdatamock_tx datamock_account1(resouces, account1);
    xdatamock_tx datamock_account2(resouces, account2);

    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);

    {
    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);
    auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 2);
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");

    base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
    xtablemaker_para_t table_para(latest_blocks);
    table_para.set_unitmaker_txs(account1, txs1);
    table_para.set_unitmaker_txs(account2, txs2);

    xtablemaker_result_t result;
    xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
    ASSERT_NE(table1, nullptr);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_EQ(table1->get_height(), 1);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 4);

    ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table1.get()));
    }

    {
    auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);
    auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 2);
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");

    base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
    xtablemaker_para_t table_para(latest_blocks);
    table_para.set_unitmaker_txs(account1, txs1);
    table_para.set_unitmaker_txs(account2, txs2);

    xtablemaker_result_t result;
    xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
    ASSERT_NE(table1, nullptr);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_EQ(table1->get_height(), 2);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 0);

    auto & units = table1->get_tableblock_units(true);
    ASSERT_EQ(units.size(), 2);
    ASSERT_EQ(units[0]->get_block_class(), base::enum_xvblock_class_nil);
    ASSERT_EQ(units[1]->get_block_class(), base::enum_xvblock_class_nil);

    ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
    ASSERT_TRUE(resouces->get_blockstore()->store_block(table1.get()));
    }

}
