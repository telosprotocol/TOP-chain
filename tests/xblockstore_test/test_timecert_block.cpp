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

#include <chrono>

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;

std::string db_path = "test_tc_db";
class test_timecert_block : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
    static void TearDownTestCase() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::string cmd = "rm -rf " + db_path;
        // system(cmd.c_str());

    }
    static void SetUpTestCase() {
    }
};

TEST_F(test_timecert_block, block_connect_normal) {

    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();

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
}

TEST_F(test_timecert_block, block_store_old_new) {
    std::shared_ptr<mbus::xmessage_bus_face_t> mb = std::make_shared<mbus::xmessage_bus_t>();
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb(mb);

    uint64_t notified_count = 0;
    uint64_t count = 100;
    top::mbus::xevent_queue_cb_t listener1 =
            [&](const top::mbus::xevent_ptr_t& e) {
                top::mbus::xevent_store_ptr_t ptr =
                        std::static_pointer_cast<top::mbus::xevent_store_t>(e);
                if (ptr->minor_type == top::mbus::xevent_store_t::type_block_to_db) {
                    top::mbus::xevent_store_block_to_db_ptr_t ptr =
                        std::static_pointer_cast<top::mbus::xevent_store_block_to_db_t>(e);

                    notified_count++;
                }
            };

    uint32_t id1 = mb->add_listener(top::mbus::xevent_major_type_store, listener1);
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

    ASSERT_EQ(blockstore->store_block(blocks[20]), true);
    ASSERT_EQ(blockstore->store_block(blocks[20]), false);
    ASSERT_EQ(blockstore->store_block(blocks[10]), true); // no notify event
    ASSERT_EQ(blockstore->store_block(blocks[30]), true);

    ASSERT_EQ(0, notified_count);
}

TEST_F(test_timecert_block, block_connect_discrete_1) {

    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();

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
        if (i % 4 == 0) {
            blockstore->store_block(blocks[i]);
        }
    }

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    ASSERT_EQ(connected_block->get_height(), count);

    base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block != nullptr);
    ASSERT_EQ(executed_block->get_height(), count);

    // outdated block
    ASSERT_TRUE(blockstore->store_block(blocks[3]));

    base::xauto_ptr<xvblock_t> executed_block1 = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block1 != nullptr);
    ASSERT_EQ(executed_block1->get_height(), count);
}

TEST_F(test_timecert_block, block_connect_discrete_2) {

    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();

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

    for (uint64_t i = 62; i <= count; i++) {
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

    base::xauto_ptr<xvblock_t> executed_block1 = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block1 != nullptr);
    ASSERT_EQ(executed_block1->get_height(), count);
}

TEST_F(test_timecert_block, block_connect_discrete_3) {

    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();

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
        if (i % 2 == 1) {
            blockstore->store_block(blocks[i]);
        }
    }

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    ASSERT_EQ(connected_block->get_height(), count - 1);

    base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block != nullptr);
    ASSERT_EQ(executed_block->get_height(), count - 1);


    for (uint64_t i = 1; i <= count; i++) {
        if (i % 2 == 0) {
            blockstore->store_block(blocks[i]);
        }
    }

    base::xauto_ptr<xvblock_t> executed_block1 = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block1 != nullptr);
    ASSERT_EQ(executed_block1->get_height(), count);

}

TEST_F(test_timecert_block, block_mem_leak_1_BENCH) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = sys_contract_beacon_timer_addr;
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock(genesis_block.get());
    ASSERT_TRUE(blockstore->store_block(block1.get()));
    ASSERT_EQ(block1->get_refcount(), 1 + 1);  // blockstore cache ref 1
    sleep(70);
    ASSERT_EQ(genesis_block->get_refcount(), 1);
    ASSERT_EQ(block1->get_refcount(), 1);
}

TEST_F(test_timecert_block, block_mem_leak_2_BENCH) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    auto store = store_face.get();

    std::string address = sys_contract_beacon_timer_addr;
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xauto_ptr<base::xvblock_t> block1 = test_blocktuil::create_next_emptyblock(genesis_block.get());
    ASSERT_TRUE(blockstore->store_block(block1.get()));
    ASSERT_EQ(block1->get_refcount(), 1 + 1);  // blockstore cache ref 1

    base::xauto_ptr<base::xvblock_t> block2 = test_blocktuil::create_next_emptyblock(block1.get());
    ASSERT_TRUE(blockstore->store_block(block2.get()));
    ASSERT_EQ(block1->get_refcount(), 1 + 1);  // blockstore cache ref 1
    sleep(70);
    ASSERT_EQ(genesis_block->get_refcount(), 1);
    ASSERT_EQ(block1->get_refcount(), 1);
    ASSERT_EQ(block2->get_refcount(), 1);
}


TEST_F(test_timecert_block, block_connect_restart_0_BENCH) {

    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_kvdb(db_path);

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
