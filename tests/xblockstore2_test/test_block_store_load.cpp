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
#include "xstore/xstore.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;

class test_block_store_load : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_block_store_load, store_batch_tables) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 19;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }
}

TEST_F(test_block_store_load, load_units_BENCH) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 1000;
    uint32_t user_count = 20;
    mock::xdatamock_table mocktable(1, user_count);
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    {
        auto start_time = std::chrono::system_clock::now();
        for (auto & block : tableblocks) {
            ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
        }
        auto end_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << " store all blocks milliseconds " << duration.count() << std::endl;
    }

    {
        auto start_time = std::chrono::system_clock::now();
        for (auto & mockunit : mockunits) {
            uint64_t unit_height = mockunit.get_cert_block()->get_height();
            for (uint64_t height = 1; height <= unit_height; height++) {
                base::xvaccount_t _vaddr(mockunit.get_account());
                auto _block = blockstore->load_block_object(_vaddr, height, 0, false);
                blockstore->load_block_input(_vaddr, _block.get());
                blockstore->load_block_output(_vaddr, _block.get());
            }
        }
        auto end_time = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        std::cout << " load all blocks milliseconds " << duration.count() << std::endl;
    }

}
