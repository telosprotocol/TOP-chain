#include "gtest/gtest.h"
#include "xstatestore/xstatestore_prune.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_address.hpp"

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

// class xstatestore_prune_t {
// public:
//     xstatestore_prune_t(common::xaccount_address_t const & table_addr);

// public:
//     void init();
//     void on_table_mpt_and_unitstate_executed(uint64_t table_block_height);
//     void prune(uint64_t latest_executed_height);

// private:
//     uint64_t prune_by_height_section(uint64_t from_height, uint64_t to_height);
//     void prune_imp(const std::shared_ptr<state_mpt::xtop_state_mpt> & mpt, base::xvblock_t* next_block);

// private:
//     mutable std::mutex m_prune_lock;
//     common::xaccount_address_t m_table_addr;
//     uint64_t m_pruned_height{0};
//     xstatestore_base_t m_statestore_base;
// };

TEST_F(test_state_prune, test_state_prune_basic) {
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
    
    auto & vaccount = mocktable.get_vaccount();
    xstatestore_prune_t table_pruner(common::xaccount_address_t(vaccount.get_account()));

    table_pruner.on_table_mpt_and_unitstate_executed(1);

    {
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(vaccount));
        auto prune_height = account_obj->get_lowest_executed_block_height();
        EXPECT_EQ(prune_height, 1);        
    }

    table_pruner.prune(45);
    {
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(vaccount));
        auto prune_height = account_obj->get_lowest_executed_block_height();
        EXPECT_EQ(prune_height, 5);        
    }

    table_pruner.prune(50);
    {
        base::xauto_ptr<base::xvaccountobj_t> account_obj(base::xvchain_t::instance().get_account(vaccount));
        auto prune_height = account_obj->get_lowest_executed_block_height();
        EXPECT_EQ(prune_height, 10);        
    }

}
