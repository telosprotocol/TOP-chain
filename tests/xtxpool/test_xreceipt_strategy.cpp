#include "gtest/gtest.h"
#include "xtxpool_service_v2/xreceipt_strategy.h"
#include "xverifier/xverifier_utl.h"

using namespace top::xtxpool_service_v2;
using namespace top;
using namespace top::base;
using namespace std;

class test_receipt_strategy : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

public:
};

TEST_F(test_receipt_strategy, is_time_for_recover) {
    uint32_t select_count = 0;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    for (uint64_t i = 0; i < 256; i++) {
        auto ret = xreceipt_strategy_t::is_time_for_refresh_table(now + i);
        if (ret) {
            select_count++;
        }
    }

    ASSERT_EQ(select_count, 4);
}

TEST_F(test_receipt_strategy, receipt_pull_msg_receiver) {
    uint32_t select_count = 0;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    uint16_t shard_size = 7;

    for (uint64_t i = 0; i < 256; i++) {
        uint64_t hash = rand();
        for (uint16_t j = 0; j < shard_size; j++) {
            auto ret = xreceipt_strategy_t::is_selected_receipt_pull_msg_receiver(hash, now + i, j, shard_size);
            if (ret) {
                select_count++;
            }
        }
    }
    ASSERT_EQ(select_count, 256);
}

TEST_F(test_receipt_strategy, pull_lacking_receipts) {
    uint32_t select_count = 0;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    uint16_t shard_size = 7;

    for (uint32_t table_id = 0; table_id < 64; table_id++) {
        for (uint64_t i = 0; i < 300; i++) {
            for (uint16_t j = 0; j < shard_size; j++) {
                auto ret = xreceipt_strategy_t::is_time_for_node_pull_lacking_receipts(now + i, table_id, shard_size, j);
                if (ret) {
                    select_count++;
                }
            }
        }
        ASSERT_EQ(select_count, 30);
        select_count = 0;
    }
}

TEST_F(test_receipt_strategy, is_selected_pos) {
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 0, 1, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 0, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 0, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 0, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 1, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 1, 1, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 1, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 1, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 2, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 2, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 2, 1, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 2, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 3, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 3, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 3, 1, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 3, 1, 4), true);

    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 0, 2, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 0, 2, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 0, 2, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 0, 2, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 1, 2, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 1, 2, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 1, 2, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 1, 2, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 2, 2, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 2, 2, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 2, 2, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 2, 2, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 3, 2, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 3, 2, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 3, 2, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 3, 2, 4), true);

    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 0, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 0, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 0, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 0, 3, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 1, 3, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 1, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 1, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 1, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 2, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 2, 3, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 2, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 2, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(0, 3, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(1, 3, 3, 4), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(2, 3, 3, 4), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_pos(3, 3, 3, 4), true);
}

TEST_F(test_receipt_strategy, is_resend_node) {
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(100, 0, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(100, 0, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(100, 0, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(100, 0, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(100, 28, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(100, 28, 4, 1), true);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(100, 28, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(100, 28, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(99, 29, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(99, 29, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(99, 29, 4, 2), true);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(99, 29, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(98, 30, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(98, 30, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(98, 30, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(98, 30, 4, 3), true);

    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(97, 31, 4, 0), true);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(97, 31, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(97, 31, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(97, 31, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(164, 28, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(164, 28, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(164, 28, 4, 2), true);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(164, 28, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(163, 29, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(163, 29, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(163, 29, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(163, 29, 4, 3), true);

    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(162, 30, 4, 0), true);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(162, 30, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(162, 30, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(162, 30, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(161, 31, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(161, 31, 4, 1), true);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(161, 31, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_resend_node_for_talbe(161, 31, 4, 3), false);
}
