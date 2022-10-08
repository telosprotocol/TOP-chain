#include <unistd.h>
#include "gtest/gtest.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_address.hpp"
#define private public
#define protected public
#include "xstatestore/xstatestore_prune.h"
#include "xstatestore/xstatestore_exec.h"

using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::mock;
using namespace top::statestore;

class test_state_prune : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_state_prune, accounts_prune_info) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t max_block_height = 50;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    xaccounts_prune_info_t accounts_prune_infos;
    for (auto & block : tableblocks) {
        accounts_prune_infos.insert_from_tableblock(block.get());
    }
    auto & prune_infos = accounts_prune_infos.get_prune_info();

    auto mock_units = mocktable.get_mock_units();

    EXPECT_EQ(mock_units.size(), prune_infos.size());

    for (auto & mock_unit : mock_units) {
        auto & account = mock_unit.get_account();
        auto last_unit = mock_unit.get_history_units().back();
        auto prune_info_iter = prune_infos.find(account);
        EXPECT_EQ(prune_info_iter != prune_infos.end(), true);
        EXPECT_EQ(prune_info_iter->second, last_unit->get_height());
    }
}

TEST_F(test_state_prune, prune_exec_storage) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    auto xdb = creator.get_xdb();

    uint64_t max_block_height = 50;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    std::shared_ptr<xstatestore_resources_t> para;
    xstatestore_prune_t pruner(common::xaccount_address_t(mocktable.get_vaccount().get_account()), para);

    for (auto & block : tableblocks) {
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), block->get_height(), block->get_block_hash());
        xdb->write(state_key, "test_state" + std::to_string(block->get_height())); // for test.
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), block->get_height(), block->get_viewid());
        xdb->write(offdata_key, "test_offdata" + std::to_string(block->get_height())); // for test.
    }

    pruner.prune_exec_storage(1, 10);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 10 && block->get_block_class() != base::enum_xvblock_class_full));
        std::string value_offdata;
        xdb->read(offdata_key, value_offdata);
        EXPECT_EQ(value_offdata.empty(), (h <= 10 && block->get_block_class() != base::enum_xvblock_class_nil));
    }

    pruner.prune_exec_storage(11, 25);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 25 && block->get_block_class() != base::enum_xvblock_class_full));
        std::string value_offdata;
        xdb->read(offdata_key, value_offdata);
        EXPECT_EQ(value_offdata.empty(), (h <= 25 && block->get_block_class() != base::enum_xvblock_class_nil));
    }
}

TEST_F(test_state_prune, prune_exec_storage_and_cons) {
    char buffer[200];
    getcwd(buffer, 200);
    std::string dir = buffer;
    std::string cmd = "rm -rf " + dir + "/test_db_prune_exec_storage_and_cons";
    system(cmd.data());
    std::cout << cmd << std::endl;
    mock::xvchain_creator creator(true, "test_db_prune_exec_storage_and_cons");
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    auto xdb = creator.get_xdb();

    uint64_t max_block_height = 50;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    std::shared_ptr<xstatestore_resources_t> para;
    xstatestore_prune_t pruner(common::xaccount_address_t(mocktable.get_vaccount().get_account()), para);

    for (auto & block : tableblocks) {
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), block->get_height(), block->get_block_hash());
        xdb->write(state_key, "test_state" + std::to_string(block->get_height())); // for test.
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), block->get_height(), block->get_viewid());
        xdb->write(offdata_key, "test_offdata" + std::to_string(block->get_height())); // for test.
    }

    auto mock_units = mocktable.get_mock_units();

    pruner.prune_exec_storage_and_cons(1, 10);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 10 && block->get_block_class() != base::enum_xvblock_class_full));
        std::string value_offdata;
        xdb->read(offdata_key, value_offdata);
        EXPECT_EQ(value_offdata.empty(), (h <= 10 && block->get_block_class() != base::enum_xvblock_class_nil));
    }

    for (auto & mock_unit : mock_units) {
        auto account = mock_unit.get_account();
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(base::xvaccount_t(account)));
        EXPECT_EQ(9, account_obj->get_lowest_executed_block_height());
    }

    pruner.prune_exec_storage_and_cons(11, 25);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 25 && block->get_block_class() != base::enum_xvblock_class_full));
        std::string value_offdata;
        xdb->read(offdata_key, value_offdata);
        EXPECT_EQ(value_offdata.empty(), (h <= 25 && block->get_block_class() != base::enum_xvblock_class_nil));
    }

    for (auto & mock_unit : mock_units) {
        auto account = mock_unit.get_account();
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(base::xvaccount_t(account)));
        EXPECT_EQ(24, account_obj->get_lowest_executed_block_height());
    }
}

class xexecute_listener_test : public xexecute_listener_face_t {
public:
    void on_executed(uint64_t height) {}
};

TEST_F(test_state_prune, prune_exec_cons) {
    char buffer[200];
    getcwd(buffer, 200);
    std::string dir = buffer;
    std::string cmd = "rm -rf " + dir + "/test_db_prune_exec_cons";
    system(cmd.data());
    std::cout << cmd << std::endl;
    mock::xvchain_creator creator(true, "test_db_prune_exec_cons");
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    auto xdb = creator.get_xdb();

    uint64_t max_block_height = 50;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    xexecute_listener_test listener_test;
    statestore::xstatestore_executor_t state_executor{common::xaccount_address_t{mocktable.get_account()}, &listener_test};
    std::error_code ec;
    for (uint64_t height=0;height<=max_block_height-2;height++) {
        auto block = blockstore->load_block_object(mocktable, height, base::enum_xvblock_flag_committed, false);
        xassert(block != nullptr);
        state_executor.on_table_block_committed(block.get());
        height++;
    }

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        evm_common::xh256_t root;
        auto ret = data::xblockextract_t::get_state_root(block.get(), root);
        EXPECT_EQ(ret, true);

        std::error_code ec;
        auto mpt = state_mpt::xtop_state_mpt::create(common::xaccount_address_t(mocktable.get_vaccount().get_account()), xhash256_t(root.to_bytes()), base::xvchain_t::instance().get_xdbstore(), state_mpt::xstate_mpt_cache_t::instance(), ec);
        xdbg("prune_exec_cons test table:%s,height:%llu,hash:%s", mocktable.get_vaccount().get_account().c_str(), h, xhash256_t(root.to_bytes()).as_hex_str().c_str());
        xassert(mpt != nullptr);
        EXPECT_EQ(mpt != nullptr, true);
    }

    std::shared_ptr<xstatestore_resources_t> para;
    xstatestore_prune_t pruner(common::xaccount_address_t(mocktable.get_vaccount().get_account()), para);
    auto mock_units = mocktable.get_mock_units();

    pruner.prune_exec_storage_and_cons(1, 10);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        evm_common::xh256_t root;
        auto ret = data::xblockextract_t::get_state_root(block.get(), root);
        EXPECT_EQ(ret, true);

        std::error_code ec;
        auto mpt = state_mpt::xtop_state_mpt::create(common::xaccount_address_t(mocktable.get_vaccount().get_account()), xhash256_t(root.to_bytes()), base::xvchain_t::instance().get_xdbstore(), state_mpt::xstate_mpt_cache_t::instance(), ec);
        EXPECT_EQ(mpt != nullptr, true);
    }

    for (auto & mock_unit : mock_units) {
        auto account = mock_unit.get_account();
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(base::xvaccount_t(account)));
        EXPECT_EQ(9, account_obj->get_lowest_executed_block_height());
    }

    pruner.prune_exec_storage_and_cons(11, 25);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 25 && block->get_block_class() != base::enum_xvblock_class_full));
        std::string value_offdata;
        xdb->read(offdata_key, value_offdata);
        EXPECT_EQ(value_offdata.empty(), (h <= 25 && block->get_block_class() != base::enum_xvblock_class_nil));
    }

    for (auto & mock_unit : mock_units) {
        auto account = mock_unit.get_account();
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(base::xvaccount_t(account)));
        EXPECT_EQ(24, account_obj->get_lowest_executed_block_height());
    }
}
