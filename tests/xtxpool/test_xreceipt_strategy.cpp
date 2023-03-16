#include "gtest/gtest.h"
#include "xtxpool_service_v2/xreceipt_strategy.h"
#include "xdata/xverifier/xverifier_utl.h"

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
    for (uint64_t i = 0; i < 256; i ++) {
        auto ret = xreceipt_strategy_t::is_time_for_refresh_table(now + i);
        if (ret) {
            select_count++;
        }
    }

    ASSERT_EQ(select_count, 4);
}

// TEST_F(test_receipt_strategy, calc_resend_time) {
//     uint64_t now = xverifier::xtx_utl::get_gmttime_s();
//     ASSERT_EQ(xreceipt_strategy_t::calc_resend_time(now - 63, now), 0);
//     ASSERT_EQ(xreceipt_strategy_t::calc_resend_time(now - 64, now), 1);
//     ASSERT_EQ(xreceipt_strategy_t::calc_resend_time(now - 65, now), 1);
//     ASSERT_EQ(xreceipt_strategy_t::calc_resend_time(now - 127, now), 1);
//     ASSERT_EQ(xreceipt_strategy_t::calc_resend_time(now - 128, now), 2);
//     ASSERT_EQ(xreceipt_strategy_t::calc_resend_time(now - 129, now), 2);
//     ASSERT_EQ(xreceipt_strategy_t::calc_resend_time(now - 191, now), 2);
//     ASSERT_EQ(xreceipt_strategy_t::calc_resend_time(now - 192, now), 3);
//     ASSERT_EQ(xreceipt_strategy_t::calc_resend_time(now - 193, now), 3);
// }

TEST_F(test_receipt_strategy, is_select) {
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

TEST_F(test_receipt_strategy, is_selected_receipt_pull_msg_processor) {
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(100, 0, 4, 0), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(100, 0, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(100, 0, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(100, 0, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(101, 0, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(101, 0, 4, 1), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(101, 0, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(101, 0, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(102, 0, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(102, 0, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(102, 0, 4, 2), true);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(102, 0, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(103, 0, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(103, 0, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(103, 0, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_selected_receipt_pull_msg_processor(103, 0, 4, 3), true);
}


TEST_F(test_receipt_strategy, is_receiptid_state_sender_for_talbe) {
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(100, 0, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(100, 0, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(100, 0, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(100, 0, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(100, 28, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(100, 28, 4, 1), true);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(100, 28, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(100, 28, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(99, 29, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(99, 29, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(99, 29, 4, 2), true);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(99, 29, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(98, 30, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(98, 30, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(98, 30, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(98, 30, 4, 3), true);

    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(97, 31, 4, 0), true);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(97, 31, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(97, 31, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(97, 31, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(164, 28, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(164, 28, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(164, 28, 4, 2), true);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(164, 28, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(163, 29, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(163, 29, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(163, 29, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(163, 29, 4, 3), true);

    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(162, 30, 4, 0), true);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(162, 30, 4, 1), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(162, 30, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(162, 30, 4, 3), false);

    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(161, 31, 4, 0), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(161, 31, 4, 1), true);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(161, 31, 4, 2), false);
    ASSERT_EQ(xreceipt_strategy_t::is_receiptid_state_sender_for_talbe(161, 31, 4, 3), false);
}

