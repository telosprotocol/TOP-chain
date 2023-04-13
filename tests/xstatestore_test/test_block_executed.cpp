#include "gtest/gtest.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#define private public
#define protected public
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
#include "xstatestore/xstatestore_prune.h"
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
    EXPECT_EQ(statestore::xstatestore_hub_t::instance()->get_latest_executed_block_height(common::xtable_address_t::build_from(mocktable.get_account())), max_count - 2);    
}

TEST_F(test_block_executed, get_unit_latest_connectted_change_state) {
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

    auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_change_state(common::xaccount_address_t{mockunits[0].get_account()});
    std::cout << "unitstate " << unitstate->get_bstate()->dump().c_str() << std::endl;
    ASSERT_TRUE(unitstate->height() == max_count-2);
}



TEST_F(test_block_executed, recover_execute_height) {

    class test_xstatestore_executor_t : public statestore::xstatestore_executor_t {
    public:
        test_xstatestore_executor_t(common::xtable_address_t const& table_addr)
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
        test_xstatestore_executor_t state_executor(common::xtable_address_t::build_from(mocktable.get_account()));
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
    statestore::xstatestore_executor_t state_executor{common::xtable_address_t::build_from(mocktable.get_account()), &listener_test};

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
    statestore::xstatestore_executor_t state_executor{common::xtable_address_t::build_from(mocktable.get_account()), &listener_test};
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
    statestore::xstatestore_executor_t state_executor{common::xtable_address_t::build_from(mocktable.get_account()), &listener_test};

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

// TODO(jimmy) can't run succ when run continuously
TEST_F(test_block_executed, xstatestore_table_state_get_BENCH_1) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 10;
    mock::xdatamock_table mocktable(1, 8);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    uint32_t count = 10;
    do {
        statestore::xtablestate_ext_ptr_t tablestate_ext = statestore::xstatestore_hub_t::instance()->get_tablestate_ext_from_block(tableblocks[max_count].get());
        ASSERT_NE(tablestate_ext, nullptr);

        std::error_code ec;
        auto table_accounts = statestore::xstatestore_hub_t::instance()->get_all_accountindex(tableblocks[max_count].get(), ec);
        if (ec) {
            xassert(false);
        }
        ASSERT_EQ(table_accounts.size(), 8);
    } while (count--);
}

TEST_F(test_block_executed, get_history_account_indexs_BENCH_2) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 40;
    mock::xdatamock_table mocktable(1, 8);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }
    for (uint32_t i=0;i<max_count-2;i++) {
        auto _block = blockstore->load_block_object(mocktable, i, base::enum_xvblock_flag_committed, false);
        statestore::xstatestore_hub_t::instance()->on_table_block_committed(_block.get());
    }

    uint64_t old_height = 10;
    xblock_ptr_t old_block = tableblocks[old_height];
    const std::string delete_key = base::xvdbkey_t::create_prunable_state_key(old_block->get_account(), old_block->get_height(), old_block->get_block_hash());
    creator.get_xdb()->single_delete(delete_key);

    statestore::xtablestate_ext_ptr_t tablestate_ext = statestore::xstatestore_hub_t::instance()->get_tablestate_ext_from_block(old_block.get());
    ASSERT_EQ(tablestate_ext, nullptr);

    std::error_code ec;
    auto table_accounts = statestore::xstatestore_hub_t::instance()->get_all_accountindex(old_block.get(), ec);
    if (ec) {
        xassert(false);
    }
    ASSERT_EQ(table_accounts.size(), 8);
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
    statestore::xstatestore_executor_t state_executor{common::xtable_address_t::build_from(mocktable.get_account()), &listener_test};
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

