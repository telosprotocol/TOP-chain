#include "gtest/gtest.h"
#include "tests/mock/xdatamock_address.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"

#include <unistd.h>
#define private public
#define protected public
#include "xstatestore/xaccount_index_cache.h"

using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::mock;
using namespace top::statestore;

class test_account_index_cache : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_account_index_cache, basic) {
    mock::xvchain_creator creator(true);
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    uint64_t max_block_height = 50;
    mock::xdatamock_table mocktable(1, 4);
    mocktable.genrate_table_chain(max_block_height, blockstore);
    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    xassert(tableblocks.size() == max_block_height + 1);

    xaccount_index_cache_t account_index_cache;
    std::map<std::string, base::xaccount_index_t> account_index_map;

    auto accounts = mocktable.get_unit_accounts();

    auto & account0 = accounts[0];
    base::xaccount_index_t account0_index_1 = base::xaccount_index_t(base::enum_xaccountindex_version_state_hash, 1, "11", "111", 1);
    account_index_map[account0] = account0_index_1;

    account_index_cache.update_new_cert_block(tableblocks[1].get(), account_index_map);

    base::xaccount_index_t account0_index_1_get;
    auto ret = account_index_cache.get_account_index(tableblocks[1].get(), account0, account0_index_1_get);
    ASSERT_TRUE(ret == true);
    ASSERT_TRUE(account0_index_1 == account0_index_1_get);

    account_index_map.clear();
    account_index_cache.update_new_cert_block(tableblocks[2].get(), account_index_map);

    base::xaccount_index_t account0_index_1_get2;
    ret = account_index_cache.get_account_index(tableblocks[2].get(), account0, account0_index_1_get2);
    ASSERT_TRUE(ret == true);
    ASSERT_TRUE(account0_index_1 == account0_index_1_get2);

    base::xaccount_index_t account0_index_1_get3;
    ret = account_index_cache.get_account_index(tableblocks[1].get(), account0, account0_index_1_get3);
    ASSERT_TRUE(ret == true);
    ASSERT_TRUE(account0_index_1 == account0_index_1_get3);

    ret = account_index_cache.get_account_index(tableblocks[2].get(), accounts[1], account0_index_1_get3);
    ASSERT_TRUE(ret == false);

    account_index_cache.update_new_cert_block(tableblocks[3].get(), account_index_map);

    base::xaccount_index_t account0_index_1_get4;
    ret = account_index_cache.get_account_index(tableblocks[3].get(), account0, account0_index_1_get4);
    ASSERT_TRUE(ret == true);
    ASSERT_TRUE(account0_index_1 == account0_index_1_get4);

    account_index_cache.update_new_cert_block(tableblocks[4].get(), account_index_map);

    base::xaccount_index_t account0_index_1_get5;
    ret = account_index_cache.get_account_index(tableblocks[4].get(), account0, account0_index_1_get5);
    ASSERT_TRUE(ret == true);
    ASSERT_TRUE(account0_index_1 == account0_index_1_get5);

    account_index_cache.update_new_cert_block(tableblocks[5].get(), account_index_map);

    base::xaccount_index_t account0_index_1_get6;
    ret = account_index_cache.get_account_index(tableblocks[5].get(), account0, account0_index_1_get6);
    ASSERT_TRUE(ret == false);
}
