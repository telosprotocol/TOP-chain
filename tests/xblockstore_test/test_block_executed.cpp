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
#include "xblockstore/src/xvblockhub.h"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;

class test_block_executed : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};
#if 0 // block execute move to xstatestore
TEST_F(test_block_executed, inorder) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 10;
    mock::xdatamock_table mocktable(1, 2);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), max_count - 2);
}

TEST_F(test_block_executed, disorder) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 10;
    mock::xdatamock_table mocktable(1, 2);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);

    uint32_t hole_height = 5;
    for (uint32_t i = 0; i <= max_count; ++i) {
        if (i != hole_height) {
            ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[i].get()));
        }
    }

    {
        EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), hole_height - 1 - 2);
    }

    {
        ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[hole_height].get()));
        EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), max_count - 2);
    }

}

TEST_F(test_block_executed, execute_height_update_1) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 100;
    mock::xdatamock_table mocktable(1, 4); 
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);

    uint32_t hole_height = 5;
    for (uint32_t i = 0; i <= max_count; ++i) {
        if (i != hole_height) {
            ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[i].get()));
        }
    }

    {
        EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), hole_height - 1 - 2);
    }

    std::cout << "before store hole, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;
    ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[hole_height].get()));
    std::cout << "after store hole, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;
    auto cert_block = blockstore->get_latest_cert_block(mocktable);
    creator.get_xblkstatestore()->get_block_state(cert_block.get());
    // std::cout << "after get_block_state 1, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;
    creator.get_xblkstatestore()->get_block_state(cert_block.get());
    // std::cout << "after get_block_state 2, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl; 
    creator.get_xblkstatestore()->get_block_state(cert_block.get());
    // std::cout << "after get_block_state 3, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;  
    xassert(blockstore->get_latest_executed_block_height(mocktable) == tableblocks[max_count-2]->get_height());
}

