#include "gtest/gtest.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xmock_auth.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xdata/xlightunit.h"
#include "xsync/xsync_store.h"
#include "xsync/xsync_on_demand.h"
#include "xvledger/xunit_proof.h"

#include <stdio.h>

#include <string>

using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;
using namespace top::sync;

class test_unit_proof : public testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
    }
};
#if 0
TEST_F(test_unit_proof, store_and_load) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];

    uint32_t tx_num = 5;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock2 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock3 = mocktable.generate_one_table();

    std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
    _tableblock1->extract_sub_blocks(sub_blocks);

    auto & unit_block = sub_blocks[0];
    auto & unit_account = unit_block->get_account();
    xunit_proof_t unit_proof(unit_block->get_height(), unit_block->get_viewid(), _tableblock3->get_cert());

    top::mock::xmock_auth_t auth{1};
    ASSERT_EQ(unit_proof.verify_unit_block(&auth, unit_block), true);
    std::string unit_proof_str;
    unit_proof.serialize_to(unit_proof_str);
    ASSERT_EQ(blockstore->set_unit_proof(unit_account, unit_proof_str, unit_block->get_height()), true);

    auto unit_proof_str_out = blockstore->get_unit_proof(unit_account, unit_block->get_height());
    ASSERT_EQ(unit_proof_str_out.empty(), false);
    base::xunit_proof_t unit_proof_tmp;
    unit_proof_tmp.serialize_from(unit_proof_str_out);
    ASSERT_EQ(unit_block->get_height(), unit_proof_tmp.get_height());
    ASSERT_EQ(unit_block->get_viewid(), unit_proof_tmp.get_viewid());
    ASSERT_EQ(unit_proof_tmp.verify_unit_block(&auth, unit_block), true);
}


TEST_F(test_unit_proof, check_unit_blocks) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    top::mbus::xmessage_bus_t mbus;
    top::mock::xmock_auth_t auth{1};

    xsync_store_t sync_store("test_node", make_observer(blockstore), nullptr);

    xsync_on_demand_t sync_on_demand("", make_observer(&mbus), make_observer(&auth), &sync_store, nullptr, nullptr, nullptr);

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];

    uint32_t tx_num = 5;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    
    std::vector<xcons_transaction_ptr_t> send_txs2 = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs2);
    xblock_ptr_t _tableblock2 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock3 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock4 = mocktable.generate_one_table();

    std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks1;
    _tableblock1->extract_sub_blocks(sub_blocks1);

    std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks2;
    _tableblock2->extract_sub_blocks(sub_blocks2);

    auto & unit_block1 = sub_blocks1[0];
    auto & unit_block2 = sub_blocks2[0];
    auto & unit_account = unit_block1->get_account();

    xunit_proof_t unit_proof1(unit_block1->get_height(), unit_block1->get_viewid(), _tableblock3->get_cert());
    xunit_proof_t unit_proof2(unit_block2->get_height(), unit_block2->get_viewid(), _tableblock4->get_cert());

    ASSERT_EQ(unit_proof1.verify_unit_block(&auth, unit_block1), true);
    std::string unit_proof_str1;
    unit_proof1.serialize_to(unit_proof_str1);
    ASSERT_EQ(blockstore->set_unit_proof(unit_account, unit_proof_str1, unit_block1->get_height()), true);

    ASSERT_EQ(unit_proof2.verify_unit_block(&auth, unit_block2), true);
    std::string unit_proof_str2;
    unit_proof2.serialize_to(unit_proof_str2);
    ASSERT_EQ(blockstore->set_unit_proof(unit_account, unit_proof_str2, unit_block2->get_height()), true);

    std::vector<data::xblock_ptr_t> unit_blocks;
    unit_blocks.push_back(xblock_t::raw_vblock_to_object_ptr(unit_block1.get()));
    sync_on_demand.check_unit_blocks(unit_blocks);





    auto unit_proof_str_out = blockstore->get_unit_proof(unit_account, unit_block->get_height());
    ASSERT_EQ(unit_proof_str_out.empty(), false);
    base::xunit_proof_t unit_proof_tmp;
    unit_proof_tmp.serialize_from(unit_proof_str_out);
    ASSERT_EQ(unit_block->get_height(), unit_proof_tmp.get_height());
    ASSERT_EQ(unit_block->get_viewid(), unit_proof_tmp.get_viewid());
    ASSERT_EQ(unit_proof_tmp.verify_unit_block(&auth, unit_block), true);
}
#endif
