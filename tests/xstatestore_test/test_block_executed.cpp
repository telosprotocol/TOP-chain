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
#include "xstatestore/xstatestore_face.h"
#include "xstatestore/xstatestore_exec.h"
#include "test_common.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;
using namespace top::statestore;

class test_block_executed : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_block_executed, order_execute_block_1) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 10;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    mocktable.store_genesis_units(blockstore);
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    base::xaccount_index_t account_index;
    for (uint64_t i = 0; i <= max_count - 4; i++) {
        auto _block = blockstore->load_block_object(mocktable, i, base::enum_xvblock_flag_committed, false);
        statestore::xstatestore_hub_t::instance()->on_table_block_committed(_block.get());
        for (auto & v : mockunits) {
            auto is_succ = statestore::xstatestore_hub_t::instance()->get_accountindex_from_table_block(common::xaccount_address_t{v.get_account()}, tableblocks[i].get(), account_index);
            xassert(is_succ);
        }
    }

    for (auto & v : mockunits) {
        auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_table(common::xaccount_address_t{v.get_account()}, "latest");
        xassert(nullptr != unitstate);
    }

    EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), max_count - 2);
    EXPECT_EQ(statestore::xstatestore_hub_t::instance()->get_latest_executed_block_height(common::xaccount_address_t{mocktable.get_account()}), max_count - 2);    
}


TEST_F(test_block_executed, recover_execute_height) {

    class test_xstatestore_executor_t : public statestore::xstatestore_executor_t {
    public:
        test_xstatestore_executor_t(common::xaccount_address_t const& table_addr)
        : statestore::xstatestore_executor_t(table_addr, nullptr) {
            init();
        }
        void reset_execute_height(uint64_t height) {
            m_executed_height = height;
        }
        void force_recover_execute_height(uint64_t old_height) {
            recover_execute_height(old_height);
        }
    };

    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 100;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    {
        test_xstatestore_executor_t state_executor(common::xaccount_address_t{mocktable.get_account()});
        for (uint64_t i = 0; i <= max_count - 2; i++) {
            auto _block = blockstore->load_block_object(mocktable, i, base::enum_xvblock_flag_committed, false);
            state_executor.on_table_block_committed(_block.get());
        }
        state_executor.reset_execute_height(max_count/2);        
        EXPECT_EQ(state_executor.get_latest_executed_block_height(), max_count/2); 
        for (uint64_t i = max_count/2; i < max_count/2+20; i++) {
            const std::string state_db_key = base::xvdbkey_t::create_prunable_state_key(common::xaccount_address_t{mocktable.get_account()}.vaccount(), tableblocks[i]->get_height(), tableblocks[i]->get_block_hash());
            base::xvchain_t::instance().get_xdbstore()->delete_value(state_db_key);
        }
        state_executor.force_recover_execute_height(max_count/2);
        EXPECT_EQ(state_executor.get_latest_executed_block_height(), max_count/2+20); 

        EXPECT_EQ(state_executor.get_latest_executed_tablestate_ext()->get_table_state()->height(), max_count - 2);        
    }
}

class xexecute_listener_test : public xexecute_listener_face_t {
public:
    void on_executed(uint64_t height) {}
};


TEST_F(test_block_executed, xstatestore_executor_t_test_1) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = statestore::xstatestore_executor_t::execute_update_limit - 1;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();


    // statestore::xstatestore_hub_t::reset_instance();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    xexecute_listener_test listener_test;
    statestore::xstatestore_executor_t state_executor{common::xaccount_address_t{mocktable.get_account()}, &listener_test};

    {
        std::error_code ec;
        base::xaccount_index_t account_index;
        for (auto & v : mockunits) {
            state_executor.execute_and_get_accountindex(tableblocks[max_count].get(), common::xaccount_address_t{v.get_account()}, account_index, ec);
            if (ec) {
                xassert(false);
            }
            std::cout << "account=" << v.get_account() << " index=" << account_index.dump() << std::endl;
        }

        EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), max_count - 2);
        EXPECT_EQ(state_executor.get_latest_executed_block_height(), max_count - 2);
    }

    {
        std::error_code ec;
        base::xaccount_index_t account_index;
        for (uint32_t i = 0; i < 100; i++) {
            state_executor.execute_and_get_accountindex(tableblocks[max_count].get(), common::xaccount_address_t{mockunits[0].get_account()}, account_index, ec);
            if (ec) {
                xassert(false);
            }
        }
    }

}

