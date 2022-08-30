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
using namespace top::mock;

class test_block_notify : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_block_notify, inorder_dup_commit) {
    mock::xdatamock_table mocktable(1, 4);
    uint64_t notified_count = 0;
    uint64_t notified_connect = 0;
    uint64_t notified_commit = 0;

    top::mbus::xevent_queue_cb_t listener1 =
            [&](const top::mbus::xevent_ptr_t& e) {
                top::mbus::xevent_store_ptr_t ptr =
                        dynamic_xobject_ptr_cast<top::mbus::xevent_store_t>(e);

                if (ptr->minor_type == top::mbus::xevent_store_t::type_block_to_db) {
                    top::mbus::xevent_store_block_to_db_ptr_t ptr =
                        dynamic_xobject_ptr_cast<top::mbus::xevent_store_block_to_db_t>(e);
                    notified_connect++;
                }

                if (ptr->minor_type == top::mbus::xevent_store_t::type_block_committed) {
                        top::mbus::xevent_store_block_committed_ptr_t ptr =
                            dynamic_xobject_ptr_cast<top::mbus::xevent_store_block_committed_t>(e);
                    notified_commit++;
                    if (ptr->owner == mocktable.get_account()) {
                        notified_count++;
                    }
                }
            };

    mock::xvchain_creator creator;
    auto mb = creator.get_mbus();
    uint32_t id1 = mb->add_listener(top::mbus::xevent_major_type_store, listener1);

    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 10;
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    EXPECT_EQ(max_count - 2, notified_count);
    EXPECT_EQ(0, notified_connect);
    EXPECT_EQ((max_count - 2) * 1, notified_commit);  // only table commit event

    // duplicate store
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    EXPECT_EQ(max_count - 2, notified_count);
    EXPECT_EQ(0, notified_connect);
    EXPECT_EQ((max_count - 2) * 1, notified_commit);// only table commit event
}