TEST_F(test_block_executed, execute_height_update_2) {
    uint64_t max_count = 140;
    mock::xdatamock_table mocktable(1, 4); 
    mocktable.genrate_table_chain(max_count, nullptr);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);

    uint64_t last_full_block_height = tableblocks[max_count]->get_last_full_block_height();
    xblock_ptr_t last_full_table = tableblocks[last_full_block_height];
    std::string property_snapshot;
    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();
        mocktable.store_genesis_units(blockstore);
        for (uint32_t i = 0; i <= max_count; ++i) {
            ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[i].get()));
        }

        auto bstate = creator.get_xblkstatestore()->get_block_state(last_full_table.get());
        xassert(bstate != nullptr);
        bstate->take_snapshot(property_snapshot);
    }

    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();        
        mocktable.store_genesis_units(blockstore);
        uint32_t hole_height = 5;
        for (uint32_t i = 0; i <= max_count; ++i) {
            if (i != hole_height) {
                ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[i].get()));
            }
        }
        EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), hole_height - 1 - 2);

        xassert(last_full_table->is_full_state_block() == false);
        last_full_table->set_offblock_snapshot(property_snapshot);
        xassert(last_full_table->is_full_state_block() == true);
        std::cout << "before store full-table, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;
        std::cout << "full-table height = " << last_full_table->get_height() << std::endl;
        ASSERT_TRUE(blockstore->store_block(mocktable, last_full_table.get()));        
        assert(blockstore->get_latest_executed_block_height(mocktable) > last_full_table->get_height());
        std::cout << "after store full-table, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;

        for (uint32_t i = 0; i < 10; i++) {
            ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[20].get()));
            // std::cout << "after store_block repeat block, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;
        }        
        xassert(blockstore->get_latest_executed_block_height(mocktable) == tableblocks[max_count-2]->get_height());
    }
}
#endif
#if 0 // TODO(jimmy) xacctmeta_t is deleted
TEST_F(test_block_executed, execute_height_update_3_BENCH) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 1000;
    mock::xdatamock_table mocktable(1, 4); 
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);

    std::string table_addr = mocktable.get_account();
    // blockstore->reset_cache_timeout(mocktable, 100); // idle time change to 100ms
    std::string meta_path = "0/" + table_addr + "/meta";
    base::xvdbstore_t* xvdb_ptr = base::xvchain_t::instance().get_xdbstore();

    uint32_t hole_height = 5;
    for (uint32_t i = 0; i <= max_count; ++i) {
        if (i != hole_height) {
            ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[i].get()));
        }
    }

    // std::cout << "before store hole, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;
    ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[hole_height].get()));
    // std::cout << "after store hole, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;

    auto cert_block = blockstore->get_latest_cert_block(mocktable);
    for (uint32_t i = 0; i < 33; i++) {
        creator.get_xblkstatestore()->get_block_state(cert_block.get());
        // std::cout << "after get_block_state 1, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;
    }
    xassert(blockstore->get_latest_executed_block_height(mocktable) == tableblocks[max_count-2]->get_height());

    sleep(20); // wait for meta save to db. table has 16 times than unit
    // change meta check execute recover
    {
        const std::string meta_value = xvdb_ptr->get_value(meta_path);
        base::xauto_ptr<store::xacctmeta_t> meta_obj = new store::xacctmeta_t();
        meta_obj->serialize_from_string(meta_value);
        xassert(meta_obj->_highest_execute_block_height == tableblocks[max_count-2]->get_height());
        xassert(meta_obj->_highest_execute_block_hash == tableblocks[max_count-2]->get_block_hash());
        // change to bad 
        meta_obj->_highest_execute_block_height = tableblocks[10]->get_height();
        meta_obj->_highest_execute_block_hash = tableblocks[10]->get_block_hash();
        std::string vmeta_bin;
        meta_obj->serialize_to_string(vmeta_bin);
        xvdb_ptr->set_value(meta_path,vmeta_bin);
    }

    xdbg("===========execute_height_update_3============");
    // std::cout << "after set_latest_executed_info, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;
    xassert(blockstore->get_latest_executed_block_height(mocktable) == tableblocks[10]->get_height());

    for (uint32_t i = 0; i < 20; i++) {
        ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[20].get()));
        // std::cout << "after store_block repeat block, execute height = " << blockstore->get_latest_executed_block_height(mocktable) << std::endl;
    }
    xassert(blockstore->get_latest_executed_block_height(mocktable) == tableblocks[max_count-2]->get_height());
}
#endif
// // xaccount_cmd_ptr_t create_or_modify_property(xstore_face_t* store, const std::string &address, const std::string& list_name, const std::string& item_value) {
// //     auto account = store->clone_account(address);
// //     xaccount_cmd_ptr_t cmd;

// //     int32_t ret;
// //     if (account == nullptr) {
// //         account = new xblockchain2_t(address);
// //         cmd = std::make_shared<xaccount_cmd>(account, store);
// //         auto ret = cmd->list_create(list_name, true);
// //         assert(ret == 0);
// //     } else {
// //         int32_t error_code;
// //         cmd = std::make_shared<xaccount_cmd>(account, store);
// //         auto prop_ptr = cmd->get_property(list_name, error_code);

// //         if (prop_ptr == nullptr) {
// //             assert(0);
// //         }
// //         auto ret = cmd->list_push_back(list_name, item_value, true);
// //         assert(ret == 0);
// //     }

// //     return cmd;
// // }


// TEST_F(test_block_executed, block_executed_normal) {

//     std::shared_ptr<mbus::xmessage_bus_face_t> mb = std::make_shared<mbus::xmessage_bus_t>();
//     xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb(mb);

//     uint64_t notified_count = 0;
//     uint64_t count = 100;

//     auto store = store_face.get();

//     std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
//     xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

//     test_blockmock_t blockmock(store_face.get());

//     std::vector<base::xvblock_t*> units;

//     std::string prop_name = "aaa";

//     base::xvblock_t *prev_block = (blockstore->get_genesis_block(address).get());
//     base::xvblock_t *curr_block = nullptr;

//     // ASSERT_TRUE(store_face->set_vblock(blockstore->get_genesis_block(address).get()));
//     std::vector<base::xvblock_t*> saved_blocks;
//     saved_blocks.push_back(prev_block);
//     for (uint64_t i = 1; i <= count; i++) {