TEST_F(test_block_executed, xstatestore_executor_t_test_2) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = statestore::xstatestore_executor_t::execute_update_limit - 1;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    mocktable.store_genesis_units(blockstore);
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    xexecute_listener_test listener_test;
    statestore::xstatestore_executor_t state_executor{common::xaccount_address_t{mocktable.get_account()}, &listener_test};
    std::error_code ec;
    base::xaccount_index_t account_index;
    state_executor.execute_and_get_accountindex(tableblocks[max_count].get(), common::xaccount_address_t{mockunits[0].get_account()}, account_index, ec);
    if (ec) {
        xassert(false);
    }
    std::cout << "account=" << mockunits[0].get_account() << " index=" << account_index.dump() << std::endl;

    EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), max_count - 2);
    EXPECT_EQ(state_executor.get_latest_executed_block_height(), max_count - 2);   
}

TEST_F(test_block_executed, xstatestore_executor_t_test_3) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = statestore::xstatestore_executor_t::execute_update_limit + 5;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    xexecute_listener_test listener_test;
    statestore::xstatestore_executor_t state_executor{common::xaccount_address_t{mocktable.get_account()}, &listener_test};

    {
        std::error_code ec;
        base::xaccount_index_t account_index;
        state_executor.execute_and_get_accountindex(tableblocks[max_count].get(), common::xaccount_address_t{mockunits[0].get_account()}, account_index, ec);
        if (!ec) {
            xassert(false);
        }
        uint64_t expect_height = statestore::xstatestore_executor_t::execute_update_limit;
        EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), expect_height);
        EXPECT_EQ(state_executor.get_latest_executed_block_height(), expect_height);   
    }
    {
        // try again
        std::error_code ec;
        base::xaccount_index_t account_index;
        state_executor.execute_and_get_accountindex(tableblocks[max_count].get(), common::xaccount_address_t{mockunits[0].get_account()}, account_index, ec);
        if (ec) {
            xassert(false);
        }
        std::cout << "account=" << mockunits[0].get_account() << " index=" << account_index.dump() << std::endl;
        EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), max_count-2);
        EXPECT_EQ(state_executor.get_latest_executed_block_height(), max_count-2);           
    }
}

TEST_F(test_block_executed, xstatestore_executor_t_test_5) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 30;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    xexecute_listener_test listener_test;
    statestore::xstatestore_executor_t state_executor{common::xaccount_address_t{mocktable.get_account()}, &listener_test};
    std::error_code ec;
    for (uint64_t height=0;height<=max_count-2;height++) {
        auto block = blockstore->load_block_object(mocktable, height, base::enum_xvblock_flag_committed, false);
        xassert(block != nullptr);
        state_executor.on_table_block_committed(block.get());
        height++;
    }
    {
        auto block = blockstore->load_block_object(mocktable, max_count-2, base::enum_xvblock_flag_committed, false);
        xassert(block != nullptr);
        state_executor.on_table_block_committed(block.get());
    }

    base::xaccount_index_t account_index;
    state_executor.execute_and_get_accountindex(tableblocks[max_count].get(), common::xaccount_address_t{mockunits[0].get_account()}, account_index, ec);
    if (ec) {
        xassert(false);
    }
    EXPECT_EQ(blockstore->get_latest_executed_block_height(mocktable), max_count-2);
    EXPECT_EQ(state_executor.get_latest_executed_block_height(), max_count-2);
}