TEST_F(test_block_notify, disorder_commit) {
    mock::xdatamock_table mocktable(1, 4);
    uint64_t notified_connect = 0;
    uint64_t notified_commit = 0;

    top::mbus::xevent_queue_cb_t listener1 =
            [&](const top::mbus::xevent_ptr_t& e) {
                top::mbus::xevent_store_ptr_t ptr =
                        dynamic_xobject_ptr_cast<top::mbus::xevent_store_t>(e);

                if (ptr->minor_type == top::mbus::xevent_store_t::type_block_to_db) {
                    top::mbus::xevent_store_block_to_db_ptr_t ptr =
                        dynamic_xobject_ptr_cast<top::mbus::xevent_store_block_to_db_t>(e);
                    if (ptr->owner == mocktable.get_account()) {
                        notified_connect++;
                    }
                }

                if (ptr->minor_type == top::mbus::xevent_store_t::type_block_committed) {
                        top::mbus::xevent_store_block_committed_ptr_t ptr =
                            dynamic_xobject_ptr_cast<top::mbus::xevent_store_block_committed_t>(e);
                    if (ptr->owner == mocktable.get_account()) {
                        notified_commit++;
                    }
                }
            };

    mock::xvchain_creator creator(true);
    auto mb = creator.get_mbus();
    uint32_t id1 = mb->add_listener(top::mbus::xevent_major_type_store, listener1);

    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 10;
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);

    for(uint32_t i = 0; i <= max_count; ++i) {
        if (i % 3 != 0) {
            ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[i].get()));
        }
    }

    // last block is lock block
    EXPECT_EQ(0, notified_connect);
    EXPECT_EQ(0, notified_commit);

    for(int32_t i = max_count; i >= 0; --i) {
        if (i % 3 == 0) {
            ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[i].get()));
        }
    }

    // last block is lock block
    EXPECT_EQ(0, notified_connect);
    EXPECT_EQ(max_count - 2, notified_commit);
}
#if 0
TEST_F(test_block_notify, block_set_discrete) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store_face2 = xstore_factory::create_store_with_memdb();

    uint64_t count = 20;

    auto store = store_face.get();
    auto store2 = store_face2.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);
    xvblockstore_t* blockstore2 = xblockstorehub_t::instance().create_block_store(*store2, address);

    test_blockmock_t blockmock(store_face.get());

    base::xvblock_t *prev_block = (blockstore->get_genesis_block(address).get());
    base::xvblock_t *curr_block = nullptr;

    std::vector<base::xvblock_t*> saved_blocks;
    saved_blocks.push_back(prev_block);
    for (uint64_t i = 1; i <= count; i++) {

        xaccount_cmd_ptr_t cmd = nullptr;
        curr_block = blockmock.create_sample_block(prev_block, cmd.get(), address);

        ASSERT_TRUE(blockstore->store_block(curr_block));
        ASSERT_EQ(curr_block->get_height(), i);
        uint64_t chainheight = store->get_blockchain_height(address);
        ASSERT_EQ(chainheight, curr_block->get_height());
        prev_block = curr_block;
        saved_blocks.push_back(prev_block);
    }

    uint64_t missing_height = 10;

    prev_block = saved_blocks[0];

    for (uint64_t i = 1; i < missing_height; i++) {
        curr_block = saved_blocks[i];
        curr_block->reset_prev_block(nullptr);
        curr_block->reset_block_flags();
        curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        curr_block->set_block_flag(base::enum_xvblock_flag_locked);
        if (prev_block->get_height() != 0) {
            prev_block->set_block_flag(base::enum_xvblock_flag_committed);
            ASSERT_TRUE(blockstore2->store_block(prev_block));
        }
        ASSERT_TRUE(blockstore2->store_block(curr_block));
        prev_block = curr_block;
    }
    prev_block->set_block_flag(base::enum_xvblock_flag_committed);
    ASSERT_TRUE(blockstore2->store_block(prev_block));

    prev_block = saved_blocks[missing_height+1];
    prev_block->reset_block_flags();
    prev_block->reset_prev_block(nullptr);
    prev_block->set_block_flag(base::enum_xvblock_flag_authenticated);
    prev_block->set_block_flag(base::enum_xvblock_flag_locked);

    for (uint64_t i = missing_height + 2; i <= count; i++) {
        curr_block = saved_blocks[i];
        curr_block->reset_prev_block(nullptr);
        curr_block->reset_block_flags();
        curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        curr_block->set_block_flag(base::enum_xvblock_flag_locked);

        prev_block->set_block_flag(base::enum_xvblock_flag_committed);
        ASSERT_TRUE(blockstore2->store_block(prev_block));

        ASSERT_TRUE(blockstore2->store_block(curr_block));
        prev_block = curr_block;
    }

    curr_block = saved_blocks[missing_height];
    curr_block->reset_block_flags();
    curr_block->reset_prev_block(nullptr);
    curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
    curr_block->set_block_flag(base::enum_xvblock_flag_locked);
    curr_block->set_block_flag(base::enum_xvblock_flag_committed);
    ASSERT_TRUE(blockstore2->store_block(curr_block));

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    ASSERT_EQ(connected_block->get_height(), count - 1);

    base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block != nullptr);
    ASSERT_EQ(executed_block->get_height(), count - 1);

}

