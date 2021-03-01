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


    // bool is_send_tx_reached_upper_limit() {
    //     if (m_counter.get_send_tx_count() >= table_send_tx_queue_size_max || m_counter.get_conf_tx_count() >= table_conf_tx_queue_size_max) {
    //         return true;
    //     }
    //     if (m_shard->get_send_tx_count() >= shard_send_tx_queue_size_max) {
    //         return true;
    //     }
    //     return false;
    // }
    // bool is_recv_tx_reached_upper_limit() {
    //     if (m_counter.get_recv_tx_count() >= table_recv_tx_queue_size_max) {
    //         return true;
    //     }
    //     if (m_shard->get_recv_tx_count() >= shard_recv_tx_queue_size_max) {
    //         return true;
    //     }
    //     return false;
    // }




TEST_F(test_xtxpool_info, txpool_info) {
    xtxpool_shard_info_t shard(0,0,0);

    xtxpool_table_info_t table1("table_test1", &shard);
    xtxpool_table_info_t table2("table_test2", &shard);

    
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
