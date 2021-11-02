#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xtxpool_v2/xtxpool_info.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_xtxpool_info : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_xtxpool_info, txpool_info) {
    xtxpool_role_info_t shard(0, 0, 15, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, "table_test1");
    xtxpool_table_info_t table1("table_test1", &shard, &statistic, &table_state_cache);
    xtxpool_table_info_t table2("table_test2", &shard, &statistic, &table_state_cache);

    
    // table1.send_tx_inc(1);
    // ASSERT_EQ(false, table1.check_send_tx_reached_upper_limit());
    // table1.send_tx_inc(table_send_tx_queue_size_max - 1);
    // ASSERT_EQ(true, table1.check_send_tx_reached_upper_limit());

    table2.send_tx_inc(1);
    ASSERT_EQ(xsuccess, table2.check_send_tx_reached_upper_limit());
    table2.send_tx_inc(table_send_tx_queue_size_max - 1);
    ASSERT_EQ(xtxpool_error_table_reached_upper_limit, table2.check_send_tx_reached_upper_limit());

    // TODO(jimmy) disable shard limit
    // table2.send_tx_inc(shard_send_tx_queue_size_max - table_send_tx_queue_size_max);
    // ASSERT_EQ(true, table1.is_send_tx_reached_upper_limit());

    // table2.send_tx_inc(role_send_tx_queue_size_max_for_each_table*16 - table_send_tx_queue_size_max);
    // ASSERT_EQ(xtxpool_error_role_reached_upper_limit, table1.check_send_tx_reached_upper_limit());

    // ASSERT_EQ(xtxpool_error_role_reached_upper_limit, table1.check_send_tx_reached_upper_limit());
}
