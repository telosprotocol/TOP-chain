#include <vector>

#include "gtest/gtest.h"

#include "xbasic/xobject_ptr.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xmbus/xevent_store.h"
#include "xmbus/xevent_behind.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xgenesis_data.h"
#include "xdata/tests/test_blockutl.hpp"

#include "xstore/xstore.h"
#include "xstore/xstore_face.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"

using namespace top;
using namespace top::store;
using namespace top::data;

class test_xstore_data_check : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

uint64_t get_gmttime_s() {
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    return (uint64_t)val.tv_sec;
}

TEST_F(test_xstore_data_check, block_set_consecutive) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::map<std::string, std::string> prop_list;

    test_datamock_t datamock(store1.get());

    uint64_t count = 50;
    auto& config_register = top::config::xconfig_register_t::get_instance();

    uint32_t full_unit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");

    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        assert(store1->set_vblock(genesis_block));
        assert(store1->execute_block(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
        ASSERT_TRUE(store->execute_block(genesis_block));
    }

    for (uint64_t i = 1; i <= count; i++) {
        datamock.create_unit(address, prop_list);
    }

    for (uint64_t i = 1; i <= count; i++) {
        auto block = store1->get_block_by_height(address, i);
        ASSERT_EQ(block->get_height(), i);
        if (block->get_height() % full_unit_count == 0) {
            ASSERT_TRUE(block->is_fullunit());
        } else {
            ASSERT_TRUE(block->is_lightunit());
        }
    }
    auto blockchain = store1->clone_account(address);
    ASSERT_NE(blockchain, nullptr);
}

TEST_F(test_xstore_data_check, block_set_non_consecutive_1) {
    std::map<std::string, std::string> prop_list;
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();

    test_datamock_t datamock(store1.get());


    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");

    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
        ASSERT_TRUE(store1->execute_block(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
        ASSERT_TRUE(store->execute_block(genesis_block));
    }

    uint64_t heights[] = {1, 3, 5, 7, 9, 2, 4, 6, 8, 10};
    std::vector<xblock_ptr_t> cached_blocks;
    uint64_t count = sizeof(heights) / sizeof(heights[0]);
    for (uint64_t i = 0; i < count; i++) {
        cached_blocks.push_back(datamock.create_unit(address, prop_list));
    }

    for (uint64_t i = 0; i < count; i++) {
        uint64_t height = heights[i];
        auto block = store1->get_block_by_height(address, height);
        ASSERT_EQ(block->get_height(), height);
        ASSERT_TRUE(store->set_vblock(block));
    }

    for (uint64_t i = 0; i < count; i++) {
        ASSERT_TRUE(store->execute_block(cached_blocks[i].get()));
    }

    account = store1->clone_account(address);
    std::cout << "account 1 balance: " << account->balance() << std::endl;

    account = store->clone_account(address);
    std::cout << "account 2 balance: " << account->balance() << std::endl;

    for (uint64_t i = 1; i <= count; i++) {
        auto block = store->get_block_by_height(address, i);
        ASSERT_EQ(block->get_height(), i);
    }
    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, count);
    account = store->clone_account(address);
    ASSERT_EQ(account->balance(), count*100);
}

TEST_F(test_xstore_data_check, block_set_non_consecutive_2) {
    std::map<std::string, std::string> prop_list;
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();

    test_datamock_t datamock(store1.get());

    uint64_t count = 10;
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");

    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
        ASSERT_TRUE(store1->execute_block(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
        ASSERT_TRUE(store->execute_block(genesis_block));
    }

    std::vector<xblock_ptr_t> cached_blocks;
    uint64_t heights[10] = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
    for (uint64_t i = 0; i < count; i++) {
        cached_blocks.push_back(datamock.create_unit(address, prop_list));
    }

    for (uint64_t i = 0; i < count; i++) {
        uint64_t block_height = heights[i];
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_EQ(block->get_height(), block_height);
        ASSERT_TRUE(store->set_vblock(block));
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }
    for (uint64_t i = 1; i <= count; i++) {
        auto block = store->get_block_by_height(address, i);
        ASSERT_EQ(block->get_height(), i);
    }

    for (uint64_t i = 0; i < count; i++) {
        ASSERT_TRUE(store->execute_block(cached_blocks[i].get()));
    }

    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, count);
    account = store->clone_account(address);
    ASSERT_NE(account, nullptr);
    ASSERT_EQ(account->balance(), count*100);
}