void check_unitstates_stored(mock::xdatamock_table & mocktable, bool all_unitstates_store) {
    statestore::xstatestore_dbaccess_t _dbaccess;
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();
    for (auto & mockunit: mockunits) {
        ASSERT_TRUE(mockunit.get_history_units().front()->get_height() < max_limit_lightunit_count);
        for (auto & unit : mockunit.get_history_units()) {
            if (unit->get_height() == 0) {
                continue;
            }
            auto unitstate = _dbaccess.read_unit_bstate(common::xaccount_address_t{mockunit.get_account()}, unit->get_height(), unit->get_block_hash());
            if (all_unitstates_store || unit->get_height() == max_limit_lightunit_count) {
                xassert(unitstate != nullptr);
            } else {
                // always update latest unitstate to db, so here unitstate can be loaded.
                // xassert(unitstate == nullptr);
            }
        }
    }
}

void store_all_table_blocks(mock::xdatamock_table & mocktable, mock::xvchain_creator & creator) {
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }
}

void execute_all_table_blocks(mock::xdatamock_table & mocktable, mock::xvchain_creator & creator) {
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    for (auto & block : tableblocks) {
        auto state = statestore::xstatestore_hub_t::instance()->get_table_state_by_block(block.get());
        xassert(state != nullptr);
    }
}

TEST_F(test_block_executed, unitstates_store_rule) {
    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();
        uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
        uint64_t max_count = max_limit_lightunit_count + 5;
        mock::xdatamock_table mocktable(1, 4);
        mocktable.genrate_table_chain(max_count, blockstore);

        // rule1 archive nodes need store units and only store fullunit's offchain state
        base::xvchain_t::instance().set_node_type(true, true);
        store_all_table_blocks(mocktable, creator);
        execute_all_table_blocks(mocktable, creator);
        check_unitstates_stored(mocktable, true);
    }
    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();
        uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
        uint64_t max_count = max_limit_lightunit_count + 5;
        mock::xdatamock_table mocktable(1, 4);
        mocktable.genrate_table_chain(max_count, blockstore);

        // rule1 archive nodes need store units and only store fullunit's offchain state
        base::xvchain_t::instance().set_node_type(false, true);
        store_all_table_blocks(mocktable, creator);
        execute_all_table_blocks(mocktable, creator);
        check_unitstates_stored(mocktable, true);        
    }
}

TEST_F(test_block_executed, accountindex_check) {
    {
        mock::xvchain_creator creator;
        base::xvblockstore_t* blockstore = creator.get_blockstore();
        uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
        uint64_t max_count = max_limit_lightunit_count + 5;
        mock::xdatamock_table mocktable(1, 4);
        mocktable.genrate_table_chain(max_count, blockstore);

        // rule1 archive nodes need store units and only store fullunit's offchain state
        base::xvchain_t::instance().set_node_type(true, true);
        store_all_table_blocks(mocktable, creator);
        execute_all_table_blocks(mocktable, creator);

        const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();
        common::xaccount_address_t account_address{mockunits[0].get_account()};
        base::xaccount_index_t accountindex;
        statestore::xstatestore_hub_t::instance()->get_accountindex(LatestConnectBlock, account_address, accountindex);
        data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_accountindex(account_address, accountindex);
        ASSERT_EQ(accountindex.get_latest_unit_height(), max_count-2);
        ASSERT_EQ(accountindex.get_version(), base::enum_xaccountindex_version_state_hash);
        std::string unitstate_bin;
        unitstate->get_bstate()->serialize_to_string(unitstate_bin);
        std::string _state_hash = base::xcontext_t::instance().hash(unitstate_bin, enum_xhash_type_sha2_256);
        ASSERT_EQ(accountindex.get_latest_state_hash(), _state_hash);
        auto unitblock = blockstore->load_unit(account_address.vaccount(), accountindex.get_latest_unit_height(), accountindex.get_latest_unit_hash());
        ASSERT_NE(unitblock, nullptr);
    }
}



TEST_F(test_block_executed, latest_executed_state_1) {
    {
        mock::xvchain_creator creator;
        mock::xdatamock_table mocktable(1, 4);
        auto tablestate = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(common::xtable_address_t::build_from(mocktable.get_account()));
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
        for (auto & block : tableblocks) {
            ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
            statestore::xstatestore_hub_t::instance()->get_tablestate_ext_from_block(block.get());
        }
        auto tablestate2 = statestore::xstatestore_hub_t::instance()->get_table_connectted_state(common::xtable_address_t::build_from(mocktable.get_account()));
        EXPECT_EQ(tablestate2->height(), max_count-2);         
    }
}

