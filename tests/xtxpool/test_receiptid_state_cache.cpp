#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xdata/xblocktool.h"
#include "xtxpool_v2/xreceiptid_state_cache.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;

class test_receiptid_state_cache : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_receiptid_state_cache, unconfirm_id_section) {
    xtable_shortid_t tableid_a = 0;
    xtable_shortid_t tableid_b = 1;
    base::xtable_index_t table_idx_a(tableid_a);
    base::xtable_index_t table_idx_b(tableid_b);
    base::xreceiptid_state_ptr_t receiptid_state_a = std::make_shared<base::xreceiptid_state_t>(tableid_a, 100);
    base::xreceiptid_state_ptr_t receiptid_state_b = std::make_shared<base::xreceiptid_state_t>(tableid_b, 200);

    uint64_t confirm_id = 0;
    uint64_t unconfirm_id_max = 0;
    xreceiptid_state_cache_t receiptid_state_cache;

    // case 1 : rsp id even, receiver b fall behind
    xreceiptid_pair_t receiptid_pair_a_b_1(10, 8, 0, 0, 0);
    receiptid_state_a->add_pair(tableid_b, receiptid_pair_a_b_1);

    xreceiptid_pair_t receiptid_pair_b_a_1(0, 0, 7, 0, 0);
    receiptid_state_b->add_pair(tableid_a, receiptid_pair_b_a_1);

