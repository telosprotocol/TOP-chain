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

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;

class test_block_connected : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_block_connected, block_connect_normal) {

    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();

    uint64_t count = 100;
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

        // curr_block->remove_block_flag(base::enum_xvblock_flag_committed);
        // curr_block->set_block_flag(base::enum_xvblock_flag_locked);
        if (prev_block->get_height() != 0) {
            prev_block->set_block_flag(base::enum_xvblock_flag_committed);
            ASSERT_TRUE(blockstore->store_block(prev_block));
            uint64_t chainheight = store->get_blockchain_height(address);
            ASSERT_EQ(chainheight, prev_block->get_height());
        }
        ASSERT_TRUE(blockstore->store_block(curr_block));
        ASSERT_EQ(curr_block->get_height(), i);
        auto latest_block = blockstore->get_latest_current_block(address);
        ASSERT_EQ(latest_block->get_height(), i);
        prev_block = curr_block;
        saved_blocks.push_back(prev_block);
    }

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    ASSERT_EQ(connected_block->get_height(), count);

    base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block != nullptr);
    ASSERT_EQ(executed_block->get_height(), count);

    // outdated block
    ASSERT_TRUE(blockstore->store_block(saved_blocks[3]));
}

TEST_F(test_block_connected, block_connect_discrete) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store_face2 = xstore_factory::create_store_with_memdb();

    uint64_t count = 19;

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
        ASSERT_TRUE(blockstore2->store_block(curr_block));
    }

    for (uint64_t i = missing_height + 1; i <= count; i++) {
        curr_block = saved_blocks[i];
        curr_block->reset_prev_block(nullptr);
        curr_block->reset_block_flags();
        curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        ASSERT_TRUE(blockstore2->store_block(curr_block));
    }

    base::xauto_ptr<xvblock_t> latest_block = blockstore2->get_latest_current_block(address);
    ASSERT_EQ(latest_block->get_height(), missing_height - 1);

    curr_block = saved_blocks[missing_height];
    curr_block->reset_block_flags();
    curr_block->reset_prev_block(nullptr);
    curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
    ASSERT_TRUE(blockstore2->store_block(curr_block));

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    ASSERT_EQ(connected_block->get_height(), count - 2);

    base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block != nullptr);
    ASSERT_EQ(executed_block->get_height(), count - 2);

    base::xauto_ptr<xvblock_t> latest_block_1 = blockstore2->get_latest_current_block(address);
    ASSERT_EQ(latest_block_1->get_height(), count);
}

TEST_F(test_block_connected, block_connect_discrete_1) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store_face2 = xstore_factory::create_store_with_memdb();

    uint64_t count = 100;

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
            ASSERT_TRUE(blockstore2->store_block(curr_block));
        }
    }

    base::xauto_ptr<xvblock_t> latest_block = blockstore2->get_latest_current_block(address);
    ASSERT_EQ(latest_block->get_height(), count - 11);

    for (uint64_t i = 1; i <= count; i++) {
        if (0 == i % 10) {
            curr_block = saved_blocks[i];
            curr_block->reset_block_flags();
            curr_block->reset_prev_block(nullptr);
            curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
            ASSERT_TRUE(blockstore2->store_block(curr_block));
        }
    }

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    ASSERT_EQ(connected_block->get_height(), count - 2);

    base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block != nullptr);
    ASSERT_EQ(executed_block->get_height(), count - 2);

    base::xauto_ptr<xvblock_t> latest_block_1 = blockstore2->get_latest_current_block(address);
    ASSERT_EQ(latest_block_1->get_height(), count);
}

TEST_F(test_block_connected, block_connect_discrete_2) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store_face2 = xstore_factory::create_store_with_memdb();

    uint64_t count = 100;

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

    uint64_t full_height = 63;
    prev_block = saved_blocks[full_height];
    prev_block->reset_prev_block(nullptr);
    prev_block->reset_next_block(nullptr);
    prev_block->reset_block_flags();
    prev_block->set_block_flag(base::enum_xvblock_flag_authenticated);
    prev_block->set_block_flag(base::enum_xvblock_flag_locked);
    prev_block->set_block_flag(base::enum_xvblock_flag_committed);
    ASSERT_TRUE(blockstore2->store_block(prev_block));

    for (uint64_t i = full_height + 1; i <= count; i++) {
        curr_block = saved_blocks[i];
        curr_block->reset_prev_block(nullptr);
        curr_block->reset_block_flags();
        curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        curr_block->set_block_flag(base::enum_xvblock_flag_locked);
        if (prev_block->check_block_flag(base::enum_xvblock_flag_committed) == false) {
            prev_block->set_block_flag(base::enum_xvblock_flag_committed);
            ASSERT_TRUE(blockstore2->store_block(prev_block));
        }
        ASSERT_TRUE(blockstore2->store_block(curr_block));
        prev_block = curr_block;
    }

    base::xauto_ptr<xvblock_t> connected_block = blockstore->get_latest_connected_block(address);
    ASSERT_TRUE(connected_block != nullptr);
    ASSERT_EQ(connected_block->get_height(), count - 1);

    base::xauto_ptr<xvblock_t> executed_block = blockstore->get_latest_executed_block(address);
    ASSERT_TRUE(executed_block != nullptr);
    ASSERT_EQ(executed_block->get_height(), count - 1);
}