TEST_F(test_block_notify, block_set_discrete_1) {
    std::shared_ptr<mbus::xmessage_bus_face_t> mb = std::make_shared<mbus::xmessage_bus_t>();
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store_face2 = xstore_factory::create_store_with_memdb(mb);

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
    auto store2 = store_face2.get();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);
    xvblockstore_t* blockstore2 = xblockstorehub_t::instance().create_block_store(*store2, address);

    test_blockmock_t blockmock(store_face.get());

    base::xvblock_t *prev_block = (blockstore->get_genesis_block(address).get());
    base::xvblock_t *curr_block = nullptr;

    std::vector<base::xvblock_t*> saved_blocks;
    saved_blocks.push_back(prev_block);
    for (uint64_t i = 1; i <= count; i++) {

        xaccount_cmd_ptr_t cmd = nullptr;
        curr_block = blockmock.create_sample_block(prev_block, cmd.get(), address);

        ASSERT_TRUE(blockstore->store_block(curr_block));
        ASSERT_EQ(curr_block->get_height(), i);
        uint64_t chainheight = store->get_blockchain_height(address);
        ASSERT_EQ(chainheight, curr_block->get_height());
        prev_block = curr_block;
        saved_blocks.push_back(prev_block);
    }

    prev_block = saved_blocks[0];

    for (uint64_t i = 1; i <= count; i++) {
        if (0 != i % 10) {
            curr_block = saved_blocks[i];
            curr_block->reset_prev_block(nullptr);
            curr_block->reset_block_flags();
            curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
            curr_block->set_block_flag(base::enum_xvblock_flag_locked);
            if (prev_block->get_height() != 0) {
                prev_block->set_block_flag(base::enum_xvblock_flag_committed);
                ASSERT_TRUE(blockstore2->store_block(prev_block));
            }
            ASSERT_TRUE(blockstore2->store_block(curr_block));
            prev_block = curr_block;
        }
    }

    for (uint64_t i = 1; i <= count; i++) {
        if (0 == i % 10) {
            curr_block = saved_blocks[i];
            curr_block->reset_block_flags();
            curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
            curr_block->set_block_flag(base::enum_xvblock_flag_locked);
            curr_block->set_block_flag(base::enum_xvblock_flag_committed);
            ASSERT_TRUE(blockstore2->store_block(curr_block));
        }
    }

    ASSERT_GE(count, notified_count);
}

TEST_F(test_block_notify, block_event_repeat_block) {
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

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    test_blockmock_t blockmock(store_face.get());

    base::xvblock_t *prev_block = (blockstore->get_genesis_block(address).get());
    base::xvblock_t *curr_block = nullptr;

    std::vector<base::xvblock_t*> saved_blocks;
    saved_blocks.push_back(prev_block);
    for (uint64_t i = 1; i <= count; i++) {

        std::string value = std::to_string(i);
        xaccount_cmd_ptr_t cmd = nullptr;
        curr_block = blockmock.create_sample_block(prev_block, cmd.get(), address);

        ASSERT_TRUE(blockstore->store_block(curr_block));
        ASSERT_EQ(curr_block->get_height(), i);
        uint64_t chainheight = store->get_blockchain_height(address);
        ASSERT_EQ(chainheight, curr_block->get_height());
        prev_block = curr_block;
        saved_blocks.push_back(prev_block);
    }

    // outdated block
    ASSERT_TRUE(blockstore->store_block(saved_blocks[3]));
    ASSERT_EQ(count, notified_count);
}

TEST_F(test_block_notify, block_event_synced_block) {
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

    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

    test_blockmock_t blockmock(store_face.get());

    base::xvblock_t *prev_block = (blockstore->get_genesis_block(address).get());
    base::xvblock_t *curr_block = nullptr;

    for (uint64_t i = 1; i <= count; i++) {

        std::string value = std::to_string(i);
        xaccount_cmd_ptr_t cmd = nullptr;
        curr_block = blockmock.create_sample_block(prev_block, cmd.get(), address);

        ASSERT_TRUE(blockstore->store_block(curr_block));
        ASSERT_EQ(curr_block->get_height(), i);
        uint64_t chainheight = store->get_blockchain_height(address);
        ASSERT_EQ(chainheight, curr_block->get_height());
        prev_block = curr_block;
    }

    base::xvblock_t* new_block = store->get_block_by_height(address, 3);
    ASSERT_TRUE(new_block != nullptr);

    new_block->set_block_flag(base::enum_xvblock_flag_locked);

    // outdated block
    ASSERT_TRUE(blockstore->store_block(new_block));
    ASSERT_EQ(count, notified_count);
}
#endif
