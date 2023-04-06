#include <gtest/gtest.h>

#include "xsync/xsync_message.h"
#include "common_func.h"
#include "xsync/xsync_util.h"
#include "xsync/xsync_store_shadow.h"
#include "xsync/xchain_downloader.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "xblockstore/src/xvblockhub.h"
#include "xdata/xblocktool.h"


using namespace top;
using namespace top::sync;

using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;

#define MAX_BLOCK_TEST (1500)

class test_xsync_span : public testing::Test {
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
    void save_empty_block();

    void test_interval_no_blocks_in_span(uint64_t cp_start_height, uint64_t cp_end_height);
    void test_interval_blocks_in_span(uint64_t cp_start_height, uint64_t cp_end_height, uint64_t check_start_height, uint64_t check_end_height);

public:
    mock::xvchain_creator* m_creator { nullptr };
    base::xvblockstore_t* m_blockstore { nullptr };
    mock::xdatamock_table m_mocktable;
    std::set<uint64_t> m_height_set; // to save span height
};

void test_xsync_span::save_block(uint64_t store_start_height, uint64_t store_count)
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

    //auto height =  m_blockstore->update_get_latest_cp_connected_block_height(address);
    //std::cout << "save_block  height " << height << std::endl;
}

void test_xsync_span::save_empty_block()
{
     std::string address = m_mocktable.get_account();
     xvaccount_t account(address);

     auto disconnect_block = m_mocktable.generate_tableblock();
     uint64_t disconnect_height = disconnect_block->get_height();

     m_mocktable.genrate_table_chain(50, m_blockstore);
     const std::vector<xblock_ptr_t>& tables = m_mocktable.get_history_tables();
     xassert(tables.size() == (MAX_BLOCK_TEST + 50 + 1));

     /*
                                  /--meptyblock disconnect_height
                                 /
     block(disconnect_height-1)--
                                 \
                                  \--block(h=disconnect_height+1)---blocks
   */

     for (uint64_t i = 0; i < tables.size(); i++) {
        if (i == (disconnect_height)) {
            ASSERT_TRUE(m_blockstore->store_block(account, disconnect_block.get()));
            continue;
        }
        m_height_set.insert(i);
        auto curr_block = tables[i].get();
        ASSERT_TRUE(m_blockstore->store_block(account, curr_block));
     }
}

void test_xsync_span::test_interval_no_blocks_in_span(uint64_t cp_start_height, uint64_t cp_end_height)
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

void test_xsync_span::test_interval_blocks_in_span(uint64_t cp_start_height, uint64_t cp_end_height, uint64_t check_start_height, uint64_t check_end_height)
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
TEST_F(test_xsync_span, test_xsync_empty_test_BENCH)
{
    test_interval_no_blocks_in_span(1, 50);
    test_interval_blocks_in_span(0, 50, 1, 50);
}

// blockstore save [0],[20-50],
TEST_F(test_xsync_span, test_xsync_0_20_test_BENCH)
{
    save_block(20, 31);
    test_interval_blocks_in_span(1, 50, 1, 19);
}

// blockstore save [0,19],[20-50],
TEST_F(test_xsync_span, test_xsync_19_19_test_BENCH)
{
    save_block(1, 18);
    save_block(20, 31);
    test_interval_blocks_in_span(1, 50, 19, 19);
}

// // blockstore save [0,50],
TEST_F(test_xsync_span, test_xsync_0_50_test_BENCH)
{
    save_block(1, 50);
    test_interval_blocks_in_span(1, 50, 0, 0);
    test_interval_blocks_in_span(1, 70, 51, 70);
}

// blockstore save [0,77]]
TEST_F(test_xsync_span, test_xsync_0_77_test_BENCH)
{
    save_block(1, 77);
    test_interval_blocks_in_span(1, 70, 0, 0);
    test_interval_blocks_in_span(1, 100, 78, 100);
}

// blockstore save [0,80]
TEST_F(test_xsync_span, test_xsync_0_80_test_BENCH)
{
    save_block(1, 80);
    test_interval_blocks_in_span(1, 70, 0, 0);
    test_interval_blocks_in_span(1, 100, 81, 100);
}

// blockstore save [0,80][90,100]
TEST_F(test_xsync_span, test_xsync_0_80_90_100_test_BENCH)
{
    save_block(1, 80);
    save_block(90, 100);
    test_interval_blocks_in_span(1, 82, 81, 82);
    test_interval_blocks_in_span(1, 100, 81, 89);
}

// blockstore save [0,80]
TEST_F(test_xsync_span, test_xsync_0_80_82_150_test_BENCH)
{
    save_block(1, 80);
    test_interval_blocks_in_span(82, 150, 82, 150);
}

// blockstore save [0,80]
TEST_F(test_xsync_span, test_xsync_100_150_test_BENCH)
{
    save_block(1, 100);
    test_interval_blocks_in_span(60, 120, 101, 120);
}

TEST_F(test_xsync_span, test_xsync_999_test_BENCH)
{
    save_block(1, 999);
    test_interval_blocks_in_span(1000, 1100, 1000, 1100);
}

