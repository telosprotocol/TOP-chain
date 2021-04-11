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
#include "xdata/tests/test_blockutl.hpp"
#include "xmbus/xevent_store.h"
#include "xmbus/xmessage_bus.h"

#include "test_blockmock.hpp"
#include "xstore/xstore.h"
#include "xblockstore/xblockstore_face.h"

#include <chrono>

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;

class test_tc_restart : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
    static void TearDownTestCase() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::string cmd = "rm -rf " + tc_restart_db_path;
        // system(cmd.c_str());

    }
    static void SetUpTestCase() {
    }
    static std::string tc_restart_db_path;
};
std::string test_tc_restart::tc_restart_db_path = "test_tc_restart_db";

TEST_F(test_tc_restart, block_connect_restart_0_BENCH) {

    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_kvdb(tc_restart_db_path);

    uint64_t count = 100;
    auto store = store_face.get();

    std::string address = sys_contract_beacon_timer_addr;
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    test_blockmock_t blockmock(store_face.get());

    std::vector<base::xvblock_t*> blocks;
    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xvblock_t* prev_block = genesis_block.get();
    prev_block->add_ref();
    blocks.push_back(prev_block);
    for (uint64_t i = 1; ; i++) {
        base::xauto_ptr<base::xvblock_t> curr_block = test_blocktuil::create_next_emptyblock(prev_block);
        curr_block->reset_block_flags();
        curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);

        curr_block->add_ref();
        blocks.push_back(curr_block.get());
        if (i >= count) {
            break;
        }
        prev_block = curr_block.get();
    }

    for (uint64_t i = 1; i <= count; i++) {
        if (i % 3 == 0) {
            continue;
        }
        blockstore->store_block(blocks[i]);
    }

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    ASSERT_EQ(connected_block->get_height(), count);

    base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block != nullptr);
    ASSERT_EQ(executed_block->get_height(), count);

    // outdated block
    ASSERT_TRUE(blockstore->store_block(blocks[3]));
    std::this_thread::sleep_for(std::chrono::seconds(2));

    sleep(100);  // account will close after idle time

    base::xauto_ptr<xvblock_t> connected_block_2 = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block_2 != nullptr);
    ASSERT_EQ(connected_block_2->get_height(), count);
}
