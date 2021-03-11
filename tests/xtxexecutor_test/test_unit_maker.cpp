#include "gtest/gtest.h"
#include "xtxexecutor/xunit_maker.h"
#include "test_common.hpp"
#include "xstore/xstore_face.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xtransaction_maker.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_tx.hpp"
#include "tests/mock/xcertauth_util.hpp"

using namespace top::txexecutor;
using namespace top::store;
using namespace top::base;
using namespace top::mock;

class test_unit_maker : public testing::Test {
 protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
 public:

};

TEST_F(test_unit_maker, unit_maker_1) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_FALSE(unitmaker->can_make_next_block());
}

TEST_F(test_unit_maker, unit_maker_2) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");

    xdatamock_tx datamock_tx(resouces, account1);

    xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);

    auto txs = datamock_tx.generate_transfer_tx(account2, 5);
    ASSERT_TRUE(unitmaker->push_tx(txs));
    ASSERT_TRUE(unitmaker->can_make_next_block());
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");

    int32_t error_code;
    xblock_ptr_t unit1 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit1, nullptr);
    ASSERT_EQ(unit1->get_height(), 1);
    ASSERT_EQ(unit1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(unit1->get_txs_count(), 5);
    xdatamock_tx::do_mock_signature(unit1.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit1.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_TRUE(unitmaker->can_make_next_block());
    xblock_ptr_t unit2 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit2, nullptr);
    ASSERT_EQ(unit2->get_height(), 2);
    ASSERT_EQ(unit2->get_block_class(), base::enum_xvblock_class_nil);
    xdatamock_tx::do_mock_signature(unit2.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit2.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_TRUE(unitmaker->can_make_next_block());
    xblock_ptr_t unit3 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit3, nullptr);
    ASSERT_EQ(unit3->get_height(), 3);
    ASSERT_EQ(unit3->get_block_class(), base::enum_xvblock_class_nil);
    xdatamock_tx::do_mock_signature(unit3.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit3.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_FALSE(unitmaker->can_make_next_block());
    xblock_ptr_t unit4 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_EQ(unit4, nullptr);
}

TEST_F(test_unit_maker, unit_maker_3) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");

    xdatamock_tx datamock_tx(resouces, account1);
    xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);

    auto txs = datamock_tx.generate_transfer_tx(account2, 5);
    unitmaker->push_tx(txs);
    ASSERT_TRUE(unitmaker->can_make_next_block());
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");
    int32_t error_code;

    xblock_ptr_t unit1 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit1, nullptr);
    ASSERT_EQ(unit1->get_height(), 1);
    ASSERT_EQ(unit1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(unit1->get_txs_count(), 5);
    xdatamock_tx::do_mock_signature(unit1.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit1.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_TRUE(unitmaker->can_make_next_block());
    xblock_ptr_t unit2 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit2, nullptr);
    ASSERT_EQ(unit2->get_height(), 2);
    ASSERT_EQ(unit2->get_block_class(), base::enum_xvblock_class_nil);
    xdatamock_tx::do_mock_signature(unit2.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit2.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_TRUE(unitmaker->can_make_next_block());
    xblock_ptr_t unit3 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit3, nullptr);
    ASSERT_EQ(unit3->get_height(), 3);
    ASSERT_EQ(unit3->get_block_class(), base::enum_xvblock_class_nil);
    xdatamock_tx::do_mock_signature(unit3.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit3.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_FALSE(unitmaker->can_make_next_block());
    xblock_ptr_t unit4 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_EQ(unit4, nullptr);

    auto txs2 = datamock_tx.generate_transfer_tx(account2, 2);
    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    unitmaker->push_tx(txs2);
    xblock_ptr_t unit5 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit5, nullptr);
    ASSERT_EQ(unit5->get_height(), 4);
    ASSERT_EQ(unit5->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(unit5->get_txs_count(), 2);
    xdatamock_tx::do_mock_signature(unit5.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit5.get()));
}

TEST_F(test_unit_maker, unit_maker_contious_1) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");

    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, 10, "random", 10, "extra");
    int32_t error_code;

    xdatamock_tx datamock_tx(resouces, account1);
    xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account1, resouces);
    auto txs = datamock_tx.generate_transfer_tx(account2, 3);

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_TRUE(unitmaker->push_tx(txs));

    xblock_ptr_t unit1 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit1, nullptr);
    xdatamock_tx::do_mock_signature(unit1.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit1.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_TRUE(unitmaker->can_make_next_block());
    xblock_ptr_t unit2 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit2, nullptr);
    xdatamock_tx::do_mock_signature(unit2.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit2.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    xblock_ptr_t unit3 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit3, nullptr);
    xdatamock_tx::do_mock_signature(unit3.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit3.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_EQ(nullptr, unitmaker->make_next_block(cs_para, error_code));

    auto txs2 = datamock_tx.generate_transfer_tx(account2, 2);
    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    unitmaker->push_tx(txs2);
    xblock_ptr_t unit4 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit4, nullptr);
    ASSERT_EQ(unit4->get_height(), 4);
    ASSERT_EQ(unit4->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(unit4->get_txs_count(), 2);
    xdatamock_tx::do_mock_signature(unit4.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit4.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_TRUE(unitmaker->can_make_next_block());
    xblock_ptr_t unit5 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit5, nullptr);
    xdatamock_tx::do_mock_signature(unit5.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit5.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    xblock_ptr_t unit6 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit6, nullptr);
    xdatamock_tx::do_mock_signature(unit6.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit6.get()));

    auto txs3 = datamock_tx.generate_transfer_tx(account2, 3);
    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    unitmaker->push_tx(txs3);
    xblock_ptr_t unit7 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit7, nullptr);
    ASSERT_EQ(unit7->get_height(), 7);
    ASSERT_EQ(unit7->get_block_class(), base::enum_xvblock_class_light);
    xdatamock_tx::do_mock_signature(unit7.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit7.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    ASSERT_TRUE(unitmaker->can_make_next_block());
    xblock_ptr_t unit8 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit8, nullptr);
    xdatamock_tx::do_mock_signature(unit8.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit8.get()));

    ASSERT_EQ(unitmaker->default_check_latest_state(), xsuccess);
    xblock_ptr_t unit9 = unitmaker->make_next_block(cs_para, error_code);
    ASSERT_NE(unit9, nullptr);
    xdatamock_tx::do_mock_signature(unit9.get());
    ASSERT_TRUE(resouces->get_blockstore()->store_block(unit9.get()));
}
