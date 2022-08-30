#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
// TODO(jimmy) #include "xbase/xvledger.h"

#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xmessage_bus.h"

// #include "test_blockmock.hpp"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "xblockstore/src/xvblockhub.h"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;

class test_block_connected : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_block_connected, block_connect_discrete) {
    mock::xvchain_creator creator(true);
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t count = 19;
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count, blockstore);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    mocktable.store_genesis_units(blockstore);
    xassert(tables.size() == count + 1);

    std::string address = mocktable.get_account();
    xvaccount_t account(address);

    uint64_t missing_height = 10;
    for (uint64_t i = 1; i < missing_height; i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(blockstore->store_block(account, curr_block));
    }
    EXPECT_EQ(missing_height - 1 - 2, blockstore->update_get_latest_cp_connected_block_height(account));

    for (uint64_t i = missing_height + 1; i <= count; i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(blockstore->store_block(account, curr_block));
    }

    base::xauto_ptr<xvblock_t> latest_block = blockstore->get_latest_connected_block(address);
    EXPECT_EQ(latest_block->get_height(), missing_height - 1 - 2);
    EXPECT_EQ(missing_height - 1 - 2, blockstore->update_get_latest_cp_connected_block_height(account));

    auto curr_block = tables[missing_height].get();
    ASSERT_TRUE(blockstore->store_block(account, curr_block));

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    EXPECT_EQ(connected_block->get_height(), count - 2);
    EXPECT_EQ(count - 2, blockstore->update_get_latest_cp_connected_block_height(account));
}

TEST_F(test_block_connected, block_connect_discrete_1) {
    mock::xvchain_creator creator(true);
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t count = mock::xdatamock_table::get_full_table_interval_count() + 20;
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count, blockstore);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    mocktable.store_genesis_units(blockstore);
    xassert(tables.size() == count + 1);

    std::string address = mocktable.get_account();
    xvaccount_t account(address);

    uint64_t genesis_continous_height = 10;
    for (uint64_t i = 1; i <= genesis_continous_height; i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(blockstore->store_block(account, curr_block));
    }

    {
        auto connected_block = blockstore->get_latest_connected_block(account);
        ASSERT_TRUE(connected_block != nullptr);
        EXPECT_EQ(connected_block->get_height(), genesis_continous_height - 2);
    }

    uint64_t full_height = mock::xdatamock_table::get_full_table_interval_count();
    for (uint64_t i = full_height; i <= full_height + 1; i++) {
        ASSERT_TRUE(blockstore->store_block(account, tables[i].get()));
    }

    {
        auto latest_committed_full_block = blockstore->get_latest_committed_full_block(account);
        ASSERT_TRUE(latest_committed_full_block != nullptr);
        EXPECT_EQ(latest_committed_full_block->get_height(), 0);

        auto connected_block = blockstore->get_latest_connected_block(account);
        ASSERT_TRUE(connected_block != nullptr);
        EXPECT_EQ(connected_block->get_height(), genesis_continous_height - 2);
    }

    ASSERT_TRUE(blockstore->store_block(account, tables[full_height + 2].get()));

    {
        auto genesis_block = blockstore->get_genesis_block(account);
        ASSERT_TRUE(genesis_block != nullptr);
        EXPECT_EQ(genesis_block->get_height(), 0);

        auto latest_committed_full_block = blockstore->get_latest_committed_full_block(account);
        ASSERT_TRUE(latest_committed_full_block != nullptr);
        EXPECT_EQ(latest_committed_full_block->get_height(), full_height);

        auto connected_block = blockstore->get_latest_connected_block(account);
        ASSERT_TRUE(connected_block != nullptr);
        EXPECT_EQ(connected_block->get_height(), full_height);
    }
}

TEST_F(test_block_connected, store_block_in_unorder_1) {
    mock::xvchain_creator creator(true);
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t count = 10;
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count, blockstore);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == count + 1);

    std::string address = mocktable.get_account();
    xvaccount_t account(address);

    ASSERT_TRUE(blockstore->store_block(account, tables[1].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[2].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[3].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[4].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[7].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[8].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[9].get()));
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 2);

    ASSERT_TRUE(blockstore->store_block(account, tables[5].get()));
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);

    ASSERT_TRUE(blockstore->store_block(account, tables[6].get()));
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
}

