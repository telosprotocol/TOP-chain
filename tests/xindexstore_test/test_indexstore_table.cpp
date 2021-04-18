#include "gtest/gtest.h"
#include "xbasic/xmemory.hpp"
#include "xindexstore/xindexstore_face.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xtableblock_util.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"

using namespace top;
using namespace top::base;
using namespace top::data;
using namespace top::mock;
using namespace top::store;

class test_indexstore_table : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_indexstore_table, account_index_query_1) {
    xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    uint64_t max_block_height = 200;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    for (uint64_t i = 0; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
    }

    xindexstore_face_ptr_t indexstore = xindexstore_factory_t::create_index_store(make_observer(xstore), make_observer(blockstore), mocktable.get_account());
    const std::vector<xdatamock_unit> & datamock_units = mocktable.get_mock_units();
    for (auto & v : datamock_units) {
        auto & account = v.get_account();

        base::xaccount_index_t account_index;
        bool ret = indexstore->get_account_index(account, account_index);
        ASSERT_TRUE(ret);
        ASSERT_NE(account_index.get_latest_unit_height(), 0);
        auto committed_unit = blockstore->get_latest_committed_block(account);
        ASSERT_EQ(committed_unit->get_height(), account_index.get_latest_unit_height());
        std::cout << "account=" << account << " index=" << account_index.dump() << std::endl;
    }
    {
        std::string temp_account1 = xblocktool_t::make_address_user_account("21111111111111111111");
        base::xaccount_index_t account_index;
        bool ret = indexstore->get_account_index(temp_account1, account_index);
        ASSERT_TRUE(ret);
        ASSERT_EQ(account_index.get_latest_unit_height(), 0);
    }
}


TEST_F(test_indexstore_table, account_index_query_2) {
    xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    uint64_t max_block_height = 200;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    for (uint64_t i = 0; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
    }

    xindexstore_face_ptr_t indexstore = xindexstore_factory_t::create_index_store(make_observer(xstore), make_observer(blockstore), mocktable.get_account());
    uint32_t count = 100000;
    while (count--)
    {
        std::string temp_account1 = xblocktool_t::make_address_user_account("21111111111111111111");
        base::xaccount_index_t account_index;
        bool ret = indexstore->get_account_index(temp_account1, account_index);
        ASSERT_TRUE(ret);
        ASSERT_EQ(account_index.get_latest_unit_height(), 0);
    }
}

TEST_F(test_indexstore_table, account_index_query_3) {
    xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    uint64_t max_block_height = 200;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    for (uint64_t i = 0; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
    }

    xindexstore_face_ptr_t indexstore = xindexstore_factory_t::create_index_store(make_observer(xstore), make_observer(blockstore), mocktable.get_account());
    const std::vector<xdatamock_unit> & datamock_units = mocktable.get_mock_units();
    for (auto & v : datamock_units) {
        auto & account = v.get_account();

        xaccount_basic_info_t account_index_info;
        bool ret = indexstore->get_account_basic_info(account, account_index_info);
        ASSERT_TRUE(ret);
    }
    {
        std::string temp_account1 = xblocktool_t::make_address_user_account("21111111111111111111");
        xaccount_basic_info_t account_index_info;
        bool ret = indexstore->get_account_basic_info(temp_account1, account_index_info);
        ASSERT_TRUE(ret);
    }
}

TEST_F(test_indexstore_table, mbt_state_query_1) {
    xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    uint64_t max_block_height = 200;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    for (uint64_t i = 0; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
    }

    xindexstore_face_ptr_t indexstore = xindexstore_factory_t::create_index_store(make_observer(xstore), make_observer(blockstore), mocktable.get_account());
    const std::vector<xdatamock_unit> & datamock_units = mocktable.get_mock_units();
    {
        auto & account = datamock_units[0].get_account();
        base::xaccount_index_t account_index;
        bool ret = indexstore->get_account_index(account, account_index);
        ASSERT_TRUE(ret);
        ASSERT_NE(account_index.get_latest_unit_height(), 0);
        auto committed_unit = blockstore->get_latest_committed_block(account);
        ASSERT_EQ(committed_unit->get_height(), account_index.get_latest_unit_height());
        std::cout << "account=" << account << " index=" << account_index.dump() << std::endl;
    }
    {
        auto & account = datamock_units[0].get_account();
        base::xaccount_index_t account_index;
        xtablestate_ptr_t new_state = indexstore->clone_tablestate();
        ASSERT_NE(new_state, nullptr);
        new_state->get_account_index(account, account_index);
        ASSERT_NE(account_index.get_latest_unit_height(), 0);
        auto committed_unit = blockstore->get_latest_committed_block(account);
        ASSERT_EQ(committed_unit->get_height(), account_index.get_latest_unit_height());
        std::cout << "account=" << account << " index=" << account_index.dump() << std::endl;
    }
}