TEST_F(test_xstore_data_check, block_set_consecutive_1_BENCH) {
    std::map<std::string, std::string> prop_list;
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store3 = xstore_factory::create_store_with_memdb();

    test_datamock_t datamock(store1.get());

    uint64_t count = 10000;
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");

    auto account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
        ASSERT_TRUE(store->execute_block(genesis_block));
    }

    for (uint64_t i = 0; i < count; i++) {
        datamock.create_unit(address, prop_list);
    }

    std::thread t1([&]() {
        std::cout << "t1 start" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        for (uint64_t i = 0; i < count; i++) {
            uint64_t block_height = i + 1;
            auto block = store1->get_block_by_height(address, block_height);
            ASSERT_EQ(block->get_height(), block_height);
            ASSERT_TRUE(store->set_vblock(block));
            ASSERT_TRUE(store->execute_block(block));
            auto account = store->clone_account(address);
            ASSERT_EQ(account->is_state_behind(), false);
        }
    });

    std::thread t2([&]() {
        std::cout << "t2 start" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        for (uint64_t i = 0; i < count; i++) {
            uint64_t block_height = i + 1;
            auto block = store1->get_block_by_height(address, block_height);
            ASSERT_EQ(block->get_height(), block_height);
            ASSERT_TRUE(store->set_vblock(block));
            ASSERT_TRUE(store->execute_block(block));
            auto account = store->clone_account(address);
            ASSERT_EQ(account->is_state_behind(), false);
        }
    });
    t1.join();
    t2.join();
}

#if 0
TEST_F(test_xstore_data_check, block_set_non_consecutive_3) {
    std::map<std::string, std::string> prop_list;
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();

    test_datamock_t datamock(store1.get());

    uint64_t count = 10;
    uint64_t sync_count = 9;
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");

    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
        ASSERT_TRUE(store1->execute_block(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
        ASSERT_TRUE(store->execute_block(genesis_block));
    }

    uint64_t heights[9] = {10, 9, 8, 7, 6, 5, 4, 3, 2};
    for (uint64_t i = 0; i < count; i++) {
        datamock.create_unit(address, prop_list);
    }

    for (uint64_t i = 0; i < sync_count; i++) {
        uint64_t block_height = heights[i];
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_EQ(block->get_height(), block_height);
        ASSERT_TRUE(store->set_vblock(block));
    }
    for (uint64_t i = 2; i <= count; i++) {
        auto block = store->get_block_by_height(address, i);
        ASSERT_EQ(block->get_height(), i);
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }
    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, count);
    account = store->clone_account(address);
    ASSERT_TRUE(account->is_state_behind());
}

TEST_F(test_xstore_data_check, fullunit_set_1) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::map<std::string, std::string> prop_list;

    test_datamock_t datamock(store1.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
        ASSERT_TRUE(store1->execute_block(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
        ASSERT_TRUE(store->execute_block(genesis_block));
    }
    auto& config_register = top::config::xconfig_register_t::get_instance();

    uint32_t full_unit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);

    uint64_t count = full_unit_count * 2;
    for (uint64_t i = 0; i < count; i++) {
        datamock.create_unit(address, prop_list);
    }

    uint64_t heights[full_unit_count * 2];
    for (uint64_t i = 0; i < count; i++) {
        heights[i] = count - i;
    }

    for (uint64_t i = 0; i < count; i++) {
        uint64_t block_height = heights[i];
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_EQ(block->get_height(), block_height);
        ASSERT_TRUE(store->set_vblock(block));
    }
    for (uint64_t i = 1; i <= count; i++) {
        auto block = store->get_block_by_height(address, i);
        ASSERT_EQ(block->get_height(), i);
    }
    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, count);
    account = store->clone_account(address);
    ASSERT_FALSE(account->is_state_behind());
    ASSERT_EQ(account->balance(), 100 * (full_unit_count * 2 - 2));
}

