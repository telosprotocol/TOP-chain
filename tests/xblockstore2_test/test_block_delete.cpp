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
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xmetrics/xmetrics.h"

// #include "test_blockmock.hpp"
#include "xstore/xstore.h"
#include "xverifier/xtx_verifier.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::xverifier;
using namespace top::mock;
using namespace top::metrics;

class test_block_delete : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_block_delete, delete_genesis_1) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 1;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    const base::xvaccount_t & _unit_vaccount = base::xvaccount_t(mockunits[0].get_account());
    std::cout << "address=" << mockunits[0].get_account() << std::endl;
    ASSERT_FALSE(blockstore->exist_genesis_block(_unit_vaccount));
    // will write default genesis block
    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->load_block_object(_unit_vaccount, 0, base::enum_xvblock_flag_committed, true);
    ASSERT_NE(genesis_block, nullptr);

    ASSERT_TRUE(blockstore->exist_genesis_block(_unit_vaccount));
    ASSERT_TRUE(blockstore->delete_block(_unit_vaccount, genesis_block.get()));
    ASSERT_FALSE(blockstore->exist_genesis_block(_unit_vaccount));

    auto & units = mockunits[0].get_history_units();
    ASSERT_TRUE(blockstore->store_block(_unit_vaccount, units[0].get()));
    ASSERT_TRUE(blockstore->exist_genesis_block(_unit_vaccount));
    base::xauto_ptr<base::xvblock_t> genesis_block_2 = blockstore->load_block_object(_unit_vaccount, 0, base::enum_xvblock_flag_committed, true);
    ASSERT_NE(genesis_block_2, nullptr);
    ASSERT_NE(genesis_block_2->get_block_hash(), genesis_block->get_block_hash());
}


TEST_F(test_block_delete, delete_genesis_2) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 20;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    const base::xvaccount_t & _unit_vaccount = base::xvaccount_t(mockunits[0].get_account());
    std::cout << "address=" << mockunits[0].get_account() << std::endl;
    ASSERT_FALSE(blockstore->exist_genesis_block(_unit_vaccount));
    // will write default genesis block
    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->load_block_object(_unit_vaccount, 0, base::enum_xvblock_flag_committed, true);
    ASSERT_NE(genesis_block, nullptr);

    // write other blocks
    auto & units = mockunits[0].get_history_units();
    for (uint64_t i = 5; i < 10; i++) {
        ASSERT_TRUE(blockstore->store_block(_unit_vaccount, units[i].get()));
    }
    
    base::xauto_ptr<base::xvblock_t> latest_connect_block = blockstore->get_latest_connected_block(_unit_vaccount);
    ASSERT_EQ(latest_connect_block->get_height(), 0);

{
    ASSERT_TRUE(blockstore->exist_genesis_block(_unit_vaccount));
    ASSERT_TRUE(blockstore->delete_block(_unit_vaccount, genesis_block.get()));
    ASSERT_FALSE(blockstore->exist_genesis_block(_unit_vaccount));
}

    for (uint64_t i = 0; i < 5; i++) {
        ASSERT_TRUE(blockstore->store_block(_unit_vaccount, units[i].get()));
    }

    base::xauto_ptr<base::xvblock_t> latest_connect_block2 = blockstore->get_latest_connected_block(_unit_vaccount);
    ASSERT_NE(latest_connect_block2->get_height(), 0);
}