//         std::string value = std::to_string(i);
//         xaccount_cmd_ptr_t cmd = create_or_modify_property(store, address, prop_name, value);
//         curr_block = blockmock.create_sample_block(prev_block, cmd.get(), address);

//         ASSERT_TRUE(blockstore->store_block(curr_block));
//         ASSERT_EQ(curr_block->get_height(), i);
//         uint64_t chainheight = store->get_blockchain_height(address);
//         ASSERT_EQ(chainheight, curr_block->get_height());
//         prev_block = curr_block;
//         saved_blocks.push_back(prev_block);
//     }

//     std::vector<std::string> values;

//     uint64_t heights[] = {8, 30, 80, 67};
//     for (uint64_t height : heights) {
//         int32_t ret = store_face->get_list_property(address, height, prop_name, values);
//         ASSERT_EQ(ret, xsuccess);

//         std::cout << "=====list all @" << height << "=====" << std::endl;
//         for (auto & v : values) {
//             std::cout << v << " ";
//         }
//         std::cout << std::endl;
//     }
// }

// TEST_F(test_block_executed, block_executed_discrete) {
//     xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
//     xobject_ptr_t<xstore_face_t> store_face2 = xstore_factory::create_store_with_memdb();

//     uint64_t notified_count = 0;
//     uint64_t count = 100;

//     auto store = store_face.get();
//     auto store2 = store_face2.get();

//     std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
//     xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);
//     xvblockstore_t* blockstore2 = xblockstorehub_t::instance().create_block_store(*store2, address);

//     test_blockmock_t blockmock(store_face.get());

//     std::vector<base::xvblock_t*> units;

//     std::string prop_name = "aaa";

//     base::xvblock_t *prev_block = (blockstore->get_genesis_block(address).get());
//     base::xvblock_t *curr_block = nullptr;

//     std::vector<base::xvblock_t*> saved_blocks;
//     saved_blocks.push_back(prev_block);
//     for (uint64_t i = 1; i <= count; i++) {

//         std::string value = std::to_string(i);
//         xaccount_cmd_ptr_t cmd = create_or_modify_property(store, address, prop_name, value);
//         curr_block = blockmock.create_sample_block(prev_block, cmd.get(), address);

//         ASSERT_TRUE(blockstore->store_block(curr_block));
//         ASSERT_EQ(curr_block->get_height(), i);
//         uint64_t chainheight = store->get_blockchain_height(address);
//         ASSERT_EQ(chainheight, curr_block->get_height());
//         prev_block = curr_block;
//         saved_blocks.push_back(prev_block);
//     }

//     for (uint64_t i = 1; i <= count; i++) {
//         if (0 != i % 10) {
//             saved_blocks[i]->reset_block_flags();
//             saved_blocks[i]->reset_prev_block(nullptr);
//             saved_blocks[i]->reset_next_block(nullptr);
//             saved_blocks[i]->set_block_flag(base::enum_xvblock_flag_authenticated);
//             saved_blocks[i]->set_block_flag(base::enum_xvblock_flag_locked);
//             saved_blocks[i]->set_block_flag(base::enum_xvblock_flag_committed);
//             ASSERT_EQ(saved_blocks[i]->get_height(), i);
//             ASSERT_TRUE(blockstore2->store_block(saved_blocks[i]));
//         }
//     }
//     // lastest fullunit is height 84
//     ASSERT_EQ(blockstore2->get_latest_executed_block(address)->get_height(), 89);

//     for (uint64_t i = 1; i <= count; i++) {
//         if (0 == i % 10) {
//             saved_blocks[i]->reset_block_flags();
//             saved_blocks[i]->reset_prev_block(nullptr);
//             saved_blocks[i]->reset_next_block(nullptr);
//             saved_blocks[i]->set_block_flag(base::enum_xvblock_flag_authenticated);
//             saved_blocks[i]->set_block_flag(base::enum_xvblock_flag_locked);
//             saved_blocks[i]->set_block_flag(base::enum_xvblock_flag_committed);
//             ASSERT_TRUE(blockstore2->store_block(saved_blocks[i]));
// 	}
//     }
//     ASSERT_EQ(blockstore2->get_latest_executed_block(address)->get_height(), count);