TEST_F(test_xstore_data_check, fullunit_set_2) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::map<std::string, std::string> prop_list;

    test_datamock_t datamock(store1.get());

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
        ASSERT_TRUE(store1->execute_block(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
        ASSERT_TRUE(store->execute_block(genesis_block));
    }
    auto& config_register = top::config::xconfig_register_t::get_instance();

    uint32_t full_unit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    uint64_t count = full_unit_count * 2;
    for (uint64_t i = 1; i <= count; i++) {
        datamock.create_unit(address, prop_list);
    }

    uint64_t heights[full_unit_count];
    for (uint64_t i = 0; i < full_unit_count; i++) {
        heights[i] = count - i - 1;
    }

    for (uint64_t i = 0; i < full_unit_count; i++) {
        uint64_t block_height = heights[i];
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_TRUE(store->set_vblock(block));
    }
    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, count - 1);
    account = store->clone_account(address);
    ASSERT_FALSE(account->is_state_behind());
    ASSERT_EQ(account->get_last_height(), chainheight);
    ASSERT_EQ(account->balance(), 100 * (full_unit_count * 2 - 2));
}

TEST_F(test_xstore_data_check, fullunit_set_3) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::map<std::string, std::string> prop_list;

    test_datamock_t datamock(store1.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
    }
    auto& config_register = top::config::xconfig_register_t::get_instance();

    uint32_t full_unit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    uint64_t count = full_unit_count * 2;
    for (uint64_t i = 0; i < count; i++) {
        datamock.create_unit(address, prop_list);
    }

    uint64_t heights[full_unit_count];
    for (uint64_t i = 0; i < full_unit_count; i++) {
        heights[i] = count - i;
    }

    xdbg("==================[update blockchain]==============");
    for (uint64_t i = 0; i < full_unit_count - 1; i++) {
        uint64_t block_height = heights[i];
        std::cout << "block height " << block_height << std::endl;
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_TRUE(store->set_vblock(block));
    }
    for (uint64_t i = 1; i < full_unit_count - 1; i++) {
        uint64_t block_height = heights[i];
        auto block = store->get_block_by_height(address, block_height);
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }
    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, count);
    account = store->clone_account(address);
    ASSERT_FALSE(account->is_state_behind());
    ASSERT_NE(account->get_last_height(), 0);
}

TEST_F(test_xstore_data_check, fullunit_set_4) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::map<std::string, std::string> prop_list;

    test_datamock_t datamock(store1.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
    }
    auto& config_register = top::config::xconfig_register_t::get_instance();

    uint32_t full_unit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    uint64_t count = full_unit_count * 2;
    for (uint64_t i = 0; i < count; i++) {
        datamock.create_unit(address, prop_list);
    }

    uint64_t heights[full_unit_count];
    for (uint64_t i = 0; i < full_unit_count; i++) {
        heights[i] = count - i;
    }

    xdbg("==================[update blockchain]==============");
    for (uint64_t i = 1; i < full_unit_count - 1; i++) {
        uint64_t block_height = heights[i];
        std::cout << "block height " << block_height << std::endl;
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_TRUE(store->set_vblock(block));
    }
    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, count-1);
    account = store->clone_account(address);
    ASSERT_TRUE(account->is_state_behind());
    ASSERT_EQ(account->get_last_height(), 0);
    ASSERT_EQ(account->balance(), 0);
}

TEST_F(test_xstore_data_check, fullunit_set_5) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = store1->clone_account(address);
        ASSERT_EQ(account, nullptr);
    }
    {
        xaccount_context_t context(address, store1.get(), {});
        auto fullunit = xblock_maker::make_full_unit(&context, {});
        fullunit->set_prev_height(10);
        fullunit->calc_block_hash();
        store1->set_block(fullunit);

        auto account = store1->clone_account(address);
        ASSERT_EQ(fullunit->get_height(), 11);
        ASSERT_EQ(account->get_chain_height(), 11);
        ASSERT_EQ(account->get_last_height(), 11);
        ASSERT_FALSE(account->is_state_behind());
    }
}

TEST_F(test_xstore_data_check, missing_block_1) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::map<std::string, std::string> prop_list;

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
    }

    test_datamock_t datamock(store1.get());

    uint64_t count = 10;
    uint64_t sync_count = 5;
    uint64_t heights[5] = {10, 8, 6, 4, 2};
    for (uint64_t i = 0; i < count; i++) {
        datamock.create_unit(address, prop_list);
    }

    for (uint64_t i = 0; i < sync_count; i++) {
        uint64_t block_height = heights[i];
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_EQ(block->get_height(), block_height);
        ASSERT_TRUE(store->set_vblock(block));
    }
    for (uint64_t i = 1; i < sync_count; i++) {
        auto block = store->get_block_by_height(address, heights[i]);
        ASSERT_EQ(block->get_height(), heights[i]);
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }
    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, count);
    account = store->clone_account(address);
    ASSERT_TRUE(account->is_state_behind());

}

