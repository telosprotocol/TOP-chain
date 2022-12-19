#include <gtest/gtest.h>

#include "xsync/xsync_message.h"
#include "common_func.h"
#include "xsync/xsync_util.h"
#include "xsync/xsync_store_shadow.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "xblockstore/src/xvblockhub.h"

using namespace top;
using namespace top::sync;

using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;

#define MAX_BLOCK_TEST (1500)

class test_xsync_chain_down : public testing::Test {
public:
    void SetUp() override
    {
        m_creator = new mock::xvchain_creator(true);
        m_creator->create_blockstore_with_xstore();
        m_blockstore = m_creator->get_blockstore();

        m_mocktable.genrate_table_chain(MAX_BLOCK_TEST, m_blockstore);
        m_height_set.insert(0);
    }
    void TearDown() override
    {
    }

    void save_block(uint64_t start_height, uint64_t end_height);

    void test_interval_no_blocks_in_span(uint64_t cp_start_height, uint64_t cp_end_height);
    void test_interval_blocks_in_span(uint64_t cp_start_height, uint64_t cp_end_height, uint64_t check_start_height, uint64_t check_end_height);

public:
    mock::xvchain_creator* m_creator { nullptr };
    base::xvblockstore_t* m_blockstore { nullptr };
    mock::xdatamock_table m_mocktable;
    std::set<uint64_t> m_height_set; // to save span height
};

void test_xsync_chain_down::save_block(uint64_t store_start_height, uint64_t store_count)
{
    std::string address = m_mocktable.get_account();
    xvaccount_t account(address);
    const std::vector<xblock_ptr_t>& tables = m_mocktable.get_history_tables();
    xassert(tables.size() == (MAX_BLOCK_TEST + 1));

    uint64_t start_height = store_start_height;
    for (uint64_t i = 0; i < store_count; i++) {
        auto curr_block = tables[start_height].get();
        ASSERT_TRUE(m_blockstore->store_block(account, curr_block));
        m_height_set.insert(start_height);
        start_height++;
    }
}

void test_xsync_chain_down::test_interval_no_blocks_in_span(uint64_t cp_start_height, uint64_t cp_end_height)
{
    std::string address = m_mocktable.get_account();
    xvaccount_t account(address);

    std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
    xsync_store_t sync_store("", make_observer(m_blockstore), store_shadow.get());

    std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, &sync_store, address);
    chain_spans_new->initialize();
    for (auto height : m_height_set) {
        chain_spans_new->set(height);
    }

    std::pair<uint64_t, uint64_t> height_interval(cp_start_height, cp_end_height);
    auto pair_result = chain_spans_new->get_continuous_unused_interval(height_interval);
    ASSERT_EQ(pair_result.first, cp_start_height);
    ASSERT_EQ(pair_result.second, cp_end_height);
}

void test_xsync_chain_down::test_interval_blocks_in_span(uint64_t cp_start_height, uint64_t cp_end_height, uint64_t check_start_height, uint64_t check_end_height)
{
    std::string address = m_mocktable.get_account();
    xvaccount_t account(address);

    std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
    xsync_store_t sync_store("", make_observer(m_blockstore), store_shadow.get());

    std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, &sync_store, address);
    chain_spans_new->initialize();
    for (auto height : m_height_set) {
        chain_spans_new->set(height);
    }

    std::pair<uint64_t, uint64_t> height_interval(cp_start_height, cp_end_height);
    auto pair_result = chain_spans_new->get_continuous_unused_interval(height_interval);
    ASSERT_EQ(pair_result.first, check_start_height);
    ASSERT_EQ(pair_result.second, check_end_height);
}

// blockstore only genesis, input[1,cp_end_height] and output [1,cp_end_height]
TEST_F(test_xsync_chain_down, test_xsync_empty_test)
{
    test_interval_no_blocks_in_span(1, 50);
    test_interval_blocks_in_span(0, 50, 1, 50);
}

// blockstore save [0],[20-50],
TEST_F(test_xsync_chain_down, test_xsync_0_20_test)
{
    save_block(20, 31);
    test_interval_blocks_in_span(1, 50, 1, 19);
}

// blockstore save [0,19],[20-50],
TEST_F(test_xsync_chain_down, test_xsync_19_19_test)
{
    save_block(1, 18);
    save_block(20, 31);
    test_interval_blocks_in_span(1, 50, 19, 19);
}

// // blockstore save [0,50],
TEST_F(test_xsync_chain_down, test_xsync_0_50_test)
{
    save_block(1, 50);
    test_interval_blocks_in_span(1, 50, 0, 0);
    test_interval_blocks_in_span(1, 70, 51, 70);
}

// blockstore save [0,77]]
TEST_F(test_xsync_chain_down, test_xsync_0_77_test)
{
    save_block(1, 77);
    test_interval_blocks_in_span(1, 70, 0, 0);
    test_interval_blocks_in_span(1, 100, 78, 100);
}

// blockstore save [0,80]
TEST_F(test_xsync_chain_down, test_xsync_0_80_test)
{
    save_block(1, 80);
    test_interval_blocks_in_span(1, 70, 0, 0);
    test_interval_blocks_in_span(1, 100, 81, 100);
}

// blockstore save [0,80][90,100]
TEST_F(test_xsync_chain_down, test_xsync_0_80_90_100_test)
{
    save_block(1, 80);
    save_block(90, 100);
    test_interval_blocks_in_span(1, 82, 81, 82);
    test_interval_blocks_in_span(1, 100, 81, 89);
}

// blockstore save [0,80]
TEST_F(test_xsync_chain_down, test_xsync_0_80_82_150_test)
{
    save_block(1, 80);
    test_interval_blocks_in_span(82, 150, 82, 150);
}

// blockstore save [0,80]
TEST_F(test_xsync_chain_down, test_xsync_100_150_test)
{
    save_block(1, 100);
    test_interval_blocks_in_span(60, 120, 101, 120);
}

TEST_F(test_xsync_chain_down, test_xsync_999_test)
{
    save_block(1, 999);
    test_interval_blocks_in_span(1000, 1100, 1000, 1100);
}

// blockstore save [0,80]
TEST_F(test_xsync_chain_down, test_xsync_1024_test)
{
    save_block(1, 1100);
    test_interval_blocks_in_span(1000, 1200, 1101, 1200);
}