TEST_F(test_block_executed, latest_executed_state_1) {
    {
        mock::xvchain_creator creator;
        mock::xdatamock_table mocktable(1, 4);
        auto tablestate = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(common::xaccount_address_t{mocktable.get_account()});
        EXPECT_TRUE(tablestate != nullptr);
        EXPECT_EQ(tablestate->height(), 0);
    }

    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();
        uint64_t max_count = 10;
        mock::xdatamock_table mocktable(1, 4);      
        mocktable.genrate_table_chain(max_count, blockstore);
        const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();        
        auto tablestate = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(common::xaccount_address_t{mocktable.get_account()});
        EXPECT_EQ(tablestate->height(), 0);      

        for (auto & block : tableblocks) {
            ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
        }
        auto tablestate2 = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(common::xaccount_address_t{mocktable.get_account()});
        EXPECT_EQ(tablestate2->height(), max_count-2);         
    }

    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();
        uint64_t max_count = 10;
        mock::xdatamock_table mocktable(1, 4);      
        mocktable.genrate_table_chain(max_count, blockstore);
        const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();        
        auto tablestate = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(common::xaccount_address_t{mocktable.get_account()});
        EXPECT_EQ(tablestate->height(), 0);      

        for (auto & block : tableblocks) {
            if (block->get_height() == 6) {
                continue;
            }
            ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
        }
        auto tablestate2 = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(common::xaccount_address_t{mocktable.get_account()});
        EXPECT_EQ(tablestate2->height(), 3);

        ASSERT_TRUE(blockstore->store_block(mocktable, tableblocks[6].get()));
        auto tablestate3 = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(common::xaccount_address_t{mocktable.get_account()});
        EXPECT_EQ(tablestate3->height(), max_count-2);        
    }

}

TEST_F(test_block_executed, not_store_units_1) {
    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();
        base::xvchain_t::instance().set_node_type(true, true);
        uint64_t max_count = 8;
        mock::xdatamock_table mocktable(1, 4);
        mocktable.genrate_table_chain(max_count, blockstore);
        const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
        xassert(tableblocks.size() == max_count + 1);
        const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

        for (auto & block : tableblocks) {
            ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
        }

        for (auto & v : mockunits) {
            auto unit = blockstore->load_block_object(common::xaccount_address_t{v.get_account()}.vaccount(), 1, base::enum_xvblock_flag_authenticated, false);
            xassert(nullptr != unit);
        }
    }
}

TEST_F(test_block_executed, not_store_units_2) {
    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();
        base::xvchain_t::instance().set_node_type(false, true);
        uint64_t max_count = 8;
        mock::xdatamock_table mocktable(1, 4);
        mocktable.genrate_table_chain(max_count, blockstore);
        const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
        xassert(tableblocks.size() == max_count + 1);
        const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

        for (auto & block : tableblocks) {
            ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
        }

        for (auto & v : mockunits) {
            auto unit = blockstore->load_block_object(common::xaccount_address_t{v.get_account()}.vaccount(), 1, base::enum_xvblock_flag_authenticated, false);
            xassert(nullptr == unit);
            auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_table(common::xaccount_address_t{v.get_account()}, "latest");
            xassert(nullptr != unitstate);
            xassert(unitstate->height() > 1);
        }        
    }
}

TEST_F(test_block_executed, get_unit_state_by_table_1) {
    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();
        base::xvchain_t::instance().set_node_type(false, true);
        uint64_t max_count = 8;
        mock::xdatamock_table mocktable(1, 4);
        mocktable.genrate_table_chain(max_count, blockstore);
        const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
        xassert(tableblocks.size() == max_count + 1);
        const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

        for (auto & block : tableblocks) {
            ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
        }

        common::xaccount_address_t unit_address(mockunits[0].get_account());
        {
            auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_table(unit_address, "latest");
            ASSERT_NE(nullptr, unitstate);
        }
        for (uint64_t i=0;i<=max_count;i++) {
            auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_table(unit_address, std::to_string(i));
            ASSERT_NE(nullptr, unitstate);            
        }
    }
}



TEST_F(test_block_executed, xstatestore_execute_BENCH) {
// total time of store blocks: 2651 ms
// dbmeta key_size:1052256 value_size = 6865708 key_count = 17231 write_count = 23739 read_count = 3241
// hash_calc_count = 28805

// total time of execute blocks: 5729 ms
// dbmeta key_size:1752869 value_size = 8335650 key_count = 28132 write_count = 38600 read_count = 10195
// hash_calc_count = 20809

    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 1000;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

{
    xhashtest_t::hash_calc_count = 0;
    auto t1 = base::xtime_utl::time_now_ms();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }
    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time of store blocks: " << t2 - t1 << " ms" << std::endl;
    db::xdb_meta_t dbmeta = creator.get_xdb()->get_meta();
    std::cout << "dbmeta key_size:" << dbmeta.m_db_key_size
    << " value_size = " << dbmeta.m_db_value_size
    << " key_count = " << dbmeta.m_key_count  
    << " write_count = " << dbmeta.m_write_count
    << " read_count = " << dbmeta.m_read_count
    << std::endl;
    std::cout << "hash_calc_count = " << xhashtest_t::hash_calc_count << std::endl<< std::endl;    
}

