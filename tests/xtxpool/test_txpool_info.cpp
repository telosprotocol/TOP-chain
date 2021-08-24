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
    xtxpool_shard_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;

    xtxpool_table_info_t table1("table_test1", &shard, &statistic);
    xtxpool_table_info_t table2("table_test2", &shard, &statistic);

    
    // table1.send_tx_inc(1);
    // ASSERT_EQ(false, table1.is_send_tx_reached_upper_limit());
    // table1.send_tx_inc(table_send_tx_queue_size_max - 1);
    // ASSERT_EQ(true, table1.is_send_tx_reached_upper_limit());

    table2.send_tx_inc(1);
    ASSERT_EQ(false, table2.is_send_tx_reached_upper_limit());
    table2.send_tx_inc(table_send_tx_queue_size_max - 1);
    ASSERT_EQ(true, table2.is_send_tx_reached_upper_limit());


    table2.send_tx_inc(shard_send_tx_queue_size_max - table_send_tx_queue_size_max);
    ASSERT_EQ(true, table1.is_send_tx_reached_upper_limit());

    ASSERT_EQ(true, table1.is_send_tx_reached_upper_limit());
}