    receiptid_state_cache.update_table_receiptid_state(nullptr, receiptid_state_a);
    auto unconfirm_tx_nums = receiptid_state_cache.get_unconfirm_tx_nums();
    auto unconfirm_num_a = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_a.get_zone_index(), table_idx_a.get_subaddr());
    auto unconfirm_num_b = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_b.get_zone_index(), table_idx_b.get_subaddr());
    ASSERT_EQ(unconfirm_num_a, 0);
    ASSERT_EQ(unconfirm_num_b, 10);

    receiptid_state_cache.update_table_receiptid_state(nullptr, receiptid_state_b);
    unconfirm_tx_nums = receiptid_state_cache.get_unconfirm_tx_nums();
    unconfirm_num_a = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_a.get_zone_index(), table_idx_a.get_subaddr());
    unconfirm_num_b = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_b.get_zone_index(), table_idx_b.get_subaddr());
    ASSERT_EQ(unconfirm_num_a, 0);
    ASSERT_EQ(unconfirm_num_b, 3);

    receiptid_state_cache.get_unconfirm_id_section_as_sender(tableid_a, tableid_b, confirm_id, unconfirm_id_max, false);
    ASSERT_EQ(confirm_id, 8);
    ASSERT_EQ(unconfirm_id_max, 10);
    receiptid_state_cache.get_unconfirm_id_section_as_sender(tableid_a, tableid_b, confirm_id, unconfirm_id_max, true);
    ASSERT_EQ(confirm_id, 8);
    ASSERT_EQ(unconfirm_id_max, 8);
    
    receiptid_state_cache.get_unconfirm_id_section_as_receiver(tableid_b, tableid_a, confirm_id, unconfirm_id_max);
    ASSERT_EQ(confirm_id, 8);
    ASSERT_EQ(unconfirm_id_max, 8);

    // case 2 : rsp id even, normal
    base::xreceiptid_state_ptr_t receiptid_state_b_2 = std::make_shared<base::xreceiptid_state_t>(tableid_b, 201);
    xreceiptid_pair_t receiptid_pair_b_a_2(0, 0, 9, 0, 0);
    receiptid_state_b_2->add_pair(tableid_a, receiptid_pair_b_a_2);

    receiptid_state_cache.update_table_receiptid_state(nullptr, receiptid_state_b_2);
    unconfirm_tx_nums = receiptid_state_cache.get_unconfirm_tx_nums();
    unconfirm_num_a = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_a.get_zone_index(), table_idx_a.get_subaddr());
    unconfirm_num_b = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_b.get_zone_index(), table_idx_b.get_subaddr());
    ASSERT_EQ(unconfirm_num_a, 0);
    ASSERT_EQ(unconfirm_num_b, 1);

    receiptid_state_cache.get_unconfirm_id_section_as_sender(tableid_a, tableid_b, confirm_id, unconfirm_id_max, false);
    ASSERT_EQ(confirm_id, 9);
    ASSERT_EQ(unconfirm_id_max, 10);
    receiptid_state_cache.get_unconfirm_id_section_as_sender(tableid_a, tableid_b, confirm_id, unconfirm_id_max, true);
    ASSERT_EQ(confirm_id, 9);
    ASSERT_EQ(unconfirm_id_max, 9);
    
    receiptid_state_cache.get_unconfirm_id_section_as_receiver(tableid_b, tableid_a, confirm_id, unconfirm_id_max);
    ASSERT_EQ(confirm_id, 9);
    ASSERT_EQ(unconfirm_id_max, 9);

    // case 3 : rsp id even, sender a fall behind
    base::xreceiptid_state_ptr_t receiptid_state_b_3 = std::make_shared<base::xreceiptid_state_t>(tableid_b, 202);
    xreceiptid_pair_t receiptid_pair_b_a_3(0, 0, 11, 0, 0);
    receiptid_state_b_3->add_pair(tableid_a, receiptid_pair_b_a_3);

    receiptid_state_cache.update_table_receiptid_state(nullptr, receiptid_state_b_3);
    unconfirm_tx_nums = receiptid_state_cache.get_unconfirm_tx_nums();
    unconfirm_num_a = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_a.get_zone_index(), table_idx_a.get_subaddr());
    unconfirm_num_b = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_b.get_zone_index(), table_idx_b.get_subaddr());
    ASSERT_EQ(unconfirm_num_a, 0);
    ASSERT_EQ(unconfirm_num_b, -1);

    receiptid_state_cache.get_unconfirm_id_section_as_sender(tableid_a, tableid_b, confirm_id, unconfirm_id_max, false);
    ASSERT_EQ(confirm_id, 10);
    ASSERT_EQ(unconfirm_id_max, 10);
    receiptid_state_cache.get_unconfirm_id_section_as_sender(tableid_a, tableid_b, confirm_id, unconfirm_id_max, true);
    ASSERT_EQ(confirm_id, 10);
    ASSERT_EQ(unconfirm_id_max, 10);
    
    receiptid_state_cache.get_unconfirm_id_section_as_receiver(tableid_b, tableid_a, confirm_id, unconfirm_id_max);
    ASSERT_EQ(confirm_id, 10);
    ASSERT_EQ(unconfirm_id_max, 11);

    
    // case 4 : rsp id not even
    base::xreceiptid_state_ptr_t receiptid_state_a_2 = std::make_shared<base::xreceiptid_state_t>(tableid_a, 101);
    xreceiptid_pair_t receiptid_pair_a_b_4(12, 8, 0, 1, 0);
    receiptid_state_a_2->add_pair(tableid_b, receiptid_pair_a_b_4);

    receiptid_state_cache.update_table_receiptid_state(nullptr, receiptid_state_a_2);
    unconfirm_tx_nums = receiptid_state_cache.get_unconfirm_tx_nums();
    unconfirm_num_a = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_a.get_zone_index(), table_idx_a.get_subaddr());
    unconfirm_num_b = unconfirm_tx_nums.get_unconfirm_tx_num(table_idx_b.get_zone_index(), table_idx_b.get_subaddr());
    ASSERT_EQ(unconfirm_num_a, 0);
    ASSERT_EQ(unconfirm_num_b, 1);

    receiptid_state_cache.get_unconfirm_id_section_as_sender(tableid_a, tableid_b, confirm_id, unconfirm_id_max, false);
    ASSERT_EQ(confirm_id, 8);
    ASSERT_EQ(unconfirm_id_max, 12);
    receiptid_state_cache.get_unconfirm_id_section_as_sender(tableid_a, tableid_b, confirm_id, unconfirm_id_max, true);
    ASSERT_EQ(confirm_id, 8);
    ASSERT_EQ(unconfirm_id_max, 11);
    
    receiptid_state_cache.get_unconfirm_id_section_as_receiver(tableid_b, tableid_a, confirm_id, unconfirm_id_max);
    ASSERT_EQ(confirm_id, 8);
    ASSERT_EQ(unconfirm_id_max, 11);
}