TEST_F(test_block_connected, store_block_in_unorder_2) {
    mock::xvchain_creator creator(true);
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t count = 10;
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count, blockstore);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == count + 1);

    std::string address = mocktable.get_account();
    xvaccount_t account(address);

    ASSERT_TRUE(blockstore->store_block(account, tables[1].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[2].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[3].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[5].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[7].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[8].get()));
    ASSERT_TRUE(blockstore->store_block(account, tables[9].get()));
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 1);

    ASSERT_TRUE(blockstore->store_block(account, tables[4].get()));
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 3);

    ASSERT_TRUE(blockstore->store_block(account, tables[6].get()));
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
}
#if 0 // TODO(jimmy) fail need fix future
TEST_F(test_block_connected, store_block_in_order_1) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t count = mock::xdatamock_table::get_full_table_interval_count() + 20;
    const uint64_t full_height = mock::xdatamock_table::get_full_table_interval_count();
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count, blockstore);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == count + 1);

    std::string address = mocktable.get_account();
    xvaccount_t account(address);

    for (uint64_t i = 1; i < count; i++) {
        ASSERT_TRUE(blockstore->store_block(account, tables[i].get()));
        ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), i - 1);
        ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_executed_block(address)->get_height(), i >= 2 ? i - 2 : 0);

        auto latest_committed_full_block = blockstore->get_latest_committed_full_block(account);
        ASSERT_TRUE(latest_committed_full_block != nullptr);
        EXPECT_EQ(latest_committed_full_block->get_height(), (i >= (full_height + 2)) ? full_height: 0);
    }

    ASSERT_FALSE(blockstore->store_block(account, tables[3].get()));
}
#endif
#if 0  // this test need 6*80s, cache in blockstore will be released after 60s
TEST_F(test_block_connected, store_block_in_order_sleep_BENCH) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t count = 6;
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count, blockstore);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == count + 1);

    std::string address = mocktable.get_account();
    xvaccount_t account(address);

    for (uint64_t i = 1; ; i++) {
        ASSERT_TRUE(blockstore->store_block(account, tables[i].get()));
        ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), i - 1);
        ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_executed_block(address)->get_height(), i >= 2 ? i - 2 : 0);

        if (i >= count) {
            break;
        }

        // wait for block cache released from memory
        sleep(80);
    }
}
#endif

