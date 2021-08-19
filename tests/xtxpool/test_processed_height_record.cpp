#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xtxpool_v2/xprocessed_height_record.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_processed_height_record : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_processed_height_record, processed_table_height) {
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
    processed_table_height.get_latest_lacking_saction(left_end, right_end);
    ASSERT_EQ(left_end, 13344);
    ASSERT_EQ(right_end, 199999);

    processed_table_height.record_height(199999);
    processed_table_height.record_height(199998);
    processed_table_height.record_height(199997);
    processed_table_height.record_height(199996);
    processed_table_height.get_latest_lacking_saction(left_end, right_end);
    ASSERT_EQ(left_end, 13344);
    ASSERT_EQ(right_end, 199995);

    processed_table_height.record_height(199994);
    processed_table_height.get_latest_lacking_saction(left_end, right_end);
    ASSERT_EQ(left_end, 199995);
    ASSERT_EQ(right_end, 199995);

    xprocessed_height_record_t processed_table_height2;
    for (uint32_t i = 0; i < sizeof(test_arr) / sizeof(uint64_t); i++) {
        ASSERT_EQ(processed_table_height2.is_record_height(test_arr[i]), false);
    }
}