TEST_F(test_block_connected, store_block_in_unorder_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    uint64_t count = 100;

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

    ASSERT_TRUE(blockstore->store_block(blocks[1]));
    ASSERT_TRUE(blockstore->store_block(blocks[2]));
    ASSERT_TRUE(blockstore->store_block(blocks[3]));
    ASSERT_TRUE(blockstore->store_block(blocks[4]));
    ASSERT_TRUE(blockstore->store_block(blocks[7]));
    ASSERT_TRUE(blockstore->store_block(blocks[8]));
    ASSERT_TRUE(blockstore->store_block(blocks[9]));
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
    ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), 4);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 2);
    ASSERT_EQ(blockstore->get_latest_executed_block(address)->get_height(), 2);

    ASSERT_TRUE(blockstore->store_block(blocks[5]));
    ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), 5);
    ASSERT_TRUE(blockstore->store_block(blocks[6]));
    ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), 9);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
}

TEST_F(test_block_connected, store_block_in_unorder_2) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    uint64_t count = 100;

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

    ASSERT_TRUE(blockstore->store_block(blocks[1]));
    ASSERT_TRUE(blockstore->store_block(blocks[2]));
    ASSERT_TRUE(blockstore->store_block(blocks[3]));
    ASSERT_TRUE(blockstore->store_block(blocks[5]));
    ASSERT_TRUE(blockstore->store_block(blocks[7]));
    ASSERT_TRUE(blockstore->store_block(blocks[8]));
    ASSERT_TRUE(blockstore->store_block(blocks[9]));
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
    ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), 3);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 1);
    ASSERT_EQ(blockstore->get_latest_executed_block(address)->get_height(), 1);

    ASSERT_TRUE(blockstore->store_block(blocks[4]));
    ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), 5);
    ASSERT_TRUE(blockstore->store_block(blocks[6]));
    ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), 9);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
}

TEST_F(test_block_connected, store_block_in_unorder_3) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    uint64_t count = 100;

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

    ASSERT_TRUE(blockstore->store_block(blocks[1]));
    ASSERT_TRUE(blockstore->store_block(blocks[2]));
    ASSERT_TRUE(blockstore->store_block(blocks[3]));
    ASSERT_TRUE(blockstore->store_block(blocks[5]));
    ASSERT_TRUE(blockstore->store_block(blocks[6]));
    ASSERT_TRUE(blockstore->store_block(blocks[7]));
    ASSERT_TRUE(blockstore->store_block(blocks[8]));
    ASSERT_TRUE(blockstore->store_block(blocks[9]));
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
    ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), 3);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 1);
    ASSERT_EQ(blockstore->get_latest_executed_block(address)->get_height(), 1);

    ASSERT_TRUE(blockstore->store_block(blocks[4]));
    ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), 9);
    ASSERT_TRUE(blockstore->store_block(blocks[6]));
    ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), 9);
    ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), 7);
    ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), 8);
    ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), 9);
}

TEST_F(test_block_connected, store_block_in_order_1) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    uint64_t count = 100;

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xvblock_t* prev_block = genesis_block.get();
    prev_block->add_ref();
    for (uint64_t i = 1; ; i++) {
        base::xauto_ptr<base::xvblock_t> curr_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->release_ref();
        curr_block->reset_block_flags();
        curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        ASSERT_TRUE(blockstore->store_block(curr_block.get()));
        ASSERT_EQ(curr_block->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), i - 1);
        ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_executed_block(address)->get_height(), i >= 2 ? i - 2 : 0);


        if (i >= count) {
            break;
        }
        prev_block = curr_block.get();
        prev_block->add_ref();
    }
}

#if 0  // this test need 6*80s
TEST_F(test_block_connected, store_block_in_order_sleep) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_shard_table_account(1);
    uint64_t count = 6;

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xvblock_t* prev_block = genesis_block.get();
    prev_block->add_ref();
    for (uint64_t i = 1; ; i++) {
        base::xauto_ptr<base::xvblock_t> curr_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->release_ref();
        curr_block->reset_block_flags();
        curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        ASSERT_TRUE(blockstore->store_block(curr_block.get()));
        ASSERT_EQ(curr_block->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), i - 1);
        ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_executed_block(address)->get_height(), i >= 2 ? i - 2 : 0);


        if (i >= count) {
            break;
        }
        prev_block = curr_block.get();
        prev_block->add_ref();

        sleep(80);
    }
}
#endif

TEST_F(test_block_connected, store_block_in_order_execute) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
    base::xauto_ptr<xvblockstore_t> blockstore(xblockstorehub_t::instance().create_block_store(*store_face.get(), {}));
    std::string address = xblocktool_t::make_address_shard_table_account(1);
    uint64_t count = 1000;

    base::xauto_ptr<base::xvblock_t> genesis_block = blockstore->get_genesis_block(address);
    base::xvblock_t* prev_block = genesis_block.get();
    prev_block->add_ref();
    for (uint64_t i = 1; ; i++) {
        base::xauto_ptr<base::xvblock_t> curr_block = test_blocktuil::create_next_emptyblock(prev_block);
        prev_block->release_ref();
        curr_block->reset_block_flags();
        curr_block->set_block_flag(base::enum_xvblock_flag_authenticated);
        ASSERT_TRUE(blockstore->store_block(curr_block.get()));
        ASSERT_EQ(curr_block->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_cert_block(address)->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_current_block(address)->get_height(), i);
        ASSERT_EQ(blockstore->get_latest_locked_block(address)->get_height(), i - 1);
        ASSERT_EQ(blockstore->get_latest_committed_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_connected_block(address)->get_height(), i >= 2 ? i - 2 : 0);
        ASSERT_EQ(blockstore->get_latest_executed_block(address)->get_height(), i >= 2 ? i - 2 : 0);


        if (i >= count) {
            break;
        }
        prev_block = curr_block.get();
        prev_block->add_ref();
    }
}

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
