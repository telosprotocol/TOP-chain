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
    xunconfirm_id_height_list_t unconfirm_list(1, 1, true);
    bool is_lacking = false;
    uint64_t min_height = 0;
    unconfirm_list.add_id_height(101, 1000, false, 100000);
    unconfirm_list.update_confirm_id(99);
    is_lacking = unconfirm_list.is_lacking();
    bool ret = unconfirm_list.get_min_height(min_height);
    ASSERT_EQ(ret, false);
    ASSERT_EQ(min_height, 0);
    ASSERT_EQ(is_lacking, true);

    is_lacking = false;
    unconfirm_list.update_confirm_id(100);
    is_lacking = unconfirm_list.is_lacking();
    ret = unconfirm_list.get_min_height(min_height);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(min_height, 1000);
    ASSERT_EQ(is_lacking, false);

    uint64_t height = 0;
    bool need_confirm = false;
    ret = unconfirm_list.get_height_by_id(101, height, need_confirm);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(height, 1000);
    ASSERT_EQ(need_confirm, false);

    uint64_t receipt_id = 0;
    height = 0;
    ret = unconfirm_list.get_resend_id_height(receipt_id, height, 100060);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(receipt_id, 101);
    ASSERT_EQ(height, 1000);

    unconfirm_list.add_id_height(103, 1010, true, 100100);
    unconfirm_list.update_confirm_id(101);
    height = 0;
    ret = unconfirm_list.get_height_by_id(101, height, need_confirm);
    ASSERT_EQ(ret, false);
    is_lacking = unconfirm_list.is_lacking();
    ret = unconfirm_list.get_min_height(min_height);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(min_height, 1001);
    ASSERT_EQ(is_lacking, true);
    std::vector<uint64_t> receipt_ids;
    ret = unconfirm_list.get_need_confirm_ids(102, 103, receipt_ids);
    ASSERT_EQ(ret, false);
    ASSERT_EQ(receipt_ids.size(), 0);

    unconfirm_list.update_confirm_id(102);
    ret = unconfirm_list.get_need_confirm_ids(103, 103, receipt_ids);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(receipt_ids.size(), 1);
    ASSERT_EQ(receipt_ids[0], 103);
}

TEST_F(test_unconfirm_id_height, table_unconfirm_id_height) {
    xtable_unconfirm_id_height_as_sender_t table_unconfirm(1);

    std::set<base::xtable_shortid_t> all_table_sids;
    all_table_sids.insert(1);

    xreceiptid_state_cache_t receiptid_state_cache;
    xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(1, 1000);
    xreceiptid_pair_t pair1(101, 99, 99, 0, 0);
    receiptid_state->add_pair(1, pair1);
    base::xvproperty_prove_ptr_t property_prove_ptr = nullptr;
    receiptid_state_cache.update_table_receiptid_state(property_prove_ptr, receiptid_state);

    bool is_lacking = false;
    table_unconfirm.add_id_height(1, 101, 1000, false, 100000);
    table_unconfirm.update_confirm_id(1, 99);
    uint64_t min_height = 0;
    bool ret = table_unconfirm.get_min_height(receiptid_state_cache, all_table_sids, 1000, min_height, true, is_lacking);
    ASSERT_EQ(ret, false);
    ASSERT_EQ(min_height, 0);
    ASSERT_EQ(is_lacking, true);

    is_lacking = false;
    table_unconfirm.update_confirm_id(1, 100);
    receiptid_state->set_tableid_and_height(1, 1001);
    xreceiptid_pair_t pair2(101, 100, 100, 0, 0);
    receiptid_state->add_pair(1, pair2);
    ret = table_unconfirm.get_min_height(receiptid_state_cache, all_table_sids, 1001, min_height, true, is_lacking);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(min_height, 1000);
    ASSERT_EQ(is_lacking, false);

    uint64_t height = 0;
    bool need_confirm = false;
    ret = table_unconfirm.get_height_by_id(1, 101, height, need_confirm);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(height, 1000);
    ASSERT_EQ(need_confirm, false);

    uint64_t receipt_id = 0;
    height = 0;
    auto resend_id_height_list = table_unconfirm.get_resend_id_height_list(100060);
    ASSERT_EQ(resend_id_height_list.empty(), false);
    ASSERT_EQ(resend_id_height_list[0].receipt_id, 101);
    ASSERT_EQ(resend_id_height_list[0].height, 1000);

    table_unconfirm.add_id_height(1, 103, 1010, true, 100100);
    table_unconfirm.update_confirm_id(1, 101);
    height = 0;
    ret = table_unconfirm.get_height_by_id(1, 101, height, need_confirm);
    ASSERT_EQ(ret, false);
    receiptid_state->set_tableid_and_height(1, 1002);
    xreceiptid_pair_t pair3(103, 100, 101, 0, 0);
    receiptid_state->add_pair(1, pair3);
    ret = table_unconfirm.get_min_height(receiptid_state_cache, all_table_sids, 1002, min_height, true, is_lacking);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(min_height, 1001);
    ASSERT_EQ(is_lacking, true);
    std::vector<uint64_t> receipt_ids;
    ret = table_unconfirm.get_need_confirm_ids(1, 102, 103, receipt_ids);
    ASSERT_EQ(ret, false);
    ASSERT_EQ(receipt_ids.size(), 0);

    table_unconfirm.update_confirm_id(1, 102);
    ret = table_unconfirm.get_need_confirm_ids(1, 103, 103, receipt_ids);
    ASSERT_EQ(ret, true);
    ASSERT_EQ(receipt_ids.size(), 1);
    ASSERT_EQ(receipt_ids[0], 103);
}