{
    xhashtest_t::hash_calc_count = 0;
    auto t1 = base::xtime_utl::time_now_ms();
    
    for (uint32_t i=0;i<max_count-2;i++) {
        tableblocks[i]->set_block_flag(base::enum_xvblock_flag_committed);
        statestore::xstatestore_hub_t::instance()->on_table_block_committed(tableblocks[i].get());
    }
    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time of execute blocks: " << t2 - t1 << " ms" << std::endl;   
    db::xdb_meta_t dbmeta = creator.get_xdb()->get_meta();
    std::cout << "dbmeta key_size:" << dbmeta.m_db_key_size
    << " value_size = " << dbmeta.m_db_value_size
    << " key_count = " << dbmeta.m_key_count  
    << " write_count = " << dbmeta.m_write_count
    << " read_count = " << dbmeta.m_read_count
    << std::endl;
    std::cout << "hash_calc_count = " << xhashtest_t::hash_calc_count << std::endl;        
}

}

TEST_F(test_block_executed, xstatestore_execute_BENCH_2) {
// test result in release
// total time of store blocks: 617 ms
// dbmeta key_size:1042158 value_size = 6847566 key_count = 17023 write_count = 23531 read_count = 3101
// hash_calc_count = 22843

// total time of execute blocks: 740 ms
// dbmeta key_size:1694253 value_size = 8274938 key_count = 26934 write_count = 42299 read_count = 12954
// hash_calc_count = 16786

    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 1000;
    mock::xdatamock_table mocktable(1, 100);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

{
    xhashtest_t::hash_calc_count = 0;
    auto t1 = base::xtime_utl::time_now_ms();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }
    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time of store blocks: " << t2 - t1 << " ms" << std::endl;
    db::xdb_meta_t dbmeta = creator.get_xdb()->get_meta();
    std::cout << "dbmeta key_size:" << dbmeta.m_db_key_size
    << " value_size = " << dbmeta.m_db_value_size
    << " key_count = " << dbmeta.m_key_count  
    << " write_count = " << dbmeta.m_write_count
    << " read_count = " << dbmeta.m_read_count
    << std::endl;
    std::cout << "hash_calc_count = " << xhashtest_t::hash_calc_count << std::endl<< std::endl;    
}

{
    xhashtest_t::hash_calc_count = 0;
    auto t1 = base::xtime_utl::time_now_ms();
    
    for (uint32_t i=0;i<max_count-2;i++) {
        tableblocks[i]->set_block_flag(base::enum_xvblock_flag_committed);
        statestore::xstatestore_hub_t::instance()->on_table_block_committed(tableblocks[i].get());
    }
    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time of execute blocks: " << t2 - t1 << " ms" << std::endl;   
    db::xdb_meta_t dbmeta = creator.get_xdb()->get_meta();
    std::cout << "dbmeta key_size:" << dbmeta.m_db_key_size
    << " value_size = " << dbmeta.m_db_value_size
    << " key_count = " << dbmeta.m_key_count  
    << " write_count = " << dbmeta.m_write_count
    << " read_count = " << dbmeta.m_read_count
    << std::endl;
    std::cout << "hash_calc_count = " << xhashtest_t::hash_calc_count << std::endl;        
}

}

TEST_F(test_block_executed, xvaccount_BENCH) {
// total time: 5759 ms  in release    after optimize   total time: 12 ms
    uint32_t count = 10000000;
{
    auto t1 = base::xtime_utl::time_now_ms();
    common::xaccount_address_t _address{"Ta0000@1"};
    for (uint32_t i =0;i<count;i++) {
        _address.vaccount();
    }
    auto t2 = base::xtime_utl::time_now_ms();
    std::cout << "total time: " << t2 - t1 << " ms" << std::endl;
}

}