//     std::vector<std::string> values;

//     uint64_t heights[] = {8, 30, 80, 67};
//     for (uint64_t height : heights) {
//         int32_t ret = store_face2->get_list_property(address, height, prop_name, values);
//         ASSERT_EQ(ret, xsuccess);

//         std::cout << "=====list all @" << height << "=====" << std::endl;
//         for (auto & v : values) {
//             std::cout << v << " ";
//         }
//         std::cout << std::endl;
//     }
// }

// TEST_F(test_block_executed, block_execute_but_not_save_bug_1) {
//     xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
//     std::string address = xblocktool_t::make_address_shard_table_account(11);
//     base::xauto_ptr<xvblockstore_t> blockstore{xblockstorehub_t::instance().create_block_store(*store_face.get(), address)};

//     base::xauto_ptr<base::xvblock_t> genesisblock{test_blocktuil::create_genesis_empty_table(address)};
//     base::xauto_ptr<base::xvblock_t> nextblock{xblocktool_t::create_next_emptyblock(genesisblock.get())};
//     test_blocktuil::do_block_consensus_without_set_flag(nextblock.get());

//     nextblock->set_block_flag(base::enum_xvblock_flag_authenticated);
//     xassert(blockstore->store_block(nextblock.get()));
//     {
//         xassert(blockstore->get_latest_cert_block(address)->get_height() == 1);
//         xassert(blockstore->get_latest_locked_block(address)->get_height() == 0);
//         xassert(blockstore->get_latest_committed_block(address)->get_height() == 0);
//         xassert(blockstore->get_latest_executed_block(address)->get_height() == 0);
//     }
//     nextblock->set_block_flag(base::enum_xvblock_flag_locked);
//     xassert(blockstore->store_block(nextblock.get()));
//     {
//         xassert(blockstore->get_latest_cert_block(address)->get_height() == 1);
//         xassert(blockstore->get_latest_locked_block(address)->get_height() == 1);
//         xassert(blockstore->get_latest_committed_block(address)->get_height() == 0);
//         xassert(blockstore->get_latest_executed_block(address)->get_height() == 0);
//     }
//     nextblock->set_block_flag(base::enum_xvblock_flag_committed);
//     xassert(blockstore->store_block(nextblock.get()));
//     {
//         xassert(blockstore->get_latest_cert_block(address)->get_height() == 1);
//         xassert(blockstore->get_latest_locked_block(address)->get_height() == 1);
//         xassert(blockstore->get_latest_committed_block(address)->get_height() == 1);
//         xassert(blockstore->get_latest_connected_block(address)->get_height() == 1);
//         xassert(blockstore->get_latest_executed_block(address)->get_height() == 1);
//     }
// }

// TEST_F(test_block_executed, block_execute_but_not_save_bug_2) {
//     xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
//     std::string address = xblocktool_t::make_address_shard_table_account(11);
//     base::xauto_ptr<xvblockstore_t> blockstore{xblockstorehub_t::instance().create_block_store(*store_face.get(), address)};

//     base::xauto_ptr<base::xvblock_t> genesisblock{test_blocktuil::create_genesis_empty_table(address)};
//     base::xauto_ptr<base::xvblock_t> nextblock_1{xblocktool_t::create_next_emptyblock(genesisblock.get())};
//     test_blocktuil::do_block_consensus_without_set_flag(nextblock_1.get());
//     nextblock_1->set_block_flag(base::enum_xvblock_flag_authenticated);
//     nextblock_1->set_block_flag(base::enum_xvblock_flag_locked);
//     xassert(blockstore->store_block(nextblock_1.get()));

//     base::xauto_ptr<base::xvblock_t> nextblock_2{xblocktool_t::create_next_emptyblock(nextblock_1.get())};
//     test_blocktuil::do_block_consensus_without_set_flag(nextblock_2.get());
//     nextblock_2->set_block_flag(base::enum_xvblock_flag_authenticated);
//     nextblock_2->set_block_flag(base::enum_xvblock_flag_locked);
//     xassert(blockstore->store_block(nextblock_2.get()));