// blockstore save [0,80]
TEST_F(test_xsync_span, test_xsync_1024_test_BENCH)
{
    save_block(1, 1100);
    test_interval_blocks_in_span(1000, 1200, 1101, 1200);
}

TEST_F(test_xsync_span, test_xsync_span_genesis_time_reflash_BENCH)
{
   
    save_block(1, 6);
    save_block(7, 2);
    save_block(11, 4);

    std::string address = m_mocktable.get_account();
    xvaccount_t account(address);

    std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
    xsync_store_t sync_store("", make_observer(m_blockstore), store_shadow.get());

    std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, &sync_store, address);
    chain_spans_new->initialize();
    uint64_t refresh_time_1 = 0;
    uint64_t refresh_time_2 = 0;
    for (auto height : m_height_set) {
        chain_spans_new->set(height);
        if(0 == refresh_time_1) {
            refresh_time_1 = chain_spans_new->genesis_height_refresh_time_ms();
            refresh_time_2 = chain_spans_new->genesis_height_refresh_time_ms();
        }
        sleep(1);
        if(0 < height && height < 8) {
            refresh_time_2 = chain_spans_new->genesis_height_refresh_time_ms();
            ASSERT_NE(refresh_time_1, refresh_time_2);
        } else if(height == 8) {
            refresh_time_1 = chain_spans_new->genesis_height_refresh_time_ms();
        } else { 
            refresh_time_2 = chain_spans_new->genesis_height_refresh_time_ms();
            ASSERT_EQ(refresh_time_1, refresh_time_2);
        }
    }
    uint64_t genesis_height = chain_spans_new->genesis_connect_height();
    ASSERT_EQ(8, genesis_height);
}

TEST_F(test_xsync_span, test_xsync_cp_reflash_lost_BENCH)
{
    save_block(1, 8);
    save_block(11, 10);

    std::string address = m_mocktable.get_account();
    xvaccount_t account(address);

    std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
    xsync_store_t sync_store("", make_observer(m_blockstore), store_shadow.get());

    std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, &sync_store, address);
    chain_spans_new->initialize();

    xchain_object_t cp_object;
    vnetwork::xvnode_address_t self_addr, target_addr;

    for (auto height : m_height_set) {
        chain_spans_new->set(height);
        cp_object.set_height(height);
    }

    uint64_t cp_lost_height = sync_store.get_latest_end_block_height(address, (enum_chain_sync_policy)2);
    ASSERT_EQ(8, cp_lost_height);
    
    std::multimap<uint64_t, mbus::chain_behind_event_address> chain_behind_address_map{};
    mbus::chain_behind_event_address chain_behind_address_info;
    chain_behind_address_info.self_addr = self_addr;
    chain_behind_address_info.from_addr = target_addr;
    chain_behind_address_info.start_height = 0;
    chain_behind_address_map.insert(std::make_pair( MAX_BLOCK_TEST + 10, chain_behind_address_info));
    cp_object.set_object_t(chain_behind_address_map);

    // first record
    int64_t now = 200000;
    cp_object.check_and_fix_behind_height(now, &sync_store, 2, address);
    std::pair<uint64_t, uint64_t> interval;
    cp_object.pick(interval, self_addr, target_addr);
    ASSERT_EQ(7, interval.first);
}

TEST_F(test_xsync_span, test_xsync_or_cp_reflash_disconnect_BENCH)
{
    save_empty_block();

    std::string address = m_mocktable.get_account();
    xvaccount_t account(address);

    std::unique_ptr<xsync_store_shadow_t> store_shadow(top::make_unique<sync::xsync_store_shadow_t>());
    xsync_store_t sync_store("", make_observer(m_blockstore), store_shadow.get());

    std::shared_ptr<xsync_chain_spans_t> chain_spans_new = std::make_shared<xsync_chain_spans_t>(2, &sync_store, address);
    chain_spans_new->initialize();

    xchain_object_t cp_object;
    vnetwork::xvnode_address_t self_addr, target_addr;

    for (auto height : m_height_set) {
        chain_spans_new->set(height);
        cp_object.set_height(height);
    }

    std::multimap<uint64_t, mbus::chain_behind_event_address> chain_behind_address_map{};
    mbus::chain_behind_event_address chain_behind_address_info;
    chain_behind_address_info.self_addr = self_addr;
    chain_behind_address_info.from_addr = target_addr;
    chain_behind_address_info.start_height = 0;
    chain_behind_address_map.insert(std::make_pair(MAX_BLOCK_TEST + 10, chain_behind_address_info));
    cp_object.set_object_t(chain_behind_address_map);

    uint64_t cp_disconnect_height = sync_store.get_latest_end_block_height(address, (enum_chain_sync_policy)2);
    ASSERT_EQ(MAX_BLOCK_TEST + 1, cp_disconnect_height);

    int64_t now = 200000;
    cp_object.check_and_fix_behind_height(now, &sync_store, 2, address);
    std::pair<uint64_t, uint64_t> interval;
    cp_object.pick(interval, self_addr, target_addr);
    ASSERT_EQ(MAX_BLOCK_TEST, interval.first);
}

