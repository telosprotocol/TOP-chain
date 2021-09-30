#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xtxpool_v2/xunconfirm_id_height.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_unconfirm_id_height : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_unconfirm_id_height, processed_table_height) {
    uint64_t test_arr[] = {0, 59, 63, 64, 100, 127, 128, 193, 192, 191, 200000, 1000, 13343};

    xprocessed_height_record_t processed_table_height;
    for (uint32_t i = 0; i < sizeof(test_arr) / sizeof(uint64_t); i++) {
        processed_table_height.record_height(test_arr[i]);
        ASSERT_EQ(processed_table_height.is_record_height(test_arr[i]), true);
    }

    processed_table_height.update_min_height(100);
    for (uint32_t i = 0; i < sizeof(test_arr) / sizeof(uint64_t); i++) {
        ASSERT_EQ(processed_table_height.is_record_height(test_arr[i]), (test_arr[i] >= 100));
    }

    uint64_t left_end;
    uint64_t right_end;
    processed_table_height.get_latest_lacking_saction(left_end, right_end, 50);
    ASSERT_EQ(left_end, 199950);
    ASSERT_EQ(right_end, 199999);

    processed_table_height.record_height(199999);
    processed_table_height.record_height(199998);
    processed_table_height.record_height(199997);
    processed_table_height.record_height(199996);
    processed_table_height.get_latest_lacking_saction(left_end, right_end, 50);
    ASSERT_EQ(left_end, 199946);
    ASSERT_EQ(right_end, 199995);

    processed_table_height.record_height(199994);
    processed_table_height.get_latest_lacking_saction(left_end, right_end, 50);
    ASSERT_EQ(left_end, 199995);
    ASSERT_EQ(right_end, 199995);

    xprocessed_height_record_t processed_table_height2;
    for (uint32_t i = 0; i < sizeof(test_arr) / sizeof(uint64_t); i++) {
        ASSERT_EQ(processed_table_height2.is_record_height(test_arr[i]), false);
    }
}

TEST_F(test_unconfirm_id_height, unconfirm_id_height_list) {
    xunconfirm_id_height_list_t unconfirm_list;
    ASSERT_EQ(unconfirm_list.is_all_loaded(), false);
    unconfirm_list.update_confirm_id(99);
    ASSERT_EQ(unconfirm_list.is_all_loaded(), true);
    unconfirm_list.add_id_height(101, 1000, 100000);
    ASSERT_EQ(unconfirm_list.is_all_loaded(), false);
    unconfirm_list.update_confirm_id(100);
    ASSERT_EQ(unconfirm_list.is_all_loaded(), true);
    uint64_t height;
    bool ret = unconfirm_list.get_height_by_id(101, height);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(height, 1000);
    ret = unconfirm_list.get_height_by_id(102, height);
    ASSERT_EQ(ret, false);
    unconfirm_list.add_id_height(102, 1001, 100010);
    ret = unconfirm_list.get_height_by_id(102, height);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(height, 1001);
    uint64_t receiptid;
    ret = unconfirm_list.get_resend_id_height(receiptid, height, 100030);
    ASSERT_EQ(ret, false);
    ret = unconfirm_list.get_resend_id_height(receiptid, height, 100070);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(receiptid, 102);
    ASSERT_EQ(height, 1001);
}

TEST_F(test_unconfirm_id_height, table_unconfirm_id_height) {
    xtable_unconfirm_id_height_t table_unconfirm;
    ASSERT_EQ(table_unconfirm.is_all_unconfirm_id_recovered(), true);
    table_unconfirm.update_confirm_id(0, 99);
    ASSERT_EQ(table_unconfirm.is_all_unconfirm_id_recovered(), true);
    table_unconfirm.add_id_height(0, 101, 1000, 100000);
    ASSERT_EQ(table_unconfirm.is_all_unconfirm_id_recovered(), false);
    table_unconfirm.update_confirm_id(0, 100);
    ASSERT_EQ(table_unconfirm.is_all_unconfirm_id_recovered(), true);
    uint64_t height;
    bool ret = table_unconfirm.get_height_by_id(0, 101, height);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(height, 1000);
    ret = table_unconfirm.get_height_by_id(0, 102, height);
    ASSERT_EQ(ret, false);
    table_unconfirm.add_id_height(0, 102, 1001, 100010);
    ret = table_unconfirm.get_height_by_id(0, 102, height);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(height, 1001);
    uint64_t receiptid;
    auto id_height_list = table_unconfirm.get_resend_id_height_list(10030);
    ASSERT_EQ(id_height_list.size(), 0);
    id_height_list = table_unconfirm.get_resend_id_height_list(100070);
    ASSERT_EQ(id_height_list.size(), 1);
    ASSERT_EQ(id_height_list[0].table_sid, 0);
    ASSERT_EQ(id_height_list[0].receipt_id, 102);
    ASSERT_EQ(id_height_list[0].height, 1001);
}
