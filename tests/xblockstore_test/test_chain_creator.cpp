#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"

#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xmessage_bus.h"

#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;

class test_chain_creator : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_chain_creator, test_1) {
    mock::xvchain_creator creator(true);
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t count = 19;
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count, blockstore);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == count + 1);

    std::string address = mocktable.get_account();
    xvaccount_t account(address);

    for (uint64_t i = 1; i <= count; i++) {
        auto curr_block = tables[i].get();
        ASSERT_TRUE(blockstore->store_block(account, curr_block));
    }

    base::xauto_ptr<xvblock_t> latest_block = blockstore->get_latest_connected_block(account);
    EXPECT_EQ(latest_block->get_height(), count - 2);
}

TEST_F(test_chain_creator, test_2_BENCH) {
    int count = 100;
    while ( count -- > 0 )
    {
        mock::xvchain_creator creator(true);
        creator.create_blockstore_with_xstore();
        base::xvblockstore_t* blockstore = creator.get_blockstore();

        uint64_t count = 19;
        mock::xdatamock_table mocktable;
        mocktable.genrate_table_chain(count, blockstore);
        const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
        xassert(tables.size() == count + 1);

        std::string address = mocktable.get_account();
        xvaccount_t account(address);

        for (uint64_t i = 1; i <= count; i++) {
            auto curr_block = tables[i].get();
            ASSERT_TRUE(blockstore->store_block(account, curr_block));
        }

        base::xauto_ptr<xvblock_t> latest_block = blockstore->get_latest_connected_block(account);
        EXPECT_EQ(latest_block->get_height(), count - 2);
    }
}

TEST_F(test_chain_creator, test_3_BENCH) {
    mock::xvchain_creator creator(true);
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    std::string address = xblocktool_t::make_address_table_account(base::enum_chain_zone_beacon_index, 0);
    xvaccount_t account(address);

    base::xauto_ptr<xvblock_t> genesis_block1 = blockstore->get_genesis_block(account);
    EXPECT_EQ(genesis_block1->get_height(), 0);
}