void check_all_units_stored(mock::xdatamock_table & mocktable, base::xvblockstore_t* blockstore) {
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }
    for (auto & v : mockunits) {
        for (uint64_t height = 0; height <= v.get_cert_block()->get_height()-2; height++) {
            auto unit = blockstore->load_unit(common::xaccount_address_t{v.get_account()}.vaccount(), height);
            ASSERT_TRUE(nullptr != unit);
        }        
    }
}

TEST_F(test_block_executed, store_units_rule) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    base::xvchain_t::instance().set_node_type(false, false);
    {
        // rec-table should always store units
        uint64_t max_count = 8;
        mock::xdatamock_table mocktable(base::enum_chain_zone_beacon_index, 0, 2);
        mocktable.genrate_table_chain(max_count, blockstore);
        check_all_units_stored(mocktable, blockstore);
    }
    {
        // zec-table should always store units
        uint64_t max_count = 8;
        mock::xdatamock_table mocktable(base::enum_chain_zone_zec_index, 0, 2);
        mocktable.genrate_table_chain(max_count, blockstore);
        check_all_units_stored(mocktable, blockstore);
    }
    {
        // relay-table should always store units
        uint64_t max_count = 8;
        mock::xdatamock_table mocktable(base::enum_chain_zone_relay_index, 0, 2);
        mocktable.genrate_table_chain(max_count, blockstore);
        check_all_units_stored(mocktable, blockstore);
    }        
    base::xvchain_t::instance().set_node_type(true, false);
    // other tables store units if archive node
    {
        uint64_t max_count = 8;
        mock::xdatamock_table mocktable(1, 4);
        mocktable.genrate_table_chain(max_count, blockstore);
        check_all_units_stored(mocktable, blockstore);
    }
}

