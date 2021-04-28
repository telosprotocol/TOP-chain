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
    xtablestate_ptr_t tablestate = indexstore->clone_tablestate(tables[max_block_height]);

    const std::vector<xdatamock_unit> & datamock_units = mocktable.get_mock_units();
    for (auto & v : datamock_units) {
        auto & account = v.get_account();

        base::xaccount_index_t account_index;
        bool ret = tablestate->get_account_index(account, account_index);
        ASSERT_TRUE(ret);
        auto highqc_unit = blockstore->get_latest_cert_block(account);
        ASSERT_EQ(highqc_unit->get_height(), account_index.get_latest_unit_height());
        std::cout << "account=" << account << " index=" << account_index.dump() << std::endl;
    }
    {
        std::string temp_account1 = xblocktool_t::make_address_user_account("21111111111111111111");
        base::xaccount_index_t account_index;
        bool ret = tablestate->get_account_index(temp_account1, account_index);
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
    const std::vector<xdatamock_unit> & datamock_units = mocktable.get_mock_units();
    for (auto & v : datamock_units) {
        auto & account = v.get_account();
        const std::vector<xblock_ptr_t> & units = v.get_history_units();
        ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(account), units[0].get()));
    }
    for (uint64_t i = 0; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
    }

    xindexstore_face_ptr_t indexstore = xindexstore_factory_t::create_index_store(make_observer(xstore), make_observer(blockstore), mocktable.get_account());
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

TEST_F(test_indexstore_table, get_state_by_block_1) {
    xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    uint64_t max_block_height = 10;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    for (uint64_t i = 0; i <= max_block_height; i++) {
        ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
    }

    xindexstore_face_ptr_t indexstore = xindexstore_factory_t::create_index_store(make_observer(xstore), make_observer(blockstore), mocktable.get_account());

    xtablestate_ptr_t tablestate1 = indexstore->clone_tablestate(tables[max_block_height]);
    xassert(tablestate1->get_binlog_height() == max_block_height);
    xtablestate_ptr_t tablestate2 = indexstore->clone_tablestate(tables[max_block_height-1]);
    xassert(tablestate2->get_binlog_height() == max_block_height-1);
    xtablestate_ptr_t tablestate3 = indexstore->clone_tablestate(tables[max_block_height-2]);
    xassert(tablestate3->get_binlog_height() == max_block_height-2);
}
TEST_F(test_indexstore_table, get_state_by_block_2) {
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

    xtablestate_ptr_t tablestate1 = indexstore->clone_tablestate(tables[max_block_height]);
    xassert(tablestate1->get_binlog_height() == max_block_height);
    xtablestate_ptr_t tablestate2 = indexstore->clone_tablestate(tables[max_block_height-1]);
    xassert(tablestate2->get_binlog_height() == max_block_height-1);
    xtablestate_ptr_t tablestate3 = indexstore->clone_tablestate(tables[max_block_height-2]);
    xassert(tablestate3->get_binlog_height() == max_block_height-2);
}

TEST_F(test_indexstore_table, get_state_by_block_3) {
    xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    uint64_t max_block_height = 1000;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);
    for (uint64_t i = max_block_height; i >= 0; i--) {
        if (tables[i]->get_block_class() == base::enum_xvblock_class_full) {
            std::cout << "full height " << tables[i]->get_height() << std::endl;
            tables[i]->reset_block_offdata(nullptr);
            ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
            break;
        } else {
            ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
        }
    }

    xassert(blockstore->get_latest_cert_block(base::xvaccount_t(mocktable.get_account()))->get_height() == max_block_height);
    xassert(blockstore->get_latest_locked_block(base::xvaccount_t(mocktable.get_account()))->get_height() == max_block_height-1);
    xassert(blockstore->get_latest_committed_block(base::xvaccount_t(mocktable.get_account()))->get_height() == max_block_height-2);
    uint64_t connect_height = blockstore->get_latest_connected_block(base::xvaccount_t(mocktable.get_account()))->get_height();
    std::cout << "connect_height " << connect_height << std::endl;
    // xassert(connect_height == max_block_height-2);
    uint64_t executed_height = blockstore->get_latest_executed_block(base::xvaccount_t(mocktable.get_account()))->get_height();
    std::cout << "executed_height " << executed_height << std::endl;
    xassert(executed_height == 0);

    xindexstore_face_ptr_t indexstore = xindexstore_factory_t::create_index_store(make_observer(xstore), make_observer(blockstore), mocktable.get_account());

    xtablestate_ptr_t tablestate1 = indexstore->clone_tablestate(tables[max_block_height]);
    xassert(tablestate1 == nullptr);
}

TEST_F(test_indexstore_table, get_state_by_block_4) {
    xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    uint64_t max_block_height = 1000;
    xdatamock_table mocktable;
    mocktable.genrate_table_chain(max_block_height);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    xassert(tables.size() == max_block_height+1);

    xobject_ptr_t<base::xvboffdata_t> offdata;
    uint64_t full_block_height;
    for (uint64_t i = max_block_height; i >= 0; i--) {
        if (tables[i]->get_block_class() == base::enum_xvblock_class_full) {
            std::cout << "full height " << tables[i]->get_height() << std::endl;
            full_block_height = tables[i]->get_height();
            xassert(tables[i]->get_offdata() != nullptr);
            tables[i]->get_offdata()->add_ref();
            offdata.attach(tables[i]->get_offdata());
            tables[i]->reset_block_offdata(nullptr);
            ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
            break;
        } else {
            ASSERT_TRUE(blockstore->store_block(base::xvaccount_t(mocktable.get_account()), tables[i].get()));
        }
    }

    uint64_t executed_height = blockstore->get_latest_executed_block(base::xvaccount_t(mocktable.get_account()))->get_height();
    std::cout << "executed_height " << executed_height << std::endl;
    xassert(executed_height == 0);

    base::xauto_ptr<base::xvblock_t>  latest_full_block = blockstore->get_latest_full_block(base::xvaccount_t(mocktable.get_account()));
    xassert(latest_full_block->get_height() == full_block_height);
    xassert(latest_full_block->get_offdata() == nullptr);
    latest_full_block->reset_block_offdata(offdata.get());
    xassert(latest_full_block->get_offdata() != nullptr);
    blockstore->store_block(base::xvaccount_t(mocktable.get_account()), latest_full_block.get());

    uint64_t executed_height2 = blockstore->get_latest_executed_block(base::xvaccount_t(mocktable.get_account()))->get_height();
    std::cout << "executed_height " << executed_height2 << std::endl;
    xassert(executed_height2 == max_block_height - 2);

    xindexstore_face_ptr_t indexstore = xindexstore_factory_t::create_index_store(make_observer(xstore), make_observer(blockstore), mocktable.get_account());

    xtablestate_ptr_t tablestate1 = indexstore->clone_tablestate(tables[max_block_height]);
    xassert(tablestate1 != nullptr);
}

TEST_F(test_indexstore_table, get_state_by_block_5) {
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

    uint32_t count = 10000;
    while (count--) {
        for (uint64_t i = max_block_height-2; i <= max_block_height; i++) {
            xtablestate_ptr_t tablestate1 = indexstore->clone_tablestate(tables[i]);
            xassert(tablestate1->get_height() == i);
        }
    }
}