TEST_F(test_xstore_data_check, missing_block_2) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::map<std::string, std::string> prop_list;

    test_datamock_t datamock(store1.get());

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
    }
    uint64_t count = 60;
    uint64_t heights[] = {1, 2, 3, 4, 5, 6, 7, 10, 11, 12, 13, 14, 15};
    size_t sync_count = sizeof(heights) / sizeof(heights[0]);
    for (uint64_t i = 1; i <= count; ++i) {
        datamock.create_unit(address, prop_list);
    }

    for (uint64_t i = 0; i < sync_count; ++i) {
        uint64_t block_height = heights[i];
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_EQ(block->get_height(), block_height);
        ASSERT_TRUE(store->set_vblock(block));
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }
    for (uint64_t i = 0; i < sync_count; i++) {
        auto block = store->get_block_by_height(address, heights[i]);
        ASSERT_EQ(block->get_height(), heights[i]);
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }

    for (uint64_t i = 36; i <= count; ++i) {
        uint64_t block_height = i;
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_EQ(block->get_height(), block_height);
        ASSERT_TRUE(store->set_vblock(block));
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }

    for (uint64_t i = 36; i <= count; i++) {
        auto block = store->get_block_by_height(address, i);
        ASSERT_EQ(block->get_height(), i);
    }

    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, 60);
    account = store->clone_account(address);
    ASSERT_FALSE(account->is_state_behind());
    ASSERT_EQ(account->get_chain_height(), count);
}

TEST_F(test_xstore_data_check, missing_block_3) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    std::map<std::string, std::string> prop_list;

    test_datamock_t datamock(store1.get());

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
    }
    uint64_t count = 60;
    for (uint64_t i = 1; i <= count; ++i) {
        datamock.create_unit(address, prop_list);
    }

    for (uint64_t i = 1; i < (count - 1); ++i) {

        auto block = store1->get_block_by_height(address, i);
        ASSERT_EQ(block->get_height(), i);
        ASSERT_TRUE(store->set_vblock(block));
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }

    // missing the (count - 1)'s block
    for (uint64_t i = count; i <= count; ++i) {
        uint64_t block_height = i;
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_EQ(block->get_height(), block_height);
        ASSERT_TRUE(store->set_vblock(block));
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }

    uint64_t chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, 60);
    account = store->clone_account(address);
    ASSERT_TRUE(account->is_state_behind());
    ASSERT_EQ(account->get_chain_height(), count);

    // syncing the (count - 1)'s block
    for (uint64_t i = count - 1; i <= count - 1; ++i) {
        uint64_t block_height = i;
        auto block = store1->get_block_by_height(address, block_height);
        ASSERT_EQ(block->get_height(), block_height);
        ASSERT_TRUE(store->set_vblock(block));
        auto blockchain = store->clone_account(address);
        ASSERT_NE(blockchain, nullptr);
    }

    chainheight = store->get_blockchain_height(address);
    ASSERT_EQ(chainheight, 60);
    account = store->clone_account(address);
    ASSERT_FALSE(account->is_state_behind());
    ASSERT_EQ(account->get_chain_height(), count);
}

TEST_F(test_xstore_data_check, property_confirm_height_1) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = store1->clone_account(address);
        ASSERT_EQ(account, nullptr);
    }
    {
        xaccount_context_t context(address, store1.get(), {});
        auto fullunit = xblock_maker::make_full_unit(&context, {});
        fullunit->set_prev_height(10);
        fullunit->calc_block_hash();
        store1->set_block(fullunit);

        auto account = store1->clone_account(address);
        ASSERT_EQ(fullunit->get_height(), 11);
        ASSERT_EQ(account->get_chain_height(), 11);
        ASSERT_EQ(account->get_last_height(), 11);
        ASSERT_FALSE(account->is_state_behind());
        ASSERT_EQ(account->get_last_height(), fullunit->get_height());
    }
}
#endif

TEST_F(test_xstore_data_check, property_confirm_height_2) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store1.get());

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    uint64_t count = 10;
    for (uint64_t i = 0; i < count; i++) {
        datamock.create_unit(address, {});
    }
    auto account = store1->clone_account(address);
    ASSERT_EQ(account->get_chain_height(), count);
    ASSERT_EQ(account->get_last_height(), count);
    ASSERT_FALSE(account->is_state_behind());
    ASSERT_EQ(account->get_last_height(), account->get_chain_height());
}