TEST_F(test_block_executed, store_meta_check) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    base::xvchain_t::instance().set_node_type(true, false);
    // other tables store units if archive node
    {
        uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
        uint64_t max_count = max_limit_lightunit_count + 5;
        mock::xdatamock_table mocktable(1, 4);
        mocktable.genrate_table_chain(max_count, blockstore);
        check_all_units_stored(mocktable, blockstore);

        // const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();
        // for (auto & v : mockunits) {
        //     ASSERT_EQ(max_limit_lightunit_count, blockstore->get_latest_full_block_height(common::xaccount_address_t{v.get_account()}.vaccount()));
        //     ASSERT_EQ(v.get_cert_block()->get_height(), blockstore->get_latest_cert_block_height(common::xaccount_address_t{v.get_account()}.vaccount()));
        //     ASSERT_EQ(v.get_cert_block()->get_height() - 2, blockstore->get_latest_committed_block_height(common::xaccount_address_t{v.get_account()}.vaccount()));
        //     ASSERT_EQ(v.get_cert_block()->get_height() - 2, blockstore->get_latest_connected_block_height(common::xaccount_address_t{v.get_account()}.vaccount()));
        // } 
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
            auto unit = blockstore->load_unit(common::xaccount_address_t{v.get_account()}.vaccount(), 1);
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

TEST_F(test_block_executed, cert_lock_unitstate) {
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

    for (auto & mockunit : mockunits) {
        for (uint64_t height = 1; height <= max_count; height++) {
            auto & tableblock = tableblocks[height];
            common::xaccount_address_t accountaddress(mockunit.get_account());
            base::xaccount_index_t accountindex;
            xassert(true == statestore::xstatestore_hub_t::instance()->get_accountindex_from_table_block(accountaddress, tableblock.get(), accountindex));
            ASSERT_EQ(accountindex.get_latest_unit_height(), height);
            ASSERT_NE(accountindex.get_latest_state_hash(), std::string());
            auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_accountindex(accountaddress, accountindex);
            ASSERT_NE(unitstate, nullptr);
        }
    }
}

TEST_F(test_block_executed, xstatestore_get_state_before_prune) {
    char buffer[200];
    getcwd(buffer, 200);
    std::string dir = buffer;
    std::string cmd = "rm -rf " + dir + "/test_db_xstatestore_get_state_before_prune";
    system(cmd.data());
    std::cout << cmd << std::endl;

    xdbg("xstatestore_get_state_before_prune begin");

    mock::xvchain_creator creator(true, "test_db_xstatestore_get_state_before_prune");
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    uint64_t max_count = 150;
    mock::xdatamock_table mocktable(63, 4);
    mocktable.genrate_table_chain(max_count, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_count + 1);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();

    base::xvchain_t::instance().set_node_type(true, true);
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));        
    }
    for (uint32_t i=1;i<max_count-2;i++) {
        // tableblocks[i]->set_block_flag(base::enum_xvblock_flag_committed);
        // statestore::xstatestore_hub_t::instance()->on_table_block_committed(tableblocks[i].get());
    }

    common::xaccount_address_t unit_addr(mockunits[0].get_account());
    base::xaccount_index_t accountindex;
    

    for (uint32_t i=1;i<max_count-2;i++) {
        // tableblocks[i]->set_block_flag(base::enum_xvblock_flag_committed);
        // statestore::xstatestore_hub_t::instance()->on_table_block_committed(tableblocks[i].get());
        ASSERT_TRUE(statestore::xstatestore_hub_t::instance()->get_accountindex_from_table_block(unit_addr, tableblocks[i].get(), accountindex));
    }

    base::xaccount_index_t accountindex1;
    ASSERT_TRUE(statestore::xstatestore_hub_t::instance()->get_accountindex_from_table_block(unit_addr, tableblocks[1].get(), accountindex1));

    std::shared_ptr<xstatestore_resources_t> para;
    xstatestore_prune_t pruner(common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), para);
    pruner.prune_imp(60);

    xdbg("xstatestore_get_state_before_prune 2222");
    base::xaccount_index_t accountindex2;
    ASSERT_TRUE(statestore::xstatestore_hub_t::instance()->get_accountindex_from_table_block(unit_addr, tableblocks[1].get(), accountindex2));
    ASSERT_EQ(accountindex1, accountindex2);

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

base::xvblock_ptr_t create_unit_for_test(base::xvblock_t* prev_unit, std::string const& binlog, const data::xblock_consensus_para_t & cs_para) {
    uint64_t height = prev_unit->get_height() + 1;
    bool is_full_unit = false;

    data::xunit_block_para_t bodypara;
    bodypara.set_binlog(binlog);
    bodypara.set_fullstate_bin_hash("1111");

    std::shared_ptr<data::xunit_build2_t> vblockmaker = std::make_shared<data::xunit_build2_t>(
        prev_unit->get_account(), height, prev_unit->get_last_block_hash(), is_full_unit, cs_para);
    vblockmaker->create_block_body(bodypara);
    base::xvblock_ptr_t proposal_block = vblockmaker->build_new_block();
    assert(false == proposal_block->get_cert()->is_consensus_flag_has_extend_cert());
    proposal_block->set_block_flag(base::enum_xvblock_flag_authenticated);    
    assert(!proposal_block->get_block_hash().empty());
    return proposal_block;
}

TEST_F(test_block_executed, get_unitstate_by_units) {
    mock::xvchain_creator creator;
    std::string addr = mock::xdatamock_address::make_user_address_random();
    common::xaccount_address_t account_address(addr);
    {
        base::xauto_ptr<base::xvblock_t> genesis_unit = data::xblocktool_t::create_genesis_empty_unit(addr);
        base::xaccount_index_t index = base::xaccount_index_t(base::enum_xaccountindex_version_state_hash, genesis_unit->get_height(), genesis_unit->get_block_hash(), "1", 1);
        auto unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_accountindex(account_address, index);
        ASSERT_NE(nullptr, unitstate);
    }
}