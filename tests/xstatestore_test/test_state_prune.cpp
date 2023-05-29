#include "gtest/gtest.h"
#include "tests/mock/xdatamock_address.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"

#include <unistd.h>
#define private public
#define protected public
#include "xstatestore/xstatestore_exec.h"
#include "xstatestore/xstatestore_prune.h"

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
    base::xvblockstore_t * blockstore = creator.get_blockstore();

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
    char buffer[200];
    getcwd(buffer, 200);
    std::string dir = buffer;
    std::string cmd = "rm -rf " + dir + "/test_db_prune_exec_storage";
    system(cmd.data());
    std::cout << cmd << std::endl;
    mock::xvchain_creator creator(true, "test_db_prune_exec_storage");
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    auto xdb = creator.get_xdb();

    uint64_t max_block_height = 100;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    std::shared_ptr<xstatestore_resources_t> para;
    xstatestore_prune_t pruner(common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), para);

    base::xvchain_t::instance().set_node_type(true, false);

    for (auto & block : tableblocks) {
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), block->get_height(), block->get_block_hash());
        xdb->write(state_key, "test_state" + std::to_string(block->get_height()));  // for test.
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), block->get_height(), block->get_viewid());
        xdb->write(offdata_key, "test_offdata" + std::to_string(block->get_height()));  // for test.
    }

    pruner.prune_imp(60);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        assert(value_state.empty() == (h <= 20 && block->get_block_class() != base::enum_xvblock_class_full));
        EXPECT_EQ(value_state.empty(), (h <= 20 && block->get_block_class() != base::enum_xvblock_class_full));
        // std::string value_offdata;
        // xdb->read(offdata_key, value_offdata);
        // EXPECT_EQ(value_offdata.empty(), (h <= 20 && block->get_block_class() != base::enum_xvblock_class_nil));
    }

    pruner.prune_imp(80);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 40 && block->get_block_class() != base::enum_xvblock_class_full));
        // not prune offdata any more
        // std::string value_offdata;
        // xdb->read(offdata_key, value_offdata);
        // EXPECT_EQ(value_offdata.empty(), (h <= 40 && block->get_block_class() != base::enum_xvblock_class_nil));
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
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    auto xdb = creator.get_xdb();
    base::xvchain_t::instance().set_node_type(true, true);

    uint64_t max_block_height = 100;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }
    for (auto & block : tableblocks) {
        auto state = statestore::xstatestore_hub_t::instance()->get_table_state_by_block(block.get());
        xassert(state != nullptr);
    }

    std::shared_ptr<xstatestore_resources_t> para;
    xstatestore_prune_t pruner(common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), para);

    base::xvchain_t::instance().set_node_type(true, true);

    for (auto & block : tableblocks) {
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), block->get_height(), block->get_block_hash());
        xdb->write(state_key, "test_state" + std::to_string(block->get_height()));  // for test.
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), block->get_height(), block->get_viewid());
        xdb->write(offdata_key, "test_offdata" + std::to_string(block->get_height()));  // for test.
    }

    auto mock_units = mocktable.get_mock_units();

    pruner.prune_imp(60);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 20 && block->get_block_class() != base::enum_xvblock_class_full));
        // not prune offdata any more
        // std::string value_offdata;
        // xdb->read(offdata_key, value_offdata);
        // EXPECT_EQ(value_offdata.empty(), (h <= 20 && block->get_block_class() != base::enum_xvblock_class_nil));
    }

    for (auto & mock_unit : mock_units) {
        auto account = mock_unit.get_account();
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(base::xvaccount_t(account)));
        EXPECT_NE(0, account_obj->get_lowest_executed_block_height());// TODO(jimmy) prune unitstate for archive node
    }

    pruner.prune_imp(80);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 40 && block->get_block_class() != base::enum_xvblock_class_full));
        // not prune offdata any more
        // std::string value_offdata;
        // xdb->read(offdata_key, value_offdata);
        // EXPECT_EQ(value_offdata.empty(), (h <= 40 && block->get_block_class() != base::enum_xvblock_class_nil));
    }

    for (auto & mock_unit : mock_units) {
        auto account = mock_unit.get_account();
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(base::xvaccount_t(account)));
        EXPECT_NE(0, account_obj->get_lowest_executed_block_height()); // not prune unitstate for archive node
    }

    statestore::xstatestore_dbaccess_t _dbaccess;
    for (auto & mockunit : mock_units) {
        for (auto & unit : mockunit.get_history_units()) {
            auto unitstate = _dbaccess.read_fullunit_bstate(common::xaccount_address_t{mockunit.get_account()}, unit->get_height(), unit->get_block_hash());
            if (unit->is_fullunit()) {
                xassert(unitstate != nullptr);
            }
        }    
    }
}

class xexecute_listener_test : public xexecute_listener_face_t {
public:
    void on_executed(uint64_t height) {
    }
};

TEST_F(test_state_prune, prune_exec_cons) {
    char buffer[200];
    getcwd(buffer, 200);
    std::string dir = buffer;
    std::string cmd = "rm -rf " + dir + "/test_db_prune_exec_cons";
    system(cmd.data());
    std::cout << cmd << std::endl;
    mock::xvchain_creator creator(true, "test_db_prune_exec_cons");
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    auto xdb = creator.get_xdb();
    base::xvchain_t::instance().set_node_type(false, true);

    uint64_t max_block_height = 100;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    xexecute_listener_test listener_test;
    statestore::xstatestore_executor_t state_executor{common::xtable_address_t::build_from(mocktable.get_account()), &listener_test};

    for (uint64_t height = 0; height <= max_block_height - 2; height++) {
        auto block = blockstore->load_block_object(mocktable, height, base::enum_xvblock_flag_committed, false);
        xassert(block != nullptr);
        state_executor.on_table_block_committed(block.get());
        height++;
    }

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        std::error_code ec;

        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto const & root = data::xblockextract_t::get_state_root(block.get(), ec);
        EXPECT_TRUE(!ec);

        auto mpt = state_mpt::xstate_mpt_t::create(
            common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), root, base::xvchain_t::instance().get_xdbstore(), ec);
        xdbg("prune_exec_cons test table:%s,height:%llu,hash:%s", mocktable.get_vaccount().get_account().c_str(), h, root.hex().c_str());
        EXPECT_TRUE(!ec);
        EXPECT_TRUE(mpt != nullptr);
    }

    std::shared_ptr<xstatestore_resources_t> para;
    xstatestore_prune_t pruner(common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), para);
    base::xvchain_t::instance().set_node_type(false, true);

    for (auto & block : tableblocks) {
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), block->get_height(), block->get_block_hash());
        xdb->write(state_key, "test_state" + std::to_string(block->get_height()));  // for test.
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), block->get_height(), block->get_viewid());
        xdb->write(offdata_key, "test_offdata" + std::to_string(block->get_height()));  // for test.
    }

    auto mock_units = mocktable.get_mock_units();

    pruner.prune_imp(60);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        std::error_code ec;

        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);

        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 20 && block->get_block_class() != base::enum_xvblock_class_full));
        // std::string value_offdata;
        // xdb->read(offdata_key, value_offdata);
        // EXPECT_EQ(value_offdata.empty(), (h <= 30 && block->get_block_class() != base::enum_xvblock_class_nil));

        auto const & root = data::xblockextract_t::get_state_root(block.get(), ec);
        EXPECT_EQ(!ec, true);

        auto mpt = state_mpt::xstate_mpt_t::create(
            common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), root, base::xvchain_t::instance().get_xdbstore(), ec);
        EXPECT_EQ(mpt != nullptr, (h >= 20));
        EXPECT_EQ(ec.value() == 0, (h >= 20));
    }

    for (auto & mock_unit : mock_units) {
        auto account = mock_unit.get_account();
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(base::xvaccount_t(account)));
        EXPECT_EQ(19, account_obj->get_lowest_executed_block_height());
    }

    pruner.prune_imp(80);

    for (uint64_t h = 1; h <= max_block_height - 2; h++) {
        std::error_code ec;

        auto block = blockstore->load_block_object(mocktable.get_vaccount(), h, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto state_key = base::xvdbkey_t::create_prunable_state_key(mocktable.get_vaccount(), h, block->get_block_hash());
        auto offdata_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(mocktable.get_account(), h, block->get_viewid());
        std::string value_state;
        xdb->read(state_key, value_state);
        EXPECT_EQ(value_state.empty(), (h <= 40 && block->get_block_class() != base::enum_xvblock_class_full));
        // std::string value_offdata;
        // xdb->read(offdata_key, value_offdata);
        // EXPECT_EQ(value_offdata.empty(), (h <= 55 && block->get_block_class() != base::enum_xvblock_class_nil));

        auto const & root = data::xblockextract_t::get_state_root(block.get(), ec);
        EXPECT_EQ(!ec, true);

        auto mpt = state_mpt::xstate_mpt_t::create(
            common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), root, base::xvchain_t::instance().get_xdbstore(), ec);
        EXPECT_EQ(mpt != nullptr, (h >= 40));
    }

    for (auto & mock_unit : mock_units) {
        auto account = mock_unit.get_account();
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(base::xvaccount_t(account)));
        EXPECT_EQ(39, account_obj->get_lowest_executed_block_height());
    }

    {
        std::error_code ec;

        auto block = blockstore->load_block_object(mocktable.get_vaccount(), 98, base::enum_xvblock_flag_committed, false);
        EXPECT_NE(block, nullptr);
        auto const & root = data::xblockextract_t::get_state_root(block.get(), ec);
        EXPECT_EQ(!ec, true);

        auto mpt = state_mpt::xstate_mpt_t::create(
            common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), root, base::xvchain_t::instance().get_xdbstore(), ec);
        xassert(mpt != nullptr);

        // mpt->prune(root, ec);
    }
}

TEST_F(test_state_prune, prune_exec_cons_rec) {
    char buffer[200];
    getcwd(buffer, 200);
    std::string dir = buffer;
    std::string cmd = "rm -rf " + dir + "/test_db_prune_exec_cons_rec";
    system(cmd.data());
    std::cout << cmd << std::endl;
    mock::xvchain_creator creator(true, "test_db_prune_exec_cons_rec");
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    auto xdb = creator.get_xdb();
    base::xvchain_t::instance().set_node_type(false, true);

    uint64_t max_block_height = 100;
    mock::xdatamock_table mocktable(base::enum_chain_zone_beacon_index, 0, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    for (auto & block : tableblocks) {
        auto state = statestore::xstatestore_hub_t::instance()->get_table_state_by_block(block.get());
        xassert(state != nullptr);
    }

    std::shared_ptr<xstatestore_resources_t> para;
    xstatestore_prune_t pruner(common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), para);
    base::xvchain_t::instance().set_node_type(false, true);

    auto mock_units = mocktable.get_mock_units();

    pruner.prune_imp(80);

    for (auto & mock_unit : mock_units) {
        auto account = mock_unit.get_account();
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(base::xvaccount_t(account)));
        EXPECT_NE(0, account_obj->get_lowest_executed_block_height());
    }

    statestore::xstatestore_dbaccess_t _dbaccess;
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    const std::vector<xdatamock_unit> & mockunits = mocktable.get_mock_units();
    for (auto & mockunit: mockunits) {
        ASSERT_TRUE(mockunit.get_history_units().back()->get_height() > max_limit_lightunit_count);
        for (auto & unit : mockunit.get_history_units()) {
            if (unit->get_height() == 0) {
                continue;
            }
            auto unitstate = _dbaccess.read_fullunit_bstate(common::xaccount_address_t{mockunit.get_account()}, unit->get_height(), unit->get_block_hash());
            if (unit->get_height() % max_limit_lightunit_count == 0) {
                xassert(unitstate != nullptr);
            }
        }
    }
}

TEST_F(test_state_prune, prune_height) {
    mock::xvchain_creator creator(true);
    std::shared_ptr<xstatestore_resources_t> para;
    mock::xdatamock_table mocktable(1, 4);
    xstatestore_prune_t pruner(common::xtable_address_t::build_from(mocktable.get_vaccount().get_account()), para);

    uint64_t keep_table_states_max_num = XGET_CONFIG(keep_table_states_max_num);
    uint64_t prune_table_state_diff = XGET_CONFIG(prune_table_state_diff);
    uint64_t prune_table_state_max = XGET_CONFIG(prune_table_state_max);

    uint64_t from_height;
    uint64_t to_height;
    uint64_t lowest_keep_height;
    bool ret = pruner.need_prune(0);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(keep_table_states_max_num);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(keep_table_states_max_num - 1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(keep_table_states_max_num + 1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(prune_table_state_diff - 1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(prune_table_state_diff);
    EXPECT_EQ(ret, true);
    ret = pruner.need_prune(prune_table_state_diff + 1);
    EXPECT_EQ(ret, true);

    ret = pruner.get_prune_section(0, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(keep_table_states_max_num, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(keep_table_states_max_num - 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(keep_table_states_max_num + 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(prune_table_state_diff - 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(prune_table_state_diff, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(from_height, 1);
    EXPECT_EQ(to_height, prune_table_state_diff - keep_table_states_max_num);
    ret = pruner.get_prune_section(prune_table_state_diff + 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(from_height, 1);
    EXPECT_EQ(to_height, prune_table_state_max);

    pruner.set_pruned_height(1);
    ret = pruner.need_prune(0);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(keep_table_states_max_num);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(keep_table_states_max_num - 1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(keep_table_states_max_num + 1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(prune_table_state_diff - 1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(prune_table_state_diff);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(prune_table_state_diff + 1);
    EXPECT_EQ(ret, true);

    ret = pruner.get_prune_section(0, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(keep_table_states_max_num, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(keep_table_states_max_num - 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(keep_table_states_max_num + 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(prune_table_state_diff - 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(prune_table_state_diff, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(prune_table_state_diff + 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, true);
    EXPECT_EQ(from_height, 2);
    EXPECT_EQ(to_height, prune_table_state_diff + 1 - keep_table_states_max_num);

    pruner.set_pruned_height(256);
    ret = pruner.need_prune(0);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(keep_table_states_max_num);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(keep_table_states_max_num - 1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(keep_table_states_max_num + 1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(prune_table_state_diff - 1);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(prune_table_state_diff);
    EXPECT_EQ(ret, false);
    ret = pruner.need_prune(prune_table_state_diff + 1);
    EXPECT_EQ(ret, false);

    ret = pruner.get_prune_section(0, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(keep_table_states_max_num, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(keep_table_states_max_num - 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(keep_table_states_max_num + 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(prune_table_state_diff - 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(prune_table_state_diff, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
    ret = pruner.get_prune_section(prune_table_state_diff + 1, from_height, to_height, lowest_keep_height);
    EXPECT_EQ(ret, false);
}

TEST_F(test_state_prune, mpt_prune_BENCH) {
    std::string db_path = "test_db_mpt_prune_bench";
    char buffer[200];
    getcwd(buffer, 200);
    std::string dir = buffer;
    std::string cmd = "rm -rf " + dir + "/" + db_path;
    system(cmd.data());
    std::cout << cmd << std::endl;
    mock::xvchain_creator creator(true, db_path);
    auto xdbstore = base::xvchain_t::instance().get_xdbstore();

    uint32_t const user_count = 50000;
    uint32_t const user_change_num_once = 250;
    uint32_t const mpt_all_num = 512;
    uint32_t const mpt_prune_num = 256;

    uint16_t tableid = 0;
    auto table_addr = xblocktool_t::make_address_shard_table_account(tableid);

    std::vector<std::string> user_addrs;
    std::vector<xh256_t> mpt_root_vec;

    xh256_t null_root;
    std::error_code ec;
    auto base_mpt = state_mpt::xstate_mpt_t::create(common::xtable_address_t::build_from(table_addr), null_root, xdbstore, ec);
    if (ec) {
        assert(false);
    }

    for (uint32_t i = 0; i < user_count; i++) {
        xaddress_key_pair_t addr_pair = xdatamock_address::make_unit_address_with_key(tableid);
        auto & addr = addr_pair.m_address;
        user_addrs.push_back(addr);

        std::string unithash = addr + static_cast<char>(rand());
        std::string statehash = addr + static_cast<char>(rand());
        base::xaccount_index_t index(base::enum_xaccountindex_version_state_hash, 1, unithash, statehash, 1);
        base_mpt->set_account_index(common::xaccount_address_t{addr}, index, ec);
    }
    // std::cout << "mpt before commit:0" << std::endl;
    base_mpt->commit(ec);
    if (ec) {
        assert(false);
    }

    // std::cout << "mpt commit:0" << std::endl;
    mpt_root_vec.push_back(base_mpt->get_root_hash(ec));
    // std::cout << "mpt get root:0" << std::endl;

    for (uint32_t j = 0; j < mpt_all_num; j++) {
        base_mpt = state_mpt::xstate_mpt_t::create(common::xtable_address_t::build_from(table_addr), mpt_root_vec.back(), xdbstore, ec);
        for (uint32_t k = 0; k < user_change_num_once; k++) {
            uint32_t pos = rand() % user_count;
            auto & addr = user_addrs[pos];
            std::string unithash = addr + std::to_string(j);
            std::string statehash = addr + "a" + "j";
            base::xaccount_index_t index(base::enum_xaccountindex_version_state_hash, j, unithash, statehash, j);

            base_mpt->set_account_index(common::xaccount_address_t{addr}, index, ec);
        }
        // std::cout << "mpt before commit:" << j+1 << std::endl;
        base_mpt->commit(ec);
        if (ec) {
            assert(false);
        }

        base_mpt->prune(ec);
        if (ec) {
            assert(false);
        }

        assert(base_mpt->original_root_hash() == mpt_root_vec.back());

        // std::cout << "mpt commit:" << j+1 << std::endl;
        mpt_root_vec.push_back(base_mpt->get_root_hash(ec));
        // std::cout << "mpt get root:" << j+1 << std::endl;
        if (j%50 == 0) {
            std::cout << "mpt commit:" << j  << "/" << mpt_all_num << std::endl;
        }
    }

    auto last_keep_mpt_root = mpt_root_vec[mpt_prune_num];

    auto last_keep_mpt = state_mpt::xstate_mpt_t::create(common::xtable_address_t::build_from(table_addr), last_keep_mpt_root, xdbstore, ec);
    if (ec) {
        assert(false);
    }


#ifdef ENABLE_METRICS
    uint32_t db_read_before_prune = XMETRICS_GAUGE_GET_VALUE(metrics::db_read);
    uint32_t db_read_last = db_read_before_prune;
    uint32_t db_write_before_prune = XMETRICS_GAUGE_GET_VALUE(metrics::db_write);
    uint32_t db_write_last = db_write_before_prune;
    uint32_t db_delete_before_prune = XMETRICS_GAUGE_GET_VALUE(metrics::db_delete);
    uint32_t db_delete_last = db_delete_before_prune;
    uint32_t db_delete_range_before_prune = XMETRICS_GAUGE_GET_VALUE(metrics::db_delete_range);
    uint32_t db_delete_range_last = db_delete_range_before_prune;

    auto t1 = base::xtime_utl::time_now_ms();
    std::cout << "before prune. db_read " << db_read_last << ", db_write " << db_write_last << ", db_delete " << db_delete_last << ", db_delete_range " << db_delete_range_last
            << "time:" << t1 << std::endl;
#else
    auto t1 = base::xtime_utl::time_now_ms();
    std::cout << "before prune.time:" << t1 << std::endl;
#endif

    //std::unordered_set<xh256_t> pruned_hashes;
    //for (uint32_t l = 0; l < mpt_prune_num; l++) {
        // xinfo("mpt_prune_BENCH before prune mpt idx:%u,db_read:%u", l, db_read_now - db_read_last);
        last_keep_mpt->commit_pruned(mpt_root_vec, ec);
        // auto t_now = base::xtime_utl::time_now_ms();
        // uint32_t db_read_now = XMETRICS_GAUGE_GET_VALUE(metrics::db_read);
        // uint32_t db_write_now = XMETRICS_GAUGE_GET_VALUE(metrics::db_write);
        // uint32_t db_delete_now = XMETRICS_GAUGE_GET_VALUE(metrics::db_delete);
        // uint32_t db_delete_range_now = XMETRICS_GAUGE_GET_VALUE(metrics::db_delete_range);
        // std::cout << "prune:" << l << " time cost(ms) " << (t_now - t_tmp) << ", db_read " << (db_read_now - db_read_last) << ", db_write " << (db_write_now - db_write_last)
        //           << ", db_delete " << (db_delete_now - db_delete_last) << ", db_delete_range " << (db_delete_range_now - db_delete_range_last) << std::endl;
        // xinfo("mpt_prune_BENCH before prune mpt idx:%u,db_read:%u", l, db_read_now - db_read_last);
        // t_tmp = t_now;
        // db_read_last = db_read_now;
        //if (ec) {
            //assert(false);
        //}
        //if (l%50 == 0) {
            //std::cout << "mpt prune:" << l << "/" << mpt_prune_num << std::endl;
        //}
    //}
    auto t2 = base::xtime_utl::time_now_ms();
#ifdef ENABLE_METRICS
    uint32_t db_read_after_prune = XMETRICS_GAUGE_GET_VALUE(metrics::db_read);
    uint32_t db_write_after_prune = XMETRICS_GAUGE_GET_VALUE(metrics::db_write);
    uint32_t db_delete_after_prune = XMETRICS_GAUGE_GET_VALUE(metrics::db_delete);
    uint32_t db_delete_range_after_prune = XMETRICS_GAUGE_GET_VALUE(metrics::db_delete_range);
    std::cout << "prune total time cost(ms):" << (t2 - t1) << ", ave:" << ((t2 - t1) / mpt_prune_num) << ", total db read:" << (db_read_after_prune - db_read_before_prune)
              << ", ave:" << ((db_read_after_prune - db_read_before_prune) / mpt_prune_num) << ", total db write:" << (db_write_after_prune - db_write_before_prune)
              << ", ave:" << ((db_write_after_prune - db_write_before_prune) / mpt_prune_num) << ", total db delete:" << (db_delete_after_prune - db_delete_before_prune)
              << ", ave:" << ((db_delete_after_prune - db_delete_before_prune) / mpt_prune_num)
              << ", total db delete range:" << (db_delete_range_after_prune - db_delete_range_before_prune)
              << ", ave:" << ((db_delete_range_after_prune - db_delete_range_before_prune) / mpt_prune_num) << std::endl;
#else
    std::cout << "prune total time cost(ms):" << (t2 - t1) << ", ave:" << ((t2 - t1) / mpt_prune_num) << ", prune called " << mpt_prune_num << std::endl;
#endif

    t2 = base::xtime_utl::time_now_ms();
    //last_keep_mpt->commit_pruned(pruned_hashes, ec);
    //if (ec) {
        //assert(false);
    //}
    auto t3 = base::xtime_utl::time_now_ms();
#ifdef ENABLE_METRICS
    uint32_t db_read_after_commit = XMETRICS_GAUGE_GET_VALUE(metrics::db_read);
    uint32_t db_write_after_commit = XMETRICS_GAUGE_GET_VALUE(metrics::db_write);
    uint32_t db_delete_after_commit = XMETRICS_GAUGE_GET_VALUE(metrics::db_delete);
    uint32_t db_delete_range_after_commit = XMETRICS_GAUGE_GET_VALUE(metrics::db_delete_range);
    std::cout << "commit prune cost:" << (t3 - t2) << ", commit db read:" << (db_read_after_commit - db_read_after_prune)
              << ", commit db write:" << (db_write_after_commit - db_write_after_prune) << ", commit db delete:" << (db_delete_after_commit - db_delete_after_prune)
              << ", commit db delete range:" << (db_delete_range_after_commit - db_delete_range_after_prune) << std::endl;
#else
    std::cout << "commit prune cost:" << (t3 - t2) << std::endl;
#endif
}

//-----------------first test result------------------//
// before prune. db_read 54647
// prune:0 time cost(ms) 292, db_read 24825
// prune:1 time cost(ms) 99, db_read 10906
// prune:2 time cost(ms) 98, db_read 10893
// prune:3 time cost(ms) 101, db_read 10879
// prune:4 time cost(ms) 101, db_read 10861
// prune:5 time cost(ms) 97, db_read 10845
// prune:6 time cost(ms) 108, db_read 10834
// prune:7 time cost(ms) 98, db_read 10819
// prune:8 time cost(ms) 116, db_read 10804
// prune:9 time cost(ms) 113, db_read 10787
// prune:10 time cost(ms) 104, db_read 10773
// prune:11 time cost(ms) 95, db_read 10764
// prune:12 time cost(ms) 102, db_read 10756
// prune:13 time cost(ms) 96, db_read 10742
// prune:14 time cost(ms) 96, db_read 10725
// prune:15 time cost(ms) 101, db_read 10714
// prune:16 time cost(ms) 106, db_read 10700
// prune:17 time cost(ms) 99, db_read 10682
// prune:18 time cost(ms) 97, db_read 10667
// prune:19 time cost(ms) 97, db_read 10647
// prune:20 time cost(ms) 93, db_read 10629
// prune:21 time cost(ms) 93, db_read 10608
// prune:22 time cost(ms) 97, db_read 10590
// prune:23 time cost(ms) 94, db_read 10575
// prune:24 time cost(ms) 92, db_read 10562
// prune:25 time cost(ms) 93, db_read 10548
// prune:26 time cost(ms) 95, db_read 10529
// prune:27 time cost(ms) 103, db_read 10513
// prune:28 time cost(ms) 96, db_read 10494
// prune:29 time cost(ms) 96, db_read 10476
// prune:30 time cost(ms) 95, db_read 10460
// prune:31 time cost(ms) 94, db_read 10439
// prune:32 time cost(ms) 93, db_read 10423
// prune:33 time cost(ms) 96, db_read 10405
// prune:34 time cost(ms) 94, db_read 10387
// prune:35 time cost(ms) 92, db_read 10372
// prune:36 time cost(ms) 93, db_read 10353
// prune:37 time cost(ms) 92, db_read 10333
// prune:38 time cost(ms) 103, db_read 10315
// prune:39 time cost(ms) 95, db_read 10296
// prune:40 time cost(ms) 95, db_read 10274
// prune:41 time cost(ms) 94, db_read 10261
// prune:42 time cost(ms) 92, db_read 10245
// prune:43 time cost(ms) 91, db_read 10231
// prune:44 time cost(ms) 95, db_read 10218
// prune:45 time cost(ms) 91, db_read 10201
// prune:46 time cost(ms) 92, db_read 10175
// prune:47 time cost(ms) 93, db_read 10159
// prune:48 time cost(ms) 92, db_read 10146
// prune:49 time cost(ms) 103, db_read 10127
// prune:50 time cost(ms) 93, db_read 10114
// prune:51 time cost(ms) 94, db_read 10096
// prune:52 time cost(ms) 96, db_read 10073
// prune:53 time cost(ms) 92, db_read 10047
// prune:54 time cost(ms) 91, db_read 10023
// prune:55 time cost(ms) 94, db_read 10006
// prune:56 time cost(ms) 90, db_read 9991
// prune:57 time cost(ms) 90, db_read 9977
// prune:58 time cost(ms) 91, db_read 9960
// prune:59 time cost(ms) 91, db_read 9941
// prune:60 time cost(ms) 107, db_read 9921
// prune:61 time cost(ms) 92, db_read 9904
// prune:62 time cost(ms) 89, db_read 9886
// prune:63 time cost(ms) 92, db_read 9872
// prune:64 time cost(ms) 90, db_read 9851
// prune:65 time cost(ms) 92, db_read 9827
// prune:66 time cost(ms) 90, db_read 9804
// prune:67 time cost(ms) 90, db_read 9770
// prune:68 time cost(ms) 88, db_read 9748
// prune:69 time cost(ms) 89, db_read 9726
// prune:70 time cost(ms) 90, db_read 9697
// prune:71 time cost(ms) 99, db_read 9677
// prune:72 time cost(ms) 88, db_read 9656
// prune:73 time cost(ms) 88, db_read 9630
// prune:74 time cost(ms) 91, db_read 9602
// prune:75 time cost(ms) 87, db_read 9578
// prune:76 time cost(ms) 93, db_read 9553
// prune:77 time cost(ms) 86, db_read 9517
// prune:78 time cost(ms) 87, db_read 9484
// prune:79 time cost(ms) 86, db_read 9460
// prune:80 time cost(ms) 86, db_read 9435
// prune:81 time cost(ms) 85, db_read 9418
// prune:82 time cost(ms) 89, db_read 9392
// prune:83 time cost(ms) 97, db_read 9369
// prune:84 time cost(ms) 91, db_read 9337
// prune:85 time cost(ms) 87, db_read 9314
// prune:86 time cost(ms) 86, db_read 9279
// prune:87 time cost(ms) 86, db_read 9254
// prune:88 time cost(ms) 92, db_read 9224
// prune:89 time cost(ms) 89, db_read 9196
// prune:90 time cost(ms) 82, db_read 9165
// prune:91 time cost(ms) 83, db_read 9140
// prune:92 time cost(ms) 82, db_read 9114
// prune:93 time cost(ms) 82, db_read 9093
// prune:94 time cost(ms) 94, db_read 9065
// prune:95 time cost(ms) 89, db_read 9036
// prune:96 time cost(ms) 88, db_read 8999
// prune:97 time cost(ms) 87, db_read 8978
// prune:98 time cost(ms) 84, db_read 8943
// prune:99 time cost(ms) 86, db_read 8916
// prune:100 time cost(ms) 84, db_read 8890
// prune:101 time cost(ms) 83, db_read 8856
// prune:102 time cost(ms) 83, db_read 8827
// prune:103 time cost(ms) 82, db_read 8803
// prune:104 time cost(ms) 81, db_read 8776
// prune:105 time cost(ms) 82, db_read 8747
// prune:106 time cost(ms) 83, db_read 8716
// prune:107 time cost(ms) 93, db_read 8681
// prune:108 time cost(ms) 85, db_read 8648
// prune:109 time cost(ms) 81, db_read 8612
// prune:110 time cost(ms) 82, db_read 8587
// prune:111 time cost(ms) 82, db_read 8548
// prune:112 time cost(ms) 81, db_read 8512
// prune:113 time cost(ms) 78, db_read 8474
// prune:114 time cost(ms) 81, db_read 8437
// prune:115 time cost(ms) 77, db_read 8409
// prune:116 time cost(ms) 78, db_read 8382
// prune:117 time cost(ms) 78, db_read 8355
// prune:118 time cost(ms) 84, db_read 8315
// prune:119 time cost(ms) 99, db_read 8278
// prune:120 time cost(ms) 88, db_read 8236
// prune:121 time cost(ms) 84, db_read 8210
// prune:122 time cost(ms) 77, db_read 8174
// prune:123 time cost(ms) 79, db_read 8136
// prune:124 time cost(ms) 77, db_read 8107
// prune:125 time cost(ms) 78, db_read 8072
// prune:126 time cost(ms) 75, db_read 8046
// prune:127 time cost(ms) 76, db_read 8017
// prune:128 time cost(ms) 74, db_read 7968
// prune:129 time cost(ms) 75, db_read 7934
// prune:130 time cost(ms) 74, db_read 7898
// prune:131 time cost(ms) 74, db_read 7866
// prune:132 time cost(ms) 86, db_read 7832
// prune:133 time cost(ms) 78, db_read 7799
// prune:134 time cost(ms) 76, db_read 7766
// prune:135 time cost(ms) 76, db_read 7740
// prune:136 time cost(ms) 79, db_read 7702
// prune:137 time cost(ms) 74, db_read 7667
// prune:138 time cost(ms) 73, db_read 7622
// prune:139 time cost(ms) 72, db_read 7581
// prune:140 time cost(ms) 71, db_read 7538
// prune:141 time cost(ms) 71, db_read 7500
// prune:142 time cost(ms) 72, db_read 7466
// prune:143 time cost(ms) 70, db_read 7421
// prune:144 time cost(ms) 69, db_read 7383
// prune:145 time cost(ms) 69, db_read 7344
// prune:146 time cost(ms) 79, db_read 7301
// prune:147 time cost(ms) 72, db_read 7271
// prune:148 time cost(ms) 73, db_read 7230
// prune:149 time cost(ms) 71, db_read 7195
// prune:150 time cost(ms) 73, db_read 7162
// prune:151 time cost(ms) 70, db_read 7132
// prune:152 time cost(ms) 70, db_read 7096
// prune:153 time cost(ms) 69, db_read 7049
// prune:154 time cost(ms) 68, db_read 7012
// prune:155 time cost(ms) 69, db_read 6976
// prune:156 time cost(ms) 67, db_read 6933
// prune:157 time cost(ms) 67, db_read 6898
// prune:158 time cost(ms) 66, db_read 6861
// prune:159 time cost(ms) 65, db_read 6811
// prune:160 time cost(ms) 67, db_read 6772
// prune:161 time cost(ms) 78, db_read 6724
// prune:162 time cost(ms) 69, db_read 6677
// prune:163 time cost(ms) 67, db_read 6637
// prune:164 time cost(ms) 66, db_read 6597
// prune:165 time cost(ms) 68, db_read 6546
// prune:166 time cost(ms) 65, db_read 6503
// prune:167 time cost(ms) 65, db_read 6457
// prune:168 time cost(ms) 65, db_read 6417
// prune:169 time cost(ms) 63, db_read 6365
// prune:170 time cost(ms) 63, db_read 6315
// prune:171 time cost(ms) 64, db_read 6266
// prune:172 time cost(ms) 61, db_read 6210
// prune:173 time cost(ms) 61, db_read 6171
// prune:174 time cost(ms) 60, db_read 6124
// prune:175 time cost(ms) 60, db_read 6074
// prune:176 time cost(ms) 61, db_read 6027
// prune:177 time cost(ms) 69, db_read 5972
// prune:178 time cost(ms) 62, db_read 5925
// prune:179 time cost(ms) 62, db_read 5875
// prune:180 time cost(ms) 60, db_read 5825
// prune:181 time cost(ms) 61, db_read 5784
// prune:182 time cost(ms) 60, db_read 5734
// prune:183 time cost(ms) 58, db_read 5679
// prune:184 time cost(ms) 57, db_read 5630
// prune:185 time cost(ms) 58, db_read 5585
// prune:186 time cost(ms) 58, db_read 5533
// prune:187 time cost(ms) 56, db_read 5475
// prune:188 time cost(ms) 65, db_read 5418
// prune:189 time cost(ms) 60, db_read 5365
// prune:190 time cost(ms) 56, db_read 5307
// prune:191 time cost(ms) 53, db_read 5248
// prune:192 time cost(ms) 52, db_read 5188
// prune:193 time cost(ms) 53, db_read 5132
// prune:194 time cost(ms) 65, db_read 5077
// prune:195 time cost(ms) 53, db_read 5015
// prune:196 time cost(ms) 54, db_read 4960
// prune:197 time cost(ms) 54, db_read 4904
// prune:198 time cost(ms) 52, db_read 4848
// prune:199 time cost(ms) 52, db_read 4786
// prune:200 time cost(ms) 51, db_read 4734
// prune:201 time cost(ms) 51, db_read 4666
// prune:202 time cost(ms) 49, db_read 4600
// prune:203 time cost(ms) 48, db_read 4542
// prune:204 time cost(ms) 47, db_read 4476
// prune:205 time cost(ms) 48, db_read 4409
// prune:206 time cost(ms) 47, db_read 4348
// prune:207 time cost(ms) 47, db_read 4288
// prune:208 time cost(ms) 46, db_read 4222
// prune:209 time cost(ms) 46, db_read 4155
// prune:210 time cost(ms) 44, db_read 4086
// prune:211 time cost(ms) 43, db_read 4022
// prune:212 time cost(ms) 42, db_read 3963
// prune:213 time cost(ms) 41, db_read 3892
// prune:214 time cost(ms) 41, db_read 3820
// prune:215 time cost(ms) 41, db_read 3751
// prune:216 time cost(ms) 51, db_read 3665
// prune:217 time cost(ms) 42, db_read 3601
// prune:218 time cost(ms) 40, db_read 3529
// prune:219 time cost(ms) 43, db_read 3463
// prune:220 time cost(ms) 39, db_read 3398
// prune:221 time cost(ms) 40, db_read 3332
// prune:222 time cost(ms) 37, db_read 3253
// prune:223 time cost(ms) 37, db_read 3172
// prune:224 time cost(ms) 37, db_read 3102
// prune:225 time cost(ms) 36, db_read 3008
// prune:226 time cost(ms) 34, db_read 2929
// prune:227 time cost(ms) 33, db_read 2848
// prune:228 time cost(ms) 33, db_read 2779
// prune:229 time cost(ms) 31, db_read 2695
// prune:230 time cost(ms) 31, db_read 2614
// prune:231 time cost(ms) 30, db_read 2532
// prune:232 time cost(ms) 29, db_read 2449
// prune:233 time cost(ms) 29, db_read 2369
// prune:234 time cost(ms) 27, db_read 2281
// prune:235 time cost(ms) 27, db_read 2199
// prune:236 time cost(ms) 25, db_read 2122
// prune:237 time cost(ms) 22, db_read 1490
// prune:238 time cost(ms) 17, db_read 553
// prune:239 time cost(ms) 14, db_read 219
// prune:240 time cost(ms) 14, db_read 232
// prune:241 time cost(ms) 13, db_read 255
// prune:242 time cost(ms) 13, db_read 258
// prune:243 time cost(ms) 12, db_read 172
// prune:244 time cost(ms) 11, db_read 177
// prune:245 time cost(ms) 11, db_read 120
// prune:246 time cost(ms) 10, db_read 121
// prune:247 time cost(ms) 10, db_read 123
// prune:248 time cost(ms) 10, db_read 118
// prune:249 time cost(ms) 8, db_read 122
// prune:250 time cost(ms) 8, db_read 94
// prune:251 time cost(ms) 6, db_read 89
// prune:252 time cost(ms) 5, db_read 75
// prune:253 time cost(ms) 5, db_read 60
// prune:254 time cost(ms) 3, db_read 46
// prune:255 time cost(ms) 2, db_read 37
// prune total time cost(ms):18227, ave:71, total db read:1859652, ave:7264
// commit prune cost:75
//-----------------first test result------------------//
