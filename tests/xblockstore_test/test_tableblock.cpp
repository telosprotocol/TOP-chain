#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xbase/xvledger.h"

#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xmbus/xevent_store.h"
#include "xmbus/xmessage_bus.h"

#include "test_blockmock.hpp"
#include "xstore/xstore.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xtableblock_util.hpp"
#include "tests/mock/xdatamock_table.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;

class test_tableblock : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_tableblock, mocktable_1) {
    xdatamock_table mocktable(1, 4);
    ASSERT_EQ(mocktable.get_blockchain()->get_last_height(), 0);
    ASSERT_EQ(mocktable.get_history_tables().size(), 1);
}

TEST_F(test_tableblock, mocktable_2) {
    {
        xdatamock_table mocktable;
        mocktable.genrate_table_chain(1);
        ASSERT_EQ(mocktable.get_blockchain()->get_last_height(), 1);
        ASSERT_EQ(mocktable.get_history_tables().size(), 2);
    }
    {
        xdatamock_table mocktable;
        mocktable.genrate_table_chain(2);
        ASSERT_EQ(mocktable.get_blockchain()->get_last_height(), 2);
        ASSERT_EQ(mocktable.get_history_tables().size(), 3);
    }
    {
        xdatamock_table mocktable;
        mocktable.genrate_table_chain(100);
        ASSERT_EQ(mocktable.get_blockchain()->get_last_height(), 100);
        ASSERT_EQ(mocktable.get_history_tables().size(), 101);
    }
}

TEST_F(test_tableblock, mocktable_tableindex_1) {
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(500);
    const std::vector<xdatamock_unit> & datamock_units = mocktable.get_mock_units();
    xblockchain_ptr_t table_blockchain = mocktable.get_blockchain();
    xtable_mbt_ptr_t table_mbt = table_blockchain->get_table_mbt();
    xtable_mbt_binlog_ptr_t table_mbt_binlog = table_blockchain->get_table_mbt_binlog();
    for (auto & datamock_unit : datamock_units) {
        xblock_ptr_t latest_unit = datamock_unit.get_latest_unit();
        xaccount_index_t accountindex;
        // first find in binlog, if not find ,then find in last full table mbt
        if (!table_mbt_binlog->get_unit_index(datamock_unit.get_account(), accountindex)) {
            ASSERT_TRUE(table_mbt->get_unit_index(datamock_unit.get_account(), accountindex));
        }
        ASSERT_EQ(latest_unit->get_height(), accountindex.get_latest_unit_height());
        ASSERT_EQ(latest_unit->get_clock(), accountindex.get_latest_unit_clock());
        ASSERT_EQ(latest_unit->get_unconfirm_sendtx_num() != 0, accountindex.is_has_unconfirm_tx());
    }
}


TEST_F(test_tableblock, store_table_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));

    xdatamock_table mocktable;
    mocktable.genrate_table_chain(100);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    for (auto & table : tables) {
        ASSERT_TRUE(blockstore->store_block(table.get()));
    }
    auto latest_cert_block = blockstore->get_latest_cert_block(mocktable.get_account());
    ASSERT_EQ(latest_cert_block->get_height(), 100);
}

TEST_F(test_tableblock, store_table_from_full_table) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));

    uint64_t max_block_height = 1000;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    auto start = std::chrono::system_clock::now();

    uint64_t begin_full_height = 0;
    uint64_t end_full_height = 0;
    for (uint64_t i = 0; i <= max_block_height; i++) {
        if (tables[i]->get_block_class() == base::enum_xvblock_class_full) {
            begin_full_height = i;
            break;
        }
    }
    for (uint64_t i = max_block_height; i > 0; i--) {
        if (tables[i]->get_block_class() == base::enum_xvblock_class_full) {
            end_full_height = i;
            break;
        }
    }
    uint64_t genesis_connect_height = begin_full_height/2;

    std::cout << "begin_full_height " << begin_full_height << " max_block_height " << max_block_height << std::endl;
    for (uint64_t i = 0; i <= genesis_connect_height; i++) {
        ASSERT_TRUE(blockstore->store_block(tables[i].get()));
    }

    for (uint64_t i = begin_full_height; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(tables[i].get()));
    }
    auto end = std::chrono::system_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << " finish milliseconds " << duration.count() << std::endl;
    auto latest_cert_block = blockstore->get_latest_cert_block(mocktable.get_account());
    ASSERT_EQ(latest_cert_block->get_height(), max_block_height);
    auto latest_lock_block = blockstore->get_latest_locked_block(mocktable.get_account());
    ASSERT_EQ(latest_lock_block->get_height(), max_block_height-1);
    auto latest_commit_block = blockstore->get_latest_committed_block(mocktable.get_account());
    ASSERT_EQ(latest_commit_block->get_height(), max_block_height-2);
    auto latest_executed_block = blockstore->get_latest_executed_block(mocktable.get_account());
    ASSERT_EQ(latest_executed_block->get_height(), genesis_connect_height-2);
    auto latest_full_block = blockstore->get_latest_full_block(mocktable.get_account());
    ASSERT_EQ(latest_full_block->get_height(), end_full_height);
    auto latest_connect_block = blockstore->get_latest_connected_block(mocktable.get_account());
    ASSERT_EQ(latest_connect_block->get_height(), latest_commit_block->get_height());
    auto latest_current_block = blockstore->get_latest_current_block(mocktable.get_account());
    ASSERT_EQ(latest_current_block->get_height(), latest_commit_block->get_height() + 2);
    auto genesis_connect_block = blockstore->get_genesis_connected_block(mocktable.get_account());
    ASSERT_EQ(genesis_connect_block->get_height(), genesis_connect_height-2);
}