#if 0
TEST_F(test_block_connected, store_block_in_order_same_height_different_viewid_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_shard_table_account(1);
    uint64_t count = 30;

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock_with_cert_flag(genesis_block.get());
    ASSERT_TRUE(blockstore->store_block(block1.get()));
    base::xauto_ptr<base::xvblock_t> block2 = test_blocktuil::create_next_emptyblock_with_cert_flag(block1.get());
    ASSERT_TRUE(blockstore->store_block(block2.get()));
    base::xauto_ptr<base::xvblock_t> block3 = test_blocktuil::create_next_emptyblock_with_cert_flag(block2.get());
    ASSERT_TRUE(blockstore->store_block(block3.get()));
    base::xauto_ptr<base::xvblock_t> block4 = test_blocktuil::create_next_emptyblock_with_cert_flag(block3.get());
    ASSERT_TRUE(blockstore->store_block(block4.get()));

    base::xauto_ptr<base::xvblock_t> block5_1 = test_blocktuil::create_next_emptyblock_with_cert_flag(block4.get());
    ASSERT_TRUE(blockstore->store_block(block5_1.get()));
    base::xauto_ptr<base::xvblock_t> block5_2 = xblocktool_t::create_next_emptyblock(block4.get());
    block5_2->get_cert()->set_viewid(block5_1->get_viewid()+1);
    test_blocktuil::do_block_consensus_with_cert_flag(block5_2.get());

    base::xauto_ptr<base::xvblock_t> block6 = test_blocktuil::create_next_emptyblock_with_cert_flag(block5_2.get());
    ASSERT_TRUE(blockstore->store_block(block6.get()));
    ASSERT_EQ(block5_1->get_height(), blockstore->get_latest_current_block(address)->get_height());

    ASSERT_FALSE(block5_1->check_block_flag(base::enum_xvblock_flag_locked));
    ASSERT_FALSE(block6->check_block_flag(base::enum_xvblock_flag_locked));
    ASSERT_FALSE(block5_2->check_block_flag(base::enum_xvblock_flag_locked));

    ASSERT_TRUE(blockstore->store_block(block5_2.get()));  // must firstly set prev block
    ASSERT_TRUE(block5_2->check_block_flag(base::enum_xvblock_flag_locked));
    ASSERT_EQ(block6->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block7 = test_blocktuil::create_next_emptyblock_with_cert_flag(block6.get());
    ASSERT_TRUE(blockstore->store_block(block7.get()));
    ASSERT_EQ(block7->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block8 = test_blocktuil::create_next_emptyblock_with_cert_flag(block7.get());
    ASSERT_TRUE(blockstore->store_block(block8.get()));
    ASSERT_EQ(block8->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block9 = test_blocktuil::create_next_emptyblock_with_cert_flag(block8.get());
    ASSERT_TRUE(blockstore->store_block(block9.get()));
    ASSERT_EQ(block9->get_height(), blockstore->get_latest_current_block(address)->get_height());
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_executed_block(address)->get_height(), 7);
}

TEST_F(test_block_connected, store_block_in_order_same_height_different_viewid_2) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_shard_table_account(1);
    uint64_t count = 30;

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock(genesis_block.get());
    ASSERT_TRUE(blockstore->store_block(block1.get()));
    base::xauto_ptr<base::xvblock_t> block2 = test_blocktuil::create_next_emptyblock(block1.get());
    ASSERT_TRUE(blockstore->store_block(block2.get()));
    base::xauto_ptr<base::xvblock_t> block3 = test_blocktuil::create_next_emptyblock(block2.get());
    ASSERT_TRUE(blockstore->store_block(block3.get()));
    base::xauto_ptr<base::xvblock_t> block4 = test_blocktuil::create_next_emptyblock(block3.get());
    ASSERT_TRUE(blockstore->store_block(block4.get()));

    base::xauto_ptr<base::xvblock_t> block5_1 = test_blocktuil::create_next_emptyblock_with_cert_flag(block4.get());
    ASSERT_TRUE(blockstore->store_block(block5_1.get()));
    base::xauto_ptr<base::xvblock_t> block5_2 = xblocktool_t::create_next_emptyblock(block4.get());
    block5_2->get_cert()->set_viewid(block5_1->get_viewid()+1);
    test_blocktuil::do_block_consensus_with_cert_flag(block5_2.get());
    ASSERT_TRUE(blockstore->store_block(block5_2.get()));

    base::xauto_ptr<base::xvblock_t> block6 = test_blocktuil::create_next_emptyblock(block5_2.get());
    ASSERT_TRUE(blockstore->store_block(block6.get()));
    ASSERT_EQ(block6->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block7 = test_blocktuil::create_next_emptyblock(block6.get());
    ASSERT_TRUE(blockstore->store_block(block7.get()));
    ASSERT_EQ(block7->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block8 = test_blocktuil::create_next_emptyblock(block7.get());
    ASSERT_TRUE(blockstore->store_block(block8.get()));
    ASSERT_EQ(block8->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block9 = test_blocktuil::create_next_emptyblock(block8.get());
    ASSERT_TRUE(blockstore->store_block(block9.get()));
    ASSERT_EQ(block9->get_height(), blockstore->get_latest_current_block(address)->get_height());
}

TEST_F(test_block_connected, store_block_in_discontinuous_order_same_height_different_viewid_2_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_shard_table_account(1);
    uint64_t count = 30;

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock(genesis_block.get());
    // ASSERT_TRUE(blockstore->store_block(block1.get()));
    base::xauto_ptr<base::xvblock_t> block2 = test_blocktuil::create_next_emptyblock(block1.get());
    // ASSERT_TRUE(blockstore->store_block(block2.get()));
    base::xauto_ptr<base::xvblock_t> block3 = test_blocktuil::create_next_emptyblock(block2.get());
    ASSERT_TRUE(blockstore->store_block(block3.get()));
    base::xauto_ptr<base::xvblock_t> block4 = test_blocktuil::create_next_emptyblock(block3.get());
    ASSERT_TRUE(blockstore->store_block(block4.get()));

    base::xauto_ptr<base::xvblock_t> block5_1 = test_blocktuil::create_next_emptyblock_with_cert_flag(block4.get());
    ASSERT_TRUE(blockstore->store_block(block5_1.get()));
    base::xauto_ptr<base::xvblock_t> block5_2 = xblocktool_t::create_next_emptyblock(block4.get());
    block5_2->get_cert()->set_viewid(block5_1->get_viewid()+1);
    test_blocktuil::do_block_consensus_with_cert_flag(block5_2.get());
    ASSERT_TRUE(blockstore->store_block(block5_2.get()));

    base::xauto_ptr<base::xvblock_t> block6 = test_blocktuil::create_next_emptyblock(block5_2.get());
    ASSERT_TRUE(blockstore->store_block(block6.get()));
    ASSERT_EQ(0, blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block7 = test_blocktuil::create_next_emptyblock(block6.get());
    ASSERT_TRUE(blockstore->store_block(block7.get()));
    ASSERT_EQ(0, blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block8 = test_blocktuil::create_next_emptyblock(block7.get());
    ASSERT_TRUE(blockstore->store_block(block8.get()));
    ASSERT_EQ(0, blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block9 = test_blocktuil::create_next_emptyblock(block8.get());
    ASSERT_TRUE(blockstore->store_block(block9.get()));
    ASSERT_EQ(0, blockstore->get_latest_current_block(address)->get_height());
}

TEST_F(test_block_connected, store_block_in_discontinuous_order_same_height_different_viewid_2_2) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_shard_table_account(1);
    uint64_t count = 30;

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock_with_cert_flag(genesis_block.get());
    // ASSERT_TRUE(blockstore->store_block(block1.get()));
    base::xauto_ptr<base::xvblock_t> block2 = test_blocktuil::create_next_emptyblock_with_cert_flag(block1.get());
    // ASSERT_TRUE(blockstore->store_block(block2.get()));
    base::xauto_ptr<base::xvblock_t> block3 = test_blocktuil::create_next_emptyblock_with_cert_flag(block2.get());
    ASSERT_TRUE(blockstore->store_block(block3.get()));
    base::xauto_ptr<base::xvblock_t> block4 = test_blocktuil::create_next_emptyblock_with_cert_flag(block3.get());
    ASSERT_TRUE(blockstore->store_block(block4.get()));

    base::xauto_ptr<base::xvblock_t> block5_1 = test_blocktuil::create_next_emptyblock_with_cert_flag(block4.get());
    ASSERT_TRUE(blockstore->store_block(block5_1.get()));
    base::xauto_ptr<base::xvblock_t> block5_2 = xblocktool_t::create_next_emptyblock(block4.get());
    block5_2->get_cert()->set_viewid(block5_1->get_viewid()+1);
    test_blocktuil::do_block_consensus_with_cert_flag(block5_2.get());
    ASSERT_TRUE(blockstore->store_block(block5_2.get()));

    base::xauto_ptr<base::xvblock_t> block6 = test_blocktuil::create_next_emptyblock_with_cert_flag(block5_2.get());
    ASSERT_TRUE(blockstore->store_block(block6.get()));
    ASSERT_EQ(0, blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block7 = test_blocktuil::create_next_emptyblock_with_cert_flag(block6.get());
    ASSERT_TRUE(blockstore->store_block(block7.get()));
    ASSERT_EQ(0, blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block8 = test_blocktuil::create_next_emptyblock_with_cert_flag(block7.get());
    ASSERT_TRUE(blockstore->store_block(block8.get()));
    ASSERT_EQ(0, blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block9 = test_blocktuil::create_next_emptyblock_with_cert_flag(block8.get());
    ASSERT_TRUE(blockstore->store_block(block9.get()));
    ASSERT_EQ(0, blockstore->get_latest_current_block(address)->get_height());
}

TEST_F(test_block_connected, store_block_in_order_modify_blockflag_outside_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_shard_table_account(1);
    uint64_t count = 30;

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock(genesis_block.get());
    ASSERT_TRUE(blockstore->store_block(block1.get()));
    base::xauto_ptr<base::xvblock_t> block2 = test_blocktuil::create_next_emptyblock(block1.get());
    ASSERT_TRUE(blockstore->store_block(block2.get()));
    base::xauto_ptr<base::xvblock_t> block3 = test_blocktuil::create_next_emptyblock(block2.get());
    ASSERT_TRUE(blockstore->store_block(block3.get()));
    base::xauto_ptr<base::xvblock_t> block4 = test_blocktuil::create_next_emptyblock(block3.get());
    ASSERT_TRUE(blockstore->store_block(block4.get()));
    base::xauto_ptr<base::xvblock_t> block5 = test_blocktuil::create_next_emptyblock(block4.get());
    ASSERT_TRUE(blockstore->store_block(block5.get()));
    base::xauto_ptr<base::xvblock_t> block5_in_blockstore = blockstore->get_latest_cert_block(address);
    base::xauto_ptr<base::xvblock_t> block4_in_blockstore = blockstore->get_latest_locked_block(address);
    block5_in_blockstore->set_block_flag(base::enum_xvblock_flag_locked);
    block4_in_blockstore->set_block_flag(base::enum_xvblock_flag_committed);
    base::xauto_ptr<base::xvblock_t> block6 = test_blocktuil::create_next_emptyblock(block5.get());
    block6->set_block_flag(base::enum_xvblock_flag_locked);
    ASSERT_TRUE(blockstore->store_block(block6.get()));
    ASSERT_EQ(block6->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block7 = test_blocktuil::create_next_emptyblock(block6.get());
    ASSERT_TRUE(blockstore->store_block(block7.get()));
    ASSERT_EQ(block7->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block8 = test_blocktuil::create_next_emptyblock(block7.get());
    ASSERT_TRUE(blockstore->store_block(block8.get()));
    ASSERT_EQ(block8->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block9 = test_blocktuil::create_next_emptyblock(block8.get());
    ASSERT_TRUE(blockstore->store_block(block9.get()));
    ASSERT_EQ(block9->get_height(), blockstore->get_latest_current_block(address)->get_height());
}

TEST_F(test_block_connected, store_block_in_order_modify_blockflag_outside_2) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_shard_table_account(1);
    uint64_t count = 30;

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock_with_cert_flag(genesis_block.get());
    ASSERT_TRUE(blockstore->store_block(block1.get()));
    base::xauto_ptr<base::xvblock_t> block2 = test_blocktuil::create_next_emptyblock_with_cert_flag(block1.get());
    ASSERT_TRUE(blockstore->store_block(block2.get()));
    base::xauto_ptr<base::xvblock_t> block3 = test_blocktuil::create_next_emptyblock_with_cert_flag(block2.get());
    ASSERT_TRUE(blockstore->store_block(block3.get()));
    base::xauto_ptr<base::xvblock_t> block4 = test_blocktuil::create_next_emptyblock_with_cert_flag(block3.get());
    ASSERT_TRUE(blockstore->store_block(block4.get()));
    base::xauto_ptr<base::xvblock_t> block5 = test_blocktuil::create_next_emptyblock_with_cert_flag(block4.get());
    ASSERT_TRUE(blockstore->store_block(block5.get()));
    base::xauto_ptr<base::xvblock_t> block5_in_blockstore = blockstore->get_latest_cert_block(address);
    base::xauto_ptr<base::xvblock_t> block4_in_blockstore = blockstore->get_latest_locked_block(address);
    base::xauto_ptr<base::xvblock_t> block3_in_blockstore = blockstore->get_latest_committed_block(address);
    ASSERT_EQ(block5_in_blockstore->get_height(), 5);
    ASSERT_EQ(block4_in_blockstore->get_height(), 4);
    ASSERT_EQ(block3_in_blockstore->get_height(), 3);
    ASSERT_EQ(block5->get_height(), blockstore->get_latest_current_block(address)->get_height());
    block4_in_blockstore->set_block_flag(base::enum_xvblock_flag_committed);
    block5_in_blockstore->set_block_flag(base::enum_xvblock_flag_locked);
    block5_in_blockstore->set_block_flag(base::enum_xvblock_flag_committed);
    ASSERT_EQ(block5->get_height(), blockstore->get_latest_current_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block6 = test_blocktuil::create_next_emptyblock_with_cert_flag(block5.get());
    ASSERT_TRUE(blockstore->store_block(block6.get()));
    ASSERT_EQ(block6->get_height(), blockstore->get_latest_current_block(address)->get_height());
    ASSERT_EQ(block6->get_height(), blockstore->get_latest_cert_block(address)->get_height());
    ASSERT_EQ(block5->get_height(), blockstore->get_latest_locked_block(address)->get_height());
    ASSERT_EQ(block5->get_height(), blockstore->get_latest_executed_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block7 = test_blocktuil::create_next_emptyblock_with_cert_flag(block6.get());
    ASSERT_TRUE(blockstore->store_block(block7.get()));
    ASSERT_EQ(block7->get_height(), blockstore->get_latest_current_block(address)->get_height());
    ASSERT_EQ(block7->get_height(), blockstore->get_latest_cert_block(address)->get_height());
    ASSERT_EQ(block6->get_height(), blockstore->get_latest_locked_block(address)->get_height());
    ASSERT_EQ(block5->get_height(), blockstore->get_latest_executed_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block8 = test_blocktuil::create_next_emptyblock_with_cert_flag(block7.get());
    ASSERT_TRUE(blockstore->store_block(block8.get()));
    ASSERT_EQ(block8->get_height(), blockstore->get_latest_current_block(address)->get_height());
    ASSERT_EQ(block8->get_height(), blockstore->get_latest_cert_block(address)->get_height());
    ASSERT_EQ(block7->get_height(), blockstore->get_latest_locked_block(address)->get_height());
    ASSERT_EQ(block6->get_height(), blockstore->get_latest_executed_block(address)->get_height());

    base::xauto_ptr<base::xvblock_t> block9 = test_blocktuil::create_next_emptyblock(block8.get());
    ASSERT_TRUE(blockstore->store_block(block9.get()));
    ASSERT_EQ(block9->get_height(), blockstore->get_latest_current_block(address)->get_height());
    ASSERT_EQ(block9->get_height(), blockstore->get_latest_cert_block(address)->get_height());
    ASSERT_EQ(block9->get_height(), blockstore->get_latest_locked_block(address)->get_height());
    ASSERT_EQ(block9->get_height(), blockstore->get_latest_executed_block(address)->get_height());
}

TEST_F(test_block_connected, store_block_disorder_highqc_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xauto_ptr<base::xvblock_t> genesis_block = test_blocktuil::create_genesis_empty_unit(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock_with_cert_flag(genesis_block.get());
    base::xauto_ptr<base::xvblock_t> block2 = test_blocktuil::create_next_emptyblock_with_cert_flag(block1.get());
    base::xauto_ptr<base::xvblock_t> block3 = test_blocktuil::create_next_emptyblock_with_cert_flag(block2.get());
    base::xauto_ptr<base::xvblock_t> block4 = test_blocktuil::create_next_emptyblock_with_cert_flag(block3.get());
    base::xauto_ptr<base::xvblock_t> block5 = test_blocktuil::create_next_emptyblock_with_cert_flag(block4.get());

    {
        base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));

        ASSERT_TRUE(blockstore->store_block(block5.get()));
        ASSERT_TRUE(blockstore->store_block(block3.get()));

        auto latest_cert_block = blockstore->get_latest_cert_block(address);
        ASSERT_EQ(latest_cert_block->get_height(), block5->get_height());
        ASSERT_EQ(latest_cert_block->get_block_hash(), block5->get_block_hash());
    }
    {
        base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
        auto latest_cert_block = blockstore->get_latest_cert_block(address);
        ASSERT_EQ(latest_cert_block->get_height(), block5->get_height());
        ASSERT_EQ(latest_cert_block->get_block_hash(), block5->get_block_hash());
    }
}
#endif
#if 0  // TODO(jimmy) xacctmeta_t is deleted
TEST_F(test_block_connected, latest_connect_update_1_BENCH) {  // take 20s
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();    

    uint64_t count = 500;
    mock::xdatamock_table mocktable;
    std::string table_addr = mocktable.get_account();
    // blockstore->reset_cache_timeout(mocktable, 1000); // idle time change to 1s TODO(jimmy)
    std::string meta_path = "0/" + table_addr + "/meta";
    base::xvdbstore_t* xvdb_ptr = base::xvchain_t::instance().get_xdbstore();

    mocktable.genrate_table_chain(count, blockstore);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == count + 1);
    uint64_t cert_height = (uint64_t)tables.size() - 1;
    uint64_t connect_height = cert_height - 2;

    for (auto & block : tables) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    {
        const std::string meta_value = xvdb_ptr->get_value(meta_path);
        base::xauto_ptr<store::xacctmeta_t> meta_obj = new store::xacctmeta_t();
        meta_obj->serialize_from_string(meta_value);
        std::cout << "after store.metaobj=" << meta_obj->dump() << std::endl;
        xassert(meta_obj->_highest_cert_block_height == cert_height);
        xassert(meta_obj->_highest_connect_block_height < connect_height);  // connect meta delay store
    }

    auto cert_block = blockstore->get_latest_cert_block(mocktable);
    xassert(cert_block->get_height() == cert_height);
    auto lock_block = blockstore->get_latest_locked_block(mocktable);
    xassert(lock_block->get_height() == cert_height-1);
    auto commit_block = blockstore->get_latest_committed_block(mocktable);
    xassert(commit_block->get_height() == cert_height-2);
    auto connect_block = blockstore->get_latest_connected_block(mocktable);
    xassert(connect_block->get_height() == cert_height-2);

    xdbg("latest_connect_update_1,begin to stop ======= table=%s",table_addr.c_str());
    sleep(1*16+2); // wait for meta save to db. table has 16 times than unit
    xdbg("latest_connect_update_1,end to stop ======= table=%s",table_addr.c_str());

    {
        const std::string meta_value = xvdb_ptr->get_value(meta_path);
        base::xauto_ptr<store::xacctmeta_t> meta_obj = new store::xacctmeta_t();
        meta_obj->serialize_from_string(meta_value);
        std::cout << "after close,metaobj=" << meta_obj->dump() << std::endl;

        xassert(meta_obj->_highest_connect_block_height == connect_block->get_height());
        xassert(meta_obj->_highest_connect_block_hash == connect_block->get_block_hash());

        // change to bad 
        meta_obj->_highest_connect_block_height = tables[10]->get_height();
        meta_obj->_highest_connect_block_hash = tables[10]->get_block_hash();
        std::string vmeta_bin;
        meta_obj->serialize_to_string(vmeta_bin);
        xvdb_ptr->set_value(meta_path,vmeta_bin);

        const std::string meta_value2 = xvdb_ptr->get_value(meta_path);
        base::xauto_ptr<store::xacctmeta_t> meta_obj2 = new store::xacctmeta_t();
        meta_obj2->serialize_from_string(meta_value);
        xassert(meta_obj->_highest_connect_block_height == tables[10]->get_height());
        xassert(meta_obj->_highest_connect_block_hash == tables[10]->get_block_hash());
    }
    xdbg("latest_connect_update_1,begin to get latest connected=======");
    auto connect_block2 = blockstore->get_latest_connected_block(mocktable);
    xassert(connect_block2->get_height() == connect_block->get_height());
    xassert(connect_block2->get_block_hash() == connect_block->get_block_hash());
}
#endif
