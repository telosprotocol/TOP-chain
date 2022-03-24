#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xvledger/xreceiptid.h"

using namespace top;
using namespace top::base;

class test_receiptid : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_receiptid, sendids_check)
{
    xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(1, 100);
    xreceiptid_pair_t pair1(1, 0, 0, 0, 0);
    receiptid_state->add_pair(0, pair1);

    xsendids_check_t sendids_check;
    sendids_check.set_id(0, 2);
    ASSERT_EQ(1, sendids_check.size());
    bool ret = sendids_check.check_continuous(receiptid_state);
    ASSERT_EQ(ret, true);
    sendids_check.set_id(0, 4);
    ret = sendids_check.check_continuous(receiptid_state);
    ASSERT_EQ(ret, false);
    sendids_check.set_id(0, 3);
    ret = sendids_check.check_continuous(receiptid_state);
    ASSERT_EQ(ret, true);

    xreceiptid_pairs_ptr_t modified_pairs = std::make_shared<base::xreceiptid_pairs_t>();

    sendids_check.get_modified_pairs(receiptid_state, modified_pairs);
    ASSERT_EQ(modified_pairs->get_size(), 1);
    auto & pairs = modified_pairs->get_all_pairs();
    ASSERT_EQ(pairs.size(), 1);
    auto iter = pairs.find(0);
    ASSERT_EQ(iter->first, 0);
    auto & receipt_pair = iter->second;
    ASSERT_EQ(receipt_pair.get_sendid_max(), 4);
}