//     base::xauto_ptr<base::xvblock_t> nextblock_3{xblocktool_t::create_next_emptyblock(nextblock_2.get())};
//     test_blocktuil::do_block_consensus_without_set_flag(nextblock_3.get());
//     nextblock_3->set_block_flag(base::enum_xvblock_flag_authenticated);
//     xassert(blockstore->store_block(nextblock_3.get()));

//     base::xauto_ptr<base::xvblock_t> nextblock_4{xblocktool_t::create_next_emptyblock(nextblock_3.get())};
//     test_blocktuil::do_block_consensus_without_set_flag(nextblock_4.get());
//     nextblock_4->set_block_flag(base::enum_xvblock_flag_authenticated);
//     xassert(blockstore->store_block(nextblock_4.get()));

//     base::xauto_ptr<base::xvblock_t> nextblock_5{xblocktool_t::create_next_emptyblock(nextblock_4.get())};
//     test_blocktuil::do_block_consensus_without_set_flag(nextblock_5.get());
//     nextblock_5->set_block_flag(base::enum_xvblock_flag_authenticated);
//     nextblock_5->set_block_flag(base::enum_xvblock_flag_locked);
//     nextblock_5->set_block_flag(base::enum_xvblock_flag_committed);
//     xassert(blockstore->store_block(nextblock_5.get()));

//     {
//         std::cout << blockstore->get_latest_cert_block(address)->get_height() << std::endl;
//         std::cout << blockstore->get_latest_locked_block(address)->get_height() << std::endl;
//         std::cout << blockstore->get_latest_committed_block(address)->get_height() << std::endl;
//         std::cout << blockstore->get_latest_connected_block(address)->get_height() << std::endl;
//         std::cout << blockstore->get_latest_executed_block(address)->get_height() << std::endl;
//         xassert(blockstore->get_latest_cert_block(address)->get_height() == 5);
//         xassert(blockstore->get_latest_locked_block(address)->get_height() == 5);
//         xassert(blockstore->get_latest_committed_block(address)->get_height() == 5);
//         xassert(blockstore->get_latest_connected_block(address)->get_height() == 5);
//         xassert(blockstore->get_latest_executed_block(address)->get_height() == 5);

//         xassert(blockstore->load_block_object(address, 1)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(blockstore->load_block_object(address, 1)->check_block_flag(base::enum_xvblock_flag_connected));
//         xassert(blockstore->load_block_object(address, 2)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(blockstore->load_block_object(address, 2)->check_block_flag(base::enum_xvblock_flag_connected));
//         xassert(blockstore->load_block_object(address, 3)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(blockstore->load_block_object(address, 3)->check_block_flag(base::enum_xvblock_flag_connected));
//         xassert(blockstore->load_block_object(address, 4)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(blockstore->load_block_object(address, 4)->check_block_flag(base::enum_xvblock_flag_connected));
//         xassert(blockstore->load_block_object(address, 5)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(blockstore->load_block_object(address, 5)->check_block_flag(base::enum_xvblock_flag_connected));

//         xassert(store_face->get_vblock(address, 1)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(store_face->get_vblock(address, 1)->check_block_flag(base::enum_xvblock_flag_connected));
//         xassert(store_face->get_vblock(address, 2)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(store_face->get_vblock(address, 2)->check_block_flag(base::enum_xvblock_flag_connected));
//         xassert(store_face->get_vblock(address, 3)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(store_face->get_vblock(address, 3)->check_block_flag(base::enum_xvblock_flag_connected));
//         xassert(store_face->get_vblock(address, 4)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(store_face->get_vblock(address, 4)->check_block_flag(base::enum_xvblock_flag_connected));
//         xassert(store_face->get_vblock(address, 5)->check_block_flag(base::enum_xvblock_flag_executed));
//         xassert(store_face->get_vblock(address, 5)->check_block_flag(base::enum_xvblock_flag_connected));
//     }


// }